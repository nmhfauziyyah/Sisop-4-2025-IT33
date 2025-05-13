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

## Soal 4
### Oleh: Revalina Erica Permatasari (5027241007)
##### a. Starter/Beginning Area - Starter Chiho
Sesuai namanya, ini adalah area pertama yang ada di universe maimai. Di sini, file yang masuk akan disimpan secara normal, tetapi ditambahkan file extension .mai di direktori asli. Di folder FUSE sendiri, ekstension .mai tidak ada.
##### Code

##### Output

##### b. World’s End Area - Metropolis Chiho
Area kedua, World’s End Area (yang sebenarnya terdiri dari 2 chiho) utamanya berisi Metropolis Chiho. Sama seperti starter, file yang masuk disimpan dengan normal, tetapi dilakukan perubahan di direktori asli. File di directory asli di shift sesuai lokasinya.
##### Code

##### Output

##### c. World Tree Area - Dragon Chiho
Area ketiga adalah World Tree Area yang berisi Dragon Chiho. File yang disimpan di area ini dienkripsi menggunakan ROT13 di direktori asli.
##### Code

##### Output

##### d. Black Rose Area - Black Rose Chiho
Area keempat adalah Black Rose Area - Black Rose Chiho. Area ini menyimpan data dalam format biner murni, tanpa enkripsi atau encoding tambahan.
##### Code

##### Output

##### e. Tenkai Area - Heaven Chiho
Area kelima adalah Tenkai Area yang berisi Heaven Chiho. File di area ini dilindungi dengan enkripsi AES-256-CBC (gunakan openssl), dengan setiap file memiliki IV yang implementasinya dibebaskan kepada praktikan (terdapat nilai tambahan jika IV tidak static).
##### Code

##### Output

##### f. Youth Area - Skystreet Chiho
Area keenam adalah Youth Area yang berisi Skystreet Chiho. File di area ini dikompresi menggunakan gzip untuk menghemat storage (gunakan zlib).
##### Code

##### Output

##### g. Prism Area - 7sRef Chiho
Area ketujuh, atau terakhir adalah Prism Area yang berisi 7sRef Chiho. Jika kita melihat world map yang ada diawal, terlihat bahwa 7sRef ini merupakan “gateway” ke semua chiho lain. Sehingga, area ini adalah area spesial yang dapat mengakses semua area lain melalui sistem penamaan khusus.
##### Code

##### Output
