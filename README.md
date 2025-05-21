# Sisop-4-2025-IT33

#### Nama Anggota
1. Revalina Erica Permatasari (5027241007)
2. Kaisar Hanif Pratama (5027241029)
3. Ni'mah Fauziyyah Atok (5027241103)

## Daftar Isi
1. [Soal 1](#soal-1)
2. [Soal 2](#soal-2)
3. [Soal3](#soal-3)
4. [Soal 4](#soal-4)

## Soal 1
### Oleh: Ni'mah Fauziyyah A (5027241103)

### Soal
Shorekeeper, penjaga Black Shores, menemukan anomali misterius berupa **file teks berisi string hexadecimal**. <br>
Tugasnya adalah membantu Shorekeeper memecahkan misteri ini dengan:

- Mengunduh dan mengekstrak file sampel dari internet
- Mengonversi string hexadecimal menjadi **file gambar** `.png`
- Menyimpan gambar ke folder khusus
- Mencatat semua proses ke dalam **log file** <br>

Konversi dilakukan secara otomatis saat file teks diakses melalui filesystem virtual (menggunakan FUSE). <br>
Berikut ini adalah rincian solusi berdasarkan soal poin **a sampai d**.

### Jawaban

#### A. Download dan Unzip File
Sampe anomali teks terdapat pada [LINK BERIKUT] (https://drive.google.com/file/d/1hi_GDdP51Kn2JJMw02WmCOxuc3qrXzh5/view)
##### Kode
```
static void setup_environment(void)
{
    struct stat st = {0};

    if (stat(SOURCE_DIR, &st) == -1) {
        printf("[INFO] Downloading and extracting %s…\n", ZIP_FILE);
        system("wget -q --show-progress -O " ZIP_FILE " \"" ZIP_URL "\"");
        system("unzip -q " ZIP_FILE " -d .");
        unlink(ZIP_FILE);
    }

    if (stat(IMAGE_DIR, &st) == -1) {
        mkdir(IMAGE_DIR, 0755);
    }
}
```
##### Penjelasan
- Mengecek keberadaan folder `anomali` dan `anomali/image` dengan `stat()`.
- Jika belum ada: <br>
    - Mengunduh file dari Google Drive via `wget`
    - Mengekstrak menggunakan `unzip`
    - Menghapus file .zip dengan `unlink()`
- Folder `image` dibuat jika belum tersedia agar bisa menyimpan file gambar hasil konversi.

#### B. Konversi String Hexadecimal ke Gambar
Setiap file teks berisi data hexadecimal harus: <br>
1. Dibuka dan dibaca
2. Difilter agar hanya menyisakan karakter hex
3. Dikonversi ke byte biner
4. Disimpan sebagai file `.png` di `anomali/image/`

##### Kode
Fungsi `convert_file_to_image()` menangani proses ini: <br>
```
static int convert_file_to_image(const char *txt_name)
{
    char src_path[PATH_MAX];
    snprintf(src_path, sizeof(src_path), "%s/%s", SOURCE_DIR, txt_name);

    FILE *src = fopen(src_path, "r");
    if (!src) return -1;

    fseek(src, 0, SEEK_END);
    long fsize = ftell(src);
    fseek(src, 0, SEEK_SET);
    char *raw = malloc(fsize + 1);
    fread(raw, 1, fsize, src);
    raw[fsize] = '\0';
    fclose(src);

    char *hex = malloc(fsize + 1);
    long hlen = 0;
    for (long i = 0; i < fsize; ++i) {
        if (isxdigit((unsigned char)raw[i])) {
            hex[hlen++] = raw[i];
        }
    }
    hex[hlen] = '\0';
    free(raw);

    if (hlen % 2 != 0) {
        fprintf(stderr, "[WARN] Odd number of hex digits in %s, skipping.\n", txt_name);
        free(hex);
        return -1;
    }

    unsigned char *bin = malloc(hlen / 2);
    long real_len = 0;
    for (long i = 0; i < hlen / 2; ++i) {
        if (sscanf(hex + 2 * i, "%2hhx", &bin[real_len]) == 1) {
            real_len++;
        }
    }
    free(hex);
```
##### Penjelasan
- File `.txt` dibaca seluruhnya, hasilnya disimpan ke buffer `raw`
- Karakter non-hex diabaikan, hanya `0-9, a-f, A-F` yang disimpan di `hex`
- Validasi: jumlah karakter hex harus genap untuk membentuk byte
- Hexadecimal diubah menjadi byte `(unsigned char)` menggunakan `sscanf()`
- Data biner ini kemudian akan ditulis sebagai file gambar

#### C. Format Penamaan File Output
File gambar hasil konversi harus dinamai sebagai berikut: <br>
```
[nama file string]_image_[YYYY-mm-dd]_[HH:MM:SS].
```

##### Kode
Kode Penamaan File:
```
time_t now = time(NULL);
struct tm tm_now;
localtime_r(&now, &tm_now);

char ts[32];
strftime(ts, sizeof(ts), "%Y-%m-%d_%H:%M:%S", &tm_now);

char base[256];
strncpy(base, txt_name, sizeof(base));
char *dot = strrchr(base, '.');
if (dot) *dot = '\0';

char out_path[PATH_MAX];
snprintf(out_path, sizeof(out_path),
         "%s/%s_image_%s.png", IMAGE_DIR, base, ts);
```
##### Penjelasan
- `time()` mendapatkan waktu saat ini
- `strftime()` mengubah waktu menjadi string dengan format tanggal dan jam
- Nama dasar file diambil tanpa ekstensi `.txt`
- Hasil akhir disusun menjadi nama file gambar seperti `2_image_2025-05-19_20:15:43.png`

#### D. Logging Hasil Konversi
Semua konversi sukses dicatat dalam `conversion.log` dengan format:
```
[YYYY-mm-dd][HH:MM:SS]: Successfully converted hexadecimal text [input.txt] to [output.png]
```

##### Kode
```
FILE *logf = fopen(LOG_FILE, "a");
if (logf) {
    fprintf(logf,
        "[%04d-%02d-%02d][%02d:%02d:%02d]: Successfully converted hexadecimal text %s to %s.\n",
        tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday,
        tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec,
        txt_name, strrchr(out_path, '/') + 1);
    fclose(logf);
}
```
##### Penjelasan
- Log dibuka dalam mode append (`a`)
- Informasi timestamp dan nama file asal serta file hasil dicatat ke file log
- Penggunaan `strrchr(out_path, '/') + 1` digunakan untuk mengambil nama file output saja (tanpa path)

#### E. Kesimpulan
Program ini berhasil memenuhi seluruh poin:
- (a) Download & unzip otomatis
- (b) Parsing string hexadecimal ke gambar
- (c) Penamaan file sesuai waktu konversi
- (d) Logging hasil konversi secara real-time

Semuanya dilakukan secara otomatis saat file teks dibuka melalui sistem file virtual (FUSE). Program ini efektif untuk mendeteksi dan memulihkan data gambar tersembunyi dari teks hexadecimal.

---
### Kendala
Selama proses saya mengerjakan nomor 1, saya mengalami kendala saat mencoba mendekripsi isi file teks menjadi gambar. Masalah utamanya adalah:

#### Masalah
1. Konversi berulang <br>
Beberapa file terkonversi lebih dari satu kali saat diakses, meskipun gambar sudah pernah dibuat sebelumnya. Hal ini terjadi karena:
    - Fungsi `convert_file_to_image()` selalu membuat file baru dengan timestamp (_image_<timestamp>.png).
    - Sementara itu, pengecekan apakah file sudah pernah dikonversi hanya melihat keberadaan file dengan nama tetap (`*_image.png`) yang sebenarnya tidak pernah dibuat.
2. Proses Baca yang Lambat atau tidak muncul
Beberapa file seperti 3.txt dan 6.txt membutuhkan waktu lama untuk ditampilkan menggunakan cat, bahkan hanya muncul setelah dihentikan dengan Ctrl+C. Ini disebabkan oleh pemanggilan fungsi konversi yang berulang kali terjadi di dalam fungsi read(), yang membuat proses macet atau berat.
3. Duplikasi Log
Untuk beberapa file, meskipun gambar hanya dibuat satu kali, catatan log (`conversion.log`) menunjukkan bahwa proses konversi tercatat dua kali. Ini terjadi karena fungsi `convert_file_to_image()` kemungkinan dipanggil lebih dari sekali dalam waktu singkat.

#### Solusi & Revisi 
Untuk mengatasi masalah-masalah tersebut, dilakukan beberapa perbaikan berikut:
1. Perbaikan Mekanisme Pengecekan File Gambar
    - Mengecek keberadaan satu file (`*_image.png`), sistem kini menggunakan pencarian terhadap file dengan pola prefix (`*_image_`) untuk memastikan apakah konversi sudah pernah dilakukan.
    - Hal ini mencegah proses konversi berulang karena nama file hasil konversi menggunakan timestamp.

2. Optimasi Fungsi `xmp_read()`
    - Membuat logika pengecekan yang lebih ketat sebelum memanggil `convert_file_to_image()`.
    - Mngurangi terjadinya proses konversi ganda selama proses `read()` dilakukan secara bertahap oleh aplikasi seperti `cat` atau `less.`

3. Pencegahan Duplikasi Log
Konversi hanya dilakukan jika memang belum ada file hasil konversi sebelumnya, sehingga log hanya mencatat proses konversi yang benar-benar terjadi.

---

## Soal 2
### Oleh: Kaisar Hanif Pratama (5027241029)

## Soal 3
### Oleh: Revalina Erica Permatasari (5027241007)
#### a.Pujo harus membuat sistem AntiNK menggunakan Docker yang menjalankan FUSE dalam container terisolasi. Sistem ini menggunakan docker-compose untuk mengelola container antink-server (FUSE Func.) dan antink-logger (Monitoring Real-Time Log). Asisten juga memberitahu bahwa docker-compose juga memiliki beberapa komponen lain yaitu
it24_host (Bind Mount -> Store Original File)

antink_mount (Mount Point)

antink-logs (Bind Mount -> Store Log)
##### Code

##### Output

#### b. Sistem harus mendeteksi file dengan kata kunci "nafis" atau "kimcun" dan membalikkan nama file tersebut saat ditampilkan. Saat file berbahaya (kimcun atau nafis) terdeteksi, sistem akan mencatat peringatan ke dalam log.
Ex: "docker exec [container-name] ls /antink_mount" 

Output: 
test.txt  vsc.sifan  txt.nucmik
##### Code

##### Output

#### c. Dikarenakan dua anomali tersebut terkenal dengan kelicikannya, Pujo mempunyai ide bahwa isi dari file teks normal akan di enkripsi menggunakan ROT13 saat dibaca, sedangkan file teks berbahaya tidak di enkripsi. 
Ex: "docker exec [container-name] cat /antink_mount/test.txt" 

Output: 
enkripsi teks asli
##### Code

##### Output

#### d. Semua aktivitas dicatat dengan ke dalam log file /var/log/it24.log yang dimonitor secara real-time oleh container logger.
##### Code

##### Output

#### e. Semua perubahan file hanya terjadi di dalam container server jadi tidak akan berpengaruh di dalam direktori host. 
##### Code

##### Output
