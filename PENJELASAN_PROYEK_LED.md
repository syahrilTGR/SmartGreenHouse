# LAPORAN & PENJELASAN PROYEK: SMART GREENHOUSE (SISTEM LAMPU GROW LIGHT)


## 🗺️ 1. DESKRIPSI UMUM PROYEK
Proyek ini mengimplementasikan sistem **Smart Greenhouse** berbasis mikrokontroler **ESP32** yang dirancang untuk memantau iklim mikro (suhu dan kelembapan udara) serta kelembapan tanah secara real-time. Sistem ini dilengkapi dengan **I2C LCD 16x2** untuk display parameter lokal, serta mekanisme aktuasi berupa:
1. **Sistem Pompa Otomatis (Irigasi Hysteresis):** Menyiram tanaman berdasarkan parameter kelembapan tanah menggunakan sistem pengaman intermiten.
2. **Sistem Lampu Grow Light (Kontrol IoT Cloud Penuh):** Menggunakan **LED 3W HPL Full Spectrum 400nm-840nm** (Ultraviolet Grow Light) yang dikendalikan secara nirkabel dari jarak jauh melalui aplikasi **Blynk IoT** (Desain sirkuit efisien tanpa tombol fisik).
3. **Dukungan Ganda:** Tersedia untuk perangkat keras fisik (via **WiFiManager**) serta skenario pengujian simulator (via **Wokwi**).

---

## 🔌 2. DIAGRAM PINOUT PERANGKAT (ESP32)

Berikut adalah konfigurasi pemetaan pin fisik (*hardware pin mapping*) yang digunakan pada berkas `main_led.cpp`:

| Komponen | Pin ESP32 | Jenis Pin | Deskripsi Fungsional |
| :--- | :--- | :--- | :--- |
| **Sensor DHT22** | `GPIO 4` | Digital Input | Membaca Suhu (°C) & Kelembapan Udara (%) |
| **Soil Moisture Sensor** | `GPIO 34` | Analog Input (ADC1) | Membaca kelembapan tanah (Kalibrasi ADC 12-bit) |
| **Relay Pompa Air** | `GPIO 5` | Digital Output | Mengontrol on/off aliran daya pompa air |
| **Relay Lampu Grow Light** | `GPIO 18` | Digital Output | Mengontrol on/off Lampu 3W HPL Full Spectrum |
| **LCD 16x2 SDA** | `GPIO 21` | I2C Data | Jalur komunikasi data layar |
| **LCD 16x2 SCL** | `GPIO 22` | I2C Clock | Jalur sinkronisasi detak layar |

---

## 🛠️ 3. FITUR UNGGULAN & LOGIKA PERANGKAT LUNAK

Berkas `main_led.cpp` menerapkan standar pemrograman mikrokontroler kelas industri dengan fitur-fitur penting berikut:

### A. Dynamic WiFiManager (Captive Portal)
* **Kelebihan:** Sistem tidak menggunakan *hardcoded* SSID dan Password WiFi di dalam kode. 
* **Cara Kerja:** Saat pertama kali menyala, jika ESP32 tidak menemukan jaringan WiFi yang dikenal, ia akan secara otomatis mengaktifkan mode Access Point sendiri bernama **"GreenHouse-AP"**. Pengguna cukup menghubungkan ponsel ke AP tersebut untuk memasukkan kredensial WiFi baru lewat layar browser (Captive Portal).

### B. Logika Hysteresis Pompa Air (Penyiraman Aman)
Untuk mencegah *relay bouncing* (relay hidup-mati dengan sangat cepat saat nilai sensor berada tepat di batas batas threshold yang merusak kontaktor relay), sistem menerapkan rentang **Hysteresis**:
* **Batas Kering (`< 40%`):** Pompa air akan dinyalakan secara otomatis.
* **Batas Basah (`> 70%`):** Pompa air akan dimatikan secara otomatis.
* **Batas Toleransi (`40% s.d. 70%`):** Pompa mempertahankan status terakhirnya (*state retention*).

### C. Sistem Fail-Safe & Max Run Timeout (Anti-Banjir & Pompa Terbakar)
Jika sensor tanah mengalami kegagalan pembacaan (*sensor delay*) atau tidak mendeteksi air yang telah diguyur:
* **Maksimal Durasi Nyala (`15 Detik`):** Pompa dibatasi menyala terus-menerus maksimal **15 detik (`MAX_PUMP_RUN_TIME = 15000`ms)**.
* Begitu melampaui batas waktu tersebut, pompa akan dimatikan paksa demi keamanan, dan status dikunci ke mode pengaman **`P:SAFE`**. Hal ini mencegah dinamo pompa panas terbakar (*overheating*) serta mencegah kebanjiran air di kebun.

