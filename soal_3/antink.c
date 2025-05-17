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
            time_str[strlen(time_str) - 1] = '\0';  // Hapus newline
            fprintf(log, "[%s] %s\n", time_str, msg);
        }
        fclose(log);
    }
    pthread_mutex_unlock(&log_mutex);
}

int is_dangerous(const char *name) {
    return strstr(name, "nafis") || strstr(name, "kimcun");
}

char *reverse_name(const char *name) {
    int len = strlen(name);
    char *rev = malloc(len + 1);
    if (!rev) return NULL;
    for (int i = 0; i < len; i++) {
        rev[i] = name[len - 1 - i];
    }
    rev[len] = '\0';
    return rev;
}

static char *to_real_path(const char *path) {
    static char fpath[1024];
    const char *filename = strrchr(path, '/');
    filename = filename ? filename + 1 : path;

    if (is_dangerous(filename)) {
        char *rev = reverse_name(filename);
        if (!rev) return NULL;
        snprintf(fpath, sizeof(fpath), "%s/%s", source_path, rev);
        free(rev);
    } else {
        snprintf(fpath, sizeof(fpath), "%s%s", source_path, path);
    }
    return fpath;
}

char *rot13(const char *text) {
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
    return lstat(fpath, stbuf) == -1 ? -errno : 0;
}

static int antink_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                          off_t offset, struct fuse_file_info *fi,
                          enum fuse_readdir_flags flags) {
    char fpath[1024];
    snprintf(fpath, sizeof(fpath), "%s%s", source_path, path);

    DIR *dp = opendir(fpath);
    if (!dp) return -errno;

    struct dirent *de;
    while ((de = readdir(dp)) != NULL) {
        struct stat st = {0};
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;

        char *display_name = is_dangerous(de->d_name) ? reverse_name(de->d_name) : strdup(de->d_name);
        if (!display_name) continue;

        if (is_dangerous(de->d_name)) {
            char log_msg[512];  // buffer diperbesar dari 256 ke 512
            snprintf(log_msg, sizeof(log_msg), "Dangerous file detected: %s", de->d_name);
            log_message(log_msg);
        }

        filler(buf, display_name, &st, 0, 0);
        free(display_name);
    }
    closedir(dp);
    return 0;
}

static int antink_open(const char *path, struct fuse_file_info *fi) {
    char *fpath = to_real_path(path);
    if (!fpath) return -ENOMEM;
    int fd = open(fpath, fi->flags);
    if (fd == -1) return -errno;
    fi->fh = fd;
    return 0;
}

static int antink_read(const char *path, char *buf, size_t size, off_t offset,
                       struct fuse_file_info *fi) {
    int fd = fi->fh;
    int res = pread(fd, buf, size, offset);
    if (res == -1) return -errno;

    const char *filename = strrchr(path, '/');
    filename = filename ? filename + 1 : path;

    if (!is_dangerous(filename) && strstr(filename, ".txt")) {
        char *encrypted = rot13(buf);
        if (encrypted) {
            strncpy(buf, encrypted, res);
            free(encrypted);
        }
    }

    char log_msg[512];  // buffer diperbesar dari 256 ke 512
    snprintf(log_msg, sizeof(log_msg), "Read file: %s", path);
    log_message(log_msg);

    return res;
}

static int antink_release(const char *path, struct fuse_file_info *fi) {
    close(fi->fh);
    return 0;
}

static struct fuse_operations antink_oper = {
    .getattr = antink_getattr,
    .readdir = antink_readdir,
    .open = antink_open,
    .read = antink_read,
    .release = antink_release,
};

int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &antink_oper, NULL);
}
