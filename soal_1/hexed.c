#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <time.h>

#define SOURCE_DIR "anomali"
#define IMAGE_DIR "anomali/image"
#define LOG_FILE  "anomali/conversion.log"
#define ZIP_FILE "anomali.zip"
#define ZIP_URL "https://drive.google.com/uc?id=1hi_GDdP51Kn2JJMw02WmCOxuc3qrXzh5"

void setup_environment() {
    struct stat st = {0};

    if (stat(SOURCE_DIR, &st) == -1) {
        printf("[INFO] Downloading and extracting %s...\n", ZIP_FILE);
        system("wget -q --show-progress -O anomali.zip \"" ZIP_URL "\"");
        system("unzip -q anomali.zip -d .");
        system("rm anomali.zip");
    }

    if (stat(IMAGE_DIR, &st) == -1) {
        mkdir(IMAGE_DIR, 0755);
    }
}

void convert_file_to_image(const char *filename) {
    char src_path[512];
    snprintf(src_path, sizeof(src_path), "%s/%s", SOURCE_DIR, filename);

    FILE *src = fopen(src_path, "r");
    if (!src) return;

    fseek(src, 0, SEEK_END);
    long len = ftell(src);
    fseek(src, 0, SEEK_SET);

    char *hex = malloc(len + 1);
    fread(hex, 1, len, src);
    hex[len] = '\0';
    fclose(src);

    int bin_len = len / 2;
    unsigned char *bin = malloc(bin_len);
    for (int i = 0; i < bin_len; i++) {
        sscanf(hex + 2 * i, "%2hhx", &bin[i]);
    }

    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d_%H:%M:%S", tm_now);

    char name[256];
    strncpy(name, filename, sizeof(name));
    name[sizeof(name) - 1] = '\0';
    char *dot = strrchr(name, '.');
    if (dot) *dot = '\0';

    char check_path[512];
    snprintf(check_path, sizeof(check_path), "%s/%s_image.png", IMAGE_DIR, name);
    if (access(check_path, F_OK) == 0) {
        return; // Gambar sudah ada, tidak perlu konversi ulang
    }

    char img_name[512];
    snprintf(img_name, sizeof(img_name), "%s/%s_image_%s.png", IMAGE_DIR, name, timestamp);

    FILE *img = fopen(img_name, "wb");
    if (img) {
        fwrite(bin, 1, bin_len, img);
        fclose(img);
    }

    FILE *log = fopen(LOG_FILE, "a");
    if (log) {
        fprintf(log, "[%04d-%02d-%02d][%02d:%02d:%02d]: Successfully converted hexadecimal text %s to %s.\n",
            tm_now->tm_year + 1900, tm_now->tm_mon + 1, tm_now->tm_mday,
            tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec,
            filename, strrchr(img_name, '/') + 1);
        fclose(log);
    }

    free(hex);
    free(bin);
}

static int xmp_getattr(const char *path, struct stat *stbuf) {
    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0 || strcmp(path, "/image") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    char real_path[512];
    if (strcmp(path, "/conversion.log") == 0)
        snprintf(real_path, sizeof(real_path), "%s", LOG_FILE);
    else if (strncmp(path, "/image/", 7) == 0)
        snprintf(real_path, sizeof(real_path), "%s%s", IMAGE_DIR, path + 6);
    else
        snprintf(real_path, sizeof(real_path), "%s%s", SOURCE_DIR, path);

    return lstat(real_path, stbuf);
}

static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi) {
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    if (strcmp(path, "/") == 0) {
        DIR *dp = opendir(SOURCE_DIR);
        struct dirent *de;

        while ((de = readdir(dp)) != NULL) {
            if (strcmp(de->d_name, "image") == 0 || strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
                continue;
            filler(buf, de->d_name, NULL, 0);
        }
        closedir(dp);
        filler(buf, "image", NULL, 0);
        filler(buf, "conversion.log", NULL, 0);

    } else if (strcmp(path, "/image") == 0) {
        DIR *dp = opendir(IMAGE_DIR);
        struct dirent *de;

        if (dp == NULL) return -errno;

        while ((de = readdir(dp)) != NULL) {
            if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
                continue;
            filler(buf, de->d_name, NULL, 0);
        }
        closedir(dp);
    }

    return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi) {
    char real_path[512];

    if (strcmp(path, "/conversion.log") == 0)
        snprintf(real_path, sizeof(real_path), "%s", LOG_FILE);
    else if (strncmp(path, "/image/", 7) == 0)
        snprintf(real_path, sizeof(real_path), "%s%s", IMAGE_DIR, path + 6);
    else {
        snprintf(real_path, sizeof(real_path), "%s%s", SOURCE_DIR, path);
    }

    int fd = open(real_path, O_RDONLY);
    if (fd == -1) return -errno;
    close(fd);
    return 0;
}

static int xmp_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    char real_path[512];
    if (strcmp(path, "/conversion.log") == 0)
        snprintf(real_path, sizeof(real_path), "%s", LOG_FILE);
    else if (strncmp(path, "/image/", 7) == 0)
        snprintf(real_path, sizeof(real_path), "%s%s", IMAGE_DIR, path + 6);
    else {
    snprintf(real_path, sizeof(real_path), "%s%s", SOURCE_DIR, path);
    
    const char *name = strrchr(path, '/');
    if (name) name++;
    else name = path;

    // Cek dulu apakah gambar sudah ada sebelum konversi
    char name_no_ext[256];
    strncpy(name_no_ext, name, sizeof(name_no_ext));
    name_no_ext[sizeof(name_no_ext) - 1] = '\0';
    char *dot = strrchr(name_no_ext, '.');
    if (dot) *dot = '\0';

    char img_path[512];
    snprintf(img_path, sizeof(img_path), "%s/%s_image.png", IMAGE_DIR, name_no_ext);

    if (access(img_path, F_OK) != 0) {
        convert_file_to_image(name);
    }
}

    FILE *f = fopen(real_path, "r");
    if (!f) return -errno;

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *temp = malloc(len);
    fread(temp, 1, len, f);
    fclose(f);

    if (offset < len) {
        if (offset + size > len) size = len - offset;
        memcpy(buf, temp + offset, size);
    } else {
        size = 0;
    }

    free(temp);
    return size;
}

static struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .readdir = xmp_readdir,
    .open = xmp_open,
    .read = xmp_read,
};

int main(int argc, char *argv[]) {
    setup_environment();
    return fuse_main(argc, argv, &xmp_oper, NULL);
}
