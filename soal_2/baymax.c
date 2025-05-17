#define FUSE_USE_VERSION 31
#include <fuse3/fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>

#define CHUNK_SIZE 1024
#define RELICS_DIR "./relics"
#define LOG_FILE "./activity.log"

void log_activity(const char *activity, const char *detail) {
    FILE *log = fopen(LOG_FILE, "a");
    if (!log) return;
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char buf[64];
    strftime(buf, sizeof(buf), "[%Y-%m-%d %H:%M:%S]", t);
    fprintf(log, "%s %s: %s\n", buf, activity, detail);
    fclose(log);
}

static int vfs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    (void) fi;
    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    char name[256];
    snprintf(name, sizeof(name), "%s", path + 1);  

    char chunk_path[512];
    snprintf(chunk_path, sizeof(chunk_path), "%s/%s.000", RELICS_DIR, name);

    FILE *chunk = fopen(chunk_path, "rb");
    if (!chunk)
        return -ENOENT;

    stbuf->st_mode = S_IFREG | 0644;
    stbuf->st_nlink = 1;

    
    size_t total_size = 0;
    for (int i = 0;; ++i) {
        snprintf(chunk_path, sizeof(chunk_path), "%s/%s.%03d", RELICS_DIR, name, i);
        chunk = fopen(chunk_path, "rb");
        if (!chunk) break;
        fseek(chunk, 0, SEEK_END);
        total_size += ftell(chunk);
        fclose(chunk);
    }
    stbuf->st_size = total_size;
    return 0;
}

static int vfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
                       struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
    (void) offset;
    (void) fi;
    (void) flags;

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    DIR *dp = opendir(RELICS_DIR);
    if (!dp) return -ENOENT;

    struct dirent *de;
    char shown[256][256];
    int shown_count = 0;

    while ((de = readdir(dp)) != NULL) {
        char *dot = strrchr(de->d_name, '.');
        if (dot && strlen(dot) == 4) {
            char base[256];
            strncpy(base, de->d_name, dot - de->d_name);
            base[dot - de->d_name] = '\0';

            int already = 0;
            for (int i = 0; i < shown_count; i++) {
                if (strcmp(shown[i], base) == 0) {
                    already = 1;
                    break;
                }
            }
            if (!already) {
                strcpy(shown[shown_count++], base);
                filler(buf, base, NULL, 0, 0);
            }
        }
    }
    closedir(dp);
    return 0;
}

static int vfs_open(const char *path, struct fuse_file_info *fi) {
    char name[256];
    snprintf(name, sizeof(name), "%s", path + 1);

    char chunk_path[512];
    snprintf(chunk_path, sizeof(chunk_path), "%s/%s.000", RELICS_DIR, name);
    FILE *f = fopen(chunk_path, "rb");
    if (!f) return -ENOENT;
    fclose(f);

    log_activity("READ", name);
    return 0;
}

static int vfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi;
    char name[256];
    snprintf(name, sizeof(name), "%s", path + 1);

    size_t read = 0, pos = 0;
    for (int i = 0; read < size; i++) {
        char chunk_path[512];
        snprintf(chunk_path, sizeof(chunk_path), "%s/%s.%03d", RELICS_DIR, name, i);

        FILE *chunk = fopen(chunk_path, "rb");
        if (!chunk) break;

        fseek(chunk, 0, SEEK_END);
        long chunk_size = ftell(chunk);
        fseek(chunk, 0, SEEK_SET);

        if (offset < pos + chunk_size) {
            size_t start = offset > pos ? offset - pos : 0;
            fseek(chunk, start, SEEK_SET);
            size_t to_read = chunk_size - start;
            if (to_read > size - read) to_read = size - read;
            fread(buf + read, 1, to_read, chunk);
            read += to_read;
        }

        pos += chunk_size;
        fclose(chunk);
    }

    return read;
}

static int vfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    (void) mode;
    (void) fi;
    return 0;
}

static int vfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) offset;
    char name[256];
    snprintf(name, sizeof(name), "%s", path + 1);

    int chunk_count = 0;
    for (size_t i = 0; i < size; i += CHUNK_SIZE, chunk_count++) {
        char chunk_path[512];
        snprintf(chunk_path, sizeof(chunk_path), "%s/%s.%03d", RELICS_DIR, name, chunk_count);
        FILE *f = fopen(chunk_path, "wb");
        if (!f) return -EIO;
        size_t to_write = CHUNK_SIZE < size - i ? CHUNK_SIZE : size - i;
        fwrite(buf + i, 1, to_write, f);
        fclose(f);
    }

    char log_msg[512];
    snprintf(log_msg, sizeof(log_msg), "%s -> %s.000 - %s.%03d", name, name, name, chunk_count - 1);
    log_activity("WRITE", log_msg);

    return size;
}

static int vfs_unlink(const char *path) {
    char name[256];
    snprintf(name, sizeof(name), "%s", path + 1);

    int count = 0;
    char chunk_path[512];
    for (int i = 0;; i++) {
        snprintf(chunk_path, sizeof(chunk_path), "%s/%s.%03d", RELICS_DIR, name, i);
        if (remove(chunk_path) != 0) break;
        count++;
    }

    if (count == 0) return -ENOENT;

    char log_msg[512];
    snprintf(log_msg, sizeof(log_msg), "%s.000 - %s.%03d", name, name, count - 1);
    log_activity("DELETE", log_msg);
    return 0;
}

static const struct fuse_operations vfs_oper = {
    .getattr = vfs_getattr,
    .readdir = vfs_readdir,
    .open    = vfs_open,
    .read    = vfs_read,
    .create  = vfs_create,
    .write   = vfs_write,
    .unlink  = vfs_unlink,
};

int main(int argc, char *argv[]) {
    umask(0);
    return fuse_main(argc, argv, &vfs_oper, NULL);
}

