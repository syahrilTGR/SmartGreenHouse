# LAPORAN & PENJELASAN PROYEK: SMART GREENHOUSE (SISTEM KIPAS & PENDINGIN OTOMATIS)


## 🗺️ 1. DESKRIPSI UMUM PROYEK
Proyek ini merupakan varian lanjutan dari Smart Greenhouse berbasis **ESP32**. Sistem ini mengintegrasikan pemantauan iklim mikro yang komprehensif dengan sistem aktusi pintar berupa:
1. **Sistem Irigasi Otomatis:** Dikontrol oleh sensor kelembapan tanah dengan algoritma hysteresis dan pengaman *fail-safe*.
2. **Sistem Pendinginan Otomatis (Smart Fan):** Mengatur sirkulasi udara berdasarkan suhu aktual dari sensor DHT22 untuk mencegah tanaman layu akibat panas berlebih.
3. **Integrasi Cloud Blynk IoT:** Memungkinkan pemantauan jarak jauh secara *real-time* melalui antarmuka smartphone atau dashboard web.

---

## 🔌 2. DIAGRAM PINOUT PERANGKAT (ESP32)

Berikut adalah konfigurasi pemetaan pin fisik (*hardware pin mapping*) yang digunakan pada berkas `main_fan.cpp` dan `main_sim_fan.cpp`:

| Komponen | Pin ESP32 | Jenis Pin | Deskripsi Fungsional |
| :--- | :--- | :--- | :--- |
| **Sensor DHT22** | `GPIO 16` | Digital Input | Membaca Suhu (°C) & Kelembapan Udara (%) |
| **Soil Moisture Sensor** | `GPIO 35` | Analog Input (ADC1) | Membaca kelembapan tanah (Kalibrasi ADC 12-bit) |
| **Relay Pompa Air** | `GPIO 5` | Digital Output | Mengontrol on/off aliran daya pompa air |
| **Relay Kipas (Fan)** | `GPIO 18` | Digital Output | Mengontrol on/off modul kipas pendingin |
| **LCD 16x2 SDA** | `GPIO 21` | I2C Data | Jalur komunikasi data layar lokal |
| **LCD 16x2 SCL** | `GPIO 22` | I2C Clock | Jalur sinkronisasi detak layar lokal |

---

## 🛠️ 3. FITUR UNGGULAN & LOGIKA PERANGKAT LUNAK

### A. Logika Pendinginan Otomatis Hysteresis (Smart Cooling)
Kipas beroperasi secara adaptif menggunakan ambang batas suhu tunggal:
* **Suhu Aktif (`> 30.0°C`):** Kipas otomatis MENYALA untuk segera mendinginkan greenhouse.
* **Suhu Mati (`<= 30.0°C`):** Kipas otomatis MATI seketika suhu sudah menyentuh batas target (30°C).

### B. Integrasi Dashboard IoT (Blynk Cloud)
Sistem mengirimkan data telemetri setiap interval non-blocking ke platform Blynk IoT menggunakan virtual pin berikut:
* `V0`: **Suhu** udara aktual (°C)
* `V1`: **Kelembapan** udara aktual (%)
* `V2`: **Kelembapan Tanah** terkalibrasi (%)
* `V3`: **Status Pompa** (Indikator Visual 0/1)
* `V4`: **Status Kipas** (Indikator Visual 0/1)

### C. Konektivitas Ganda Hybrid
1. **Hardware Mode (`main_fan.cpp`):** Menggunakan **WiFiManager** untuk koneksi dinamis tanpa kode statis + library Blynk dalam mode *Non-Blocking connection*.
2. **Simulation Mode (`main_sim_fan.cpp`):** Menggunakan teknik **Wokwi IP Bypass** (`128.199.144.129`) untuk menjamin koneksi internet di simulator bebas dari masalah `DNS Failed`.

### D. Sistem Fail-Safe Pompa & Penyiraman Intermiten
Mewarisi fitur keamanan mutakhir:
* **Max Run Time:** Pompa mati paksa setelah menyala **15 detik** berturut-turut tanpa henti.
* **Cooldown Period:** Masa istirahat **30 detik** (menampilkan `P:WT` di LCD) agar air memiliki waktu meresap sempurna menembus lapisan tanah sebelum sensor melakukan pembacaan ulang.

### E. Fitur Pemulihan Otomatis Layar (Anti-Scramble)
Sistem memiliki perlindungan proaktif terhadap gangguan listrik induktif (Noise/EMI) dari pompa & kipas:
* **Auto-Restore:** Memaksa reset Driver I2C (`lcd.init()`) setiap 2 detik untuk otomatis memperbaiki tulisan yang mendadak menjadi karakter acak tanpa perlu me-restart ESP32 secara manual.

---

## 🖥️ 4. STRUKTUR UTAMA KODE PROGRAM

* **`calibrateMoisture()`**: Mengonversi nilai mentah ADC 0-4095 menjadi 0-100% kelembapan.
* **`BLYNK_CONNECTED()`**: Berfungsi untuk melakukan inisialisasi sinkronisasi data saat perangkat tersambung ke cloud.
* **`setup()`**: Mengonfigurasi pin hardware, menginisialisasi layar LCD, menyalakan DHT22, dan menangani protokol jabat tangan (*handshake*) WiFi & Blynk.
* **`loop()`**:
  * Menjalankan `Blynk.run()` sesering mungkin guna pemantauan _Real-Time_.
  * Melakukan pengecekan kondisi sensor (Suhu, Udara, Tanah) setiap **2 detik sekali** secara non-blocking menggunakan fungsi penanda waktu `millis()`.
  * Mengambil keputusan otomatis: Irigasi air (berdasarkan tanah) dan Pendinginan fan (berdasarkan suhu).
