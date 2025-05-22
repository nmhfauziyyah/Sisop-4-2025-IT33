#define FUSE_USE_VERSION 30
#include <fuse3/fuse.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <pthread.h>

#define LOG_FILE "/var/log/it24.log"
static const char *source_path = "/it24_host";

pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void log_message(const char *msg) {
    pthread_mutex_lock(&log_mutex);
    FILE *log = fopen(LOG_FILE, "a");
    if (log) {
        time_t now = time(NULL);
        char *time_str = ctime(&now);
        if (time_str) {
            time_str[strlen(time_str) - 1] = '\0';  
            fprintf(log, "[%s] %s\n", time_str, msg);
        }
        fclose(log);
    }
    pthread_mutex_unlock(&log_mutex);
}

int is_dangerous(const char *name) {
    if (!name) return 0;
    return (strstr(name, "nafis") != NULL) || (strstr(name, "kimcun") != NULL);
}

char *reverse_string(const char *str) {
    if (!str) return NULL;
    int len = strlen(str);
    char *rev = malloc(len + 1);
    if (!rev) return NULL;
    
    for (int i = 0; i < len; i++) {
        rev[i] = str[len - 1 - i];
    }
    rev[len] = '\0';
    return rev;
}

static char *to_real_path(const char *path) {
    static char real_path[1024];

    if (strcmp(path, "/") == 0) {
        snprintf(real_path, sizeof(real_path), "%s/", source_path);
        return real_path;
    }
    
    const char *filename = strrchr(path, '/');
    if (filename) {
        filename++; 
    } else {
        filename = path;
    }
    
    char dir_path[1024] = {0};
    if (filename != path) {
        strncpy(dir_path, path, filename - path);
    }
  
    char *temp = reverse_string(filename);
    if (temp && is_dangerous(temp)) {
        snprintf(real_path, sizeof(real_path), "%s%s%s", source_path, dir_path, temp);
        free(temp);
    } else {
        if (temp) free(temp);
        snprintf(real_path, sizeof(real_path), "%s%s", source_path, path);
    }
    
    return real_path;
}

char *rot13(const char *text) {
    if (!text) return NULL;
    char *result = strdup(text);
    if (!result) return NULL;
    
    for (int i = 0; result[i]; i++) {
        if ((result[i] >= 'a' && result[i] <= 'm') || (result[i] >= 'A' && result[i] <= 'M')) {
            result[i] += 13;
        } else if ((result[i] >= 'n' && result[i] <= 'z') || (result[i] >= 'N' && result[i] <= 'Z')) {
            result[i] -= 13;
        }
    }
    return result;
}

static int antink_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    (void) fi;
    char *fpath = to_real_path(path);
    if (!fpath) return -ENOMEM;
    
    int res = lstat(fpath, stbuf);
    if (res == -1) 
        return -errno;
    
    return 0;
}

static int antink_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi,
                         enum fuse_readdir_flags flags) {
    DIR *dp;
    struct dirent *de;
    char fpath[1024];
    
    (void) offset;
    (void) fi;
    (void) flags;
    
    snprintf(fpath, sizeof(fpath), "%s%s", source_path, path);
    
    dp = opendir(fpath);
    if (dp == NULL)
        return -errno;
    
    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    while ((de = readdir(dp)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
            continue;
            
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        
        if (is_dangerous(de->d_name)) {
            char log_msg[512];
            snprintf(log_msg, sizeof(log_msg), "Dangerous file detected: %s", de->d_name);
            log_message(log_msg);
            
            char *reversed = reverse_string(de->d_name);
            if (reversed) {
                filler(buf, reversed, &st, 0, 0);
                free(reversed);
            }
        } else {
            filler(buf, de->d_name, &st, 0, 0);
        }
    }
    
    closedir(dp);
    return 0;
}

static int antink_open(const char *path, struct fuse_file_info *fi) {
    char *fpath = to_real_path(path);
    if (!fpath) return -ENOMEM;
    
    int fd = open(fpath, fi->flags);
    if (fd == -1)
        return -errno;
    
    fi->fh = fd;
    return 0;
}

static int antink_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi) {
    int res;
    
    (void) fi;
    
    char *fpath = to_real_path(path);
    if (!fpath) return -ENOMEM;
    
    int fd = open(fpath, O_RDONLY);
    if (fd == -1)
        return -errno;
    
    res = pread(fd, buf, size, offset);
    if (res == -1)
        res = -errno;
    
    close(fd);
    
    const char *filename = strrchr(path, '/');
    filename = filename ? filename + 1 : path;

    char log_msg[512];
    snprintf(log_msg, sizeof(log_msg), "Read file: %s", path);
    log_message(log_msg);
    
    if (res > 0) {
        char *temp = reverse_string(filename);
        int is_dangerous_file = temp && is_dangerous(temp);
        if (temp) free(temp);
        
        if (!is_dangerous_file && strstr(filename, ".txt")) {
            char *encrypted = rot13(buf);
            if (encrypted) {
                memcpy(buf, encrypted, res);
                free(encrypted);
            }
        }
    }
    
    return res;
}

static int antink_write(const char *path, const char *buf, size_t size, off_t offset,
                        struct fuse_file_info *fi) {
    int res;
    
    (void) fi;
    
    char *fpath = to_real_path(path);
    if (!fpath) return -ENOMEM;
    
    int fd = open(fpath, O_WRONLY);
    if (fd == -1)
        return -errno;
    
    res = pwrite(fd, buf, size, offset);
    if (res == -1)
        res = -errno;
    
    close(fd);
    
    char log_msg[512];
    snprintf(log_msg, sizeof(log_msg), "Write file: %s", path);
    log_message(log_msg);
    
    return res;
}

static int antink_release(const char *path, struct fuse_file_info *fi) {
    (void) path;
    close(fi->fh);
    return 0;
}

static struct fuse_operations antink_oper = {
    .getattr  = antink_getattr,
    .readdir  = antink_readdir,
    .open     = antink_open,
    .read     = antink_read,
    .write    = antink_write,
    .release  = antink_release,
};

int main(int argc, char *argv[]) {
    FILE *log = fopen(LOG_FILE, "w");
    if (log) {
        fprintf(log, "=== AntiNK System Started ===\n");
        fclose(log);
    }
    
    return fuse_main(argc, argv, &antink_oper, NULL);
}
