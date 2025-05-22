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
#include <glob.h>
#include <ctype.h>

#ifndef PATH_MAX
#define PATH_MAX 4096 
#endif

#ifndef NAME_MAX
#define NAME_MAX 255
#endif

#define SOURCE_DIR "anomali"
#define IMAGE_DIR  "anomali/image"
#define LOG_FILE   "anomali/conversion.log"
#define ZIP_FILE   "anomali.zip"
#define ZIP_URL    "https://drive.google.com/uc?id=1hi_GDdP51Kn2JJMw02WmCOxuc3qrXzh5"

// Mengecek dan menyiapkan direktori dan file awal (download & ekstrak zip jika belum ada)
static void setup_environment(void)
{
    struct stat st = {0};

    if (stat(SOURCE_DIR, &st) == -1) {
        printf("[INFO] Downloading and extracting %s...\n", ZIP_FILE);
        system("wget -q --show-progress -O " ZIP_FILE " \"" ZIP_URL "\"");
        system("unzip -q " ZIP_FILE " -d .");
        unlink(ZIP_FILE);
    }
    if (stat(IMAGE_DIR, &st) == -1) mkdir(IMAGE_DIR, 0755);
}

// Mengubah file .txt berisi data hex menjadi file gambar (.png) biner dan mencatat log-nya
static int convert_file_to_image(const char *txt_name)
{
    //buat path sumber
    char src_path[PATH_MAX];
    snprintf(src_path, sizeof(src_path), "%s/%s", SOURCE_DIR, txt_name);

    FILE *src = fopen(src_path, "r");
    if (!src) {
        fprintf(stderr, "[ERR] Failed to open source file %s\n", src_path);
        return -1;
    }

    fseek(src, 0, SEEK_END);
    long fsize = ftell(src);
    fseek(src, 0, SEEK_SET);

    if (fsize <= 0) {
        fprintf(stderr, "[ERR] Empty file %s\n", txt_name);
        fclose(src);
        return -1;
    }

    char *raw = malloc(fsize + 1);
    if (!raw) {
        fclose(src);
        fprintf(stderr, "[ERR] Memory allocation failed for raw data\n");
        return -1;
    }

    size_t bytes_read = fread(raw, 1, fsize, src);
    fclose(src);
    
    if (bytes_read != fsize) {
        fprintf(stderr, "[ERR] Read incomplete data from %s\n", txt_name);
        free(raw);
        return -1;
    }
    raw[fsize] = '\0';

    // Filter hanya karakter hex
    char *hex = malloc(fsize + 1);
    if (!hex) {
        free(raw);
        fprintf(stderr, "[ERR] Memory allocation failed for hex data\n");
        return -1;
    }

    long hlen = 0;
    for (long i = 0; i < fsize; ++i) {
        if (isxdigit(raw[i])) {
            hex[hlen++] = tolower(raw[i]);
        }
    }
    hex[hlen] = '\0';
    free(raw);

    if (hlen == 0) {
        fprintf(stderr, "[ERR] No hex digits found in %s\n", txt_name);
        free(hex);
        return -1;
    }

    if (hlen % 2 != 0) {
        hex[hlen++] = '0';
        hex[hlen] = '\0';
    }

    long bin_len = hlen / 2;
    unsigned char *bin = malloc(bin_len);
    if (!bin) {
        free(hex);
        fprintf(stderr, "[ERR] Memory allocation failed for binary data\n");
        return -1;
    }

    // konversi ke biner
    for (long i = 0; i < bin_len; i++) {
        char byte_str[3] = {hex[2*i], hex[2*i+1], '\0'};
        bin[i] = (unsigned char)strtol(byte_str, NULL, 16);
    }
    free(hex);

    // timestamp
    time_t now = time(NULL);
    struct tm tm_now;
    localtime_r(&now, &tm_now);

    char ts[32];
    strftime(ts, sizeof(ts), "%Y-%m-%d_%H:%M:%S", &tm_now);

    // nama tanpa ekstensi
    char base[NAME_MAX];
    strncpy(base, txt_name, sizeof(base));
    base[sizeof(base) - 1] = '\0';
    char *dot = strrchr(base, '.');
    if (dot) *dot = '\0';

    // membuat path output
    char out_path[PATH_MAX];
    snprintf(out_path, sizeof(out_path),
             "%s/%s_image_%s.png", IMAGE_DIR, base, ts);

    // menulis gambar
    FILE *img = fopen(out_path, "wb");
    if (!img) {
        free(bin);
        fprintf(stderr, "[ERR] Failed to create image file %s\n", out_path);
        return -1;
    }

    size_t bytes_written = fwrite(bin, 1, bin_len, img);
    fclose(img);
    free(bin);

    if (bytes_written != bin_len) {
        fprintf(stderr, "[ERR] Incomplete write to image file %s\n", out_path);
        unlink(out_path);
        return -1;
    }

    // Log the conversion
    FILE *logf = fopen(LOG_FILE, "a");
    if (logf) {
        fprintf(logf,
            "[%04d-%02d-%02d][%02d:%02d:%02d]: Successfully converted hexadecimal %s to %s\n",
            tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday,
            tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec,
            txt_name, strrchr(out_path, '/') + 1);
        fclose(logf);
    }

    return 0;
}