### D. Penyiraman Intermiten / Cooldown Berdenyut (*Pulsed Watering*)
* Ketika pompa dimatikan paksa oleh sistem *fail-safe*, program akan memasuki masa **cooldown (istirahat) selama 30 detik** (`SAFETY_COOLDOWN = 30000`ms).
* Layar LCD akan menampilkan status **`P:WAIT`**. Sisa waktu hitung mundur ditampilkan di Serial Monitor.
* **Fungsi Fisik:** Memberi jeda waktu agar air yang diguyur di permukaan tanah memiliki kesempatan untuk meresap ke bawah hingga menyentuh lempeng sensor kelembapan tanah (*soil moisture*).
* Setelah 30 detik selesai, pengaman otomatis di-reset untuk mendeteksi kondisi tanah terbaru secara berkala.

### E. Dashboard Pemantauan & Kontrol Jarak Jauh (Blynk Cloud)
Sistem telah sepenuhnya terintegrasi ke cloud untuk pemantauan dan aktivasi tanpa kabel fisik:
* **Telemetri Cloud:** Mengirimkan data Suhu (`V0`), Kelembapan Udara (`V1`), dan Tanah (`V2`).
* **Pusat Kontrol Aktuator (`V4`):** Fungsi `BLYNK_WRITE(V4)` memungkinkan pengguna **MENYALAKAN / MEMATIKAN** Lampu HPL secara instan dari aplikasi Blynk di mana saja.
* **Sinkronisasi Status:** Menggunakan `Blynk.syncVirtual(V4)` saat boot agar kondisi lampu lokal di ESP32 langsung menyesuaikan diri dengan status server cloud terkini.

### F. Rata Kiri Serial Monitor (No Staircase Effect)
* Seluruh log pengiriman teks ke Serial Monitor menggunakan transisi baris lengkap **`\r\n`** (Carriage Return + Line Feed). Hal ini menjamin kursor pembacaan terminal selalu kembali ke posisi paling kiri (bebas dari efek tulisan bergeser menyerupai tangga).

### G. Fitur Pemulihan Otomatis Layar (Anti-Scramble)
Sistem memiliki perlindungan proaktif terhadap gangguan listrik induktif (Noise/EMI) dari pompa & peralatan lainnya:
* **Auto-Restore:** Memaksa reset Driver I2C (`lcd.init()`) setiap 2 detik untuk otomatis memperbaiki tulisan yang mendadak menjadi karakter acak tanpa perlu me-restart ESP32 secara manual.

### H. Fitur Pengaman Booting (Blocking Isolation)
Mencegah pompa & lampu menyala liar saat ESP32 sibuk mencari sinyal WiFi:
* **Active-Low Safe Guard:** Inisialisasi awal langsung memaksa status pin bernilai **`HIGH` (MATI)** tepat di awal fungsi `setup()`. Hal ini menjamin aktuator tetap aman dalam kondisi mati meskipun program sedang tertahan (*blocking*) di proses pencarian koneksi `WiFiManager`.

---

## 🖥️ 4. STRUKTUR UTAMA KODE PROGRAM

* **`calibrateMoisture(int sensorReading)`**: Melakukan pemetaan (*mapping*) linear dari nilai mentah ADC 12-bit ESP32 (`0 - 4095`) menjadi persentase kelembapan tanah (`0% - 100%`) berdasarkan nilai referensi kalibrasi kondisi udara (`airValue = 4095`) dan air (`waterValue = 1500`).
* **`setup()`**: Mengatur arah aliran pin hardware, memulai port komunikasi serial `115200 bps`, menginisialisasi layar LCD, meluncurkan WiFiManager untuk konektivitas, serta mengaktifkan sensor DHT22.
* **`loop()`**:
  * Menjalankan engine `Blynk.run()` tanpa jeda agar komunikasi IoT tetap responsif.
  * Menangani pengawasan *fail-safe* durasi aktif pompa air per milidetik.
  * Mengeksekusi pembacaan sensor DHT22, sensor kelembapan tanah, pembaruan display LCD, serta kalkulasi keputusan irigasi dan pelaporan data ke dashboard cloud setiap **2 detik sekali** secara non-blocking.


