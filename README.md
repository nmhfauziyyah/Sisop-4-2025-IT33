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
### Oleh: Ni'mah Fauziyyah Atok (5027241103)

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