/* ---------- FUSE callbacks ---------- */

// Mendapatkan atribut file/direktori (ukuran, jenis file, izin akses, dll.)
static int x_getattr(const char *path, struct stat *st)
{
    memset(st, 0, sizeof(struct stat));

    if (!strcmp(path, "/") || !strcmp(path, "/image")) {
        st->st_mode = S_IFDIR | 0755;
        st->st_nlink = 2;
        return 0;
    }

    char real[PATH_MAX];
    if (!strcmp(path, "/conversion.log"))
        snprintf(real, sizeof(real), "%s", LOG_FILE);
    else if (!strncmp(path, "/image/", 7))
        snprintf(real, sizeof(real), "%s%s", IMAGE_DIR, path + 6);
    else
        snprintf(real, sizeof(real), "%s%s", SOURCE_DIR, path);

    return lstat(real, st);
}

// Membaca isi direktori dan menampilkannya ke FUSE (daftar file/direktori)
static int x_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                     off_t off, struct fuse_file_info *fi)
{
    (void)off; (void)fi;
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    if (!strcmp(path, "/")) {
        DIR *dp = opendir(SOURCE_DIR);
        if (!dp) return -errno;
        struct dirent *de;
        while ((de = readdir(dp))) {
            if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")
                || !strcmp(de->d_name, "image")) continue;
            filler(buf, de->d_name, NULL, 0);
        }
        closedir(dp);
        filler(buf, "image", NULL, 0);
        filler(buf, "conversion.log", NULL, 0);

    } else if (!strcmp(path, "/image")) {
        DIR *dp = opendir(IMAGE_DIR);
        if (!dp) return -errno;
        struct dirent *de;
        while ((de = readdir(dp))) {
            if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
            filler(buf, de->d_name, NULL, 0);
        }
        closedir(dp);
    }
    return 0;
}

// Membuka file; jika file .txt di-root dibuka, otomatis dikonversi ke gambar
static int x_open(const char *path, struct fuse_file_info *fi)
{
    /* konversi jika perlu (hanya berkas .txt di SOURCE_DIR) */
    if (strlen(path) > 4 && strcmp(path, "/conversion.log") && 
        strncmp(path, "/image/", 7) && strcmp(path + strlen(path) - 4, ".txt") == 0) {
        
        const char *fname = path + 1; // Skip leading '/'
        fprintf(stderr, "[INFO] Converting %s...\n", fname);
        convert_file_to_image(fname);
    }

    char real[PATH_MAX];
    if (!strcmp(path, "/conversion.log"))
        snprintf(real, sizeof(real), "%s", LOG_FILE);
    else if (!strncmp(path, "/image/", 7))
        snprintf(real, sizeof(real), "%s%s", IMAGE_DIR, path + 6);
    else
        snprintf(real, sizeof(real), "%s%s", SOURCE_DIR, path);

    int fd = open(real, O_RDONLY);
    if (fd == -1) return -errno;
    close(fd);
    return 0;
}

// Membaca isi file sesuai permintaan FUSE (offset dan ukuran tertentu)
static int x_read(const char *path, char *buf, size_t size,
                  off_t offset, struct fuse_file_info *fi)
{
    (void)fi;
    char real[PATH_MAX];
    if (!strcmp(path, "/conversion.log"))
        snprintf(real, sizeof(real), "%s", LOG_FILE);
    else if (!strncmp(path, "/image/", 7))
        snprintf(real, sizeof(real), "%s%s", IMAGE_DIR, path + 6);
    else
        snprintf(real, sizeof(real), "%s%s", SOURCE_DIR, path);

    FILE *f = fopen(real, "r");
    if (!f) return -errno;

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (offset >= len) { fclose(f); return 0; }
    if (offset + size > len) size = len - offset;

    fseek(f, offset, SEEK_SET);
    size_t bytes_read = fread(buf, 1, size, f);
    fclose(f);
    
    return bytes_read;
}

/* ------------------ main ------------------ */

// Struktur callback untuk operasi-operasi FUSE (getattr, readdir, open, read)
static struct fuse_operations ops = {
    .getattr = x_getattr,
    .readdir = x_readdir,
    .open    = x_open,
    .read    = x_read,
};

int main(int argc, char *argv[])
{
    setup_environment();
    return fuse_main(argc, argv, &ops, NULL);
}
