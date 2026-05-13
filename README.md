# 🌿 Smart Greenhouse IoT System (ESP32)

Sistem monitoring dan otomatisasi rumah kaca (Greenhouse) cerdas berbasis **ESP32** yang terintegrasi dengan **Blynk IoT Cloud**, dilengkapi dua sub-sistem independen untuk pendinginan otomatis dan pencahayaan buatan (Grow Light).

## 🚀 Fitur Utama
- **🌐 Dual-Mode Connectivity**: Mendukung mode **Hardware** (dengan WiFiManager dinamis) & mode **Simulation** (dengan DNS IP Bypass untuk Wokwi Web).
- **📊 IoT Cloud Monitoring**: Sinkronisasi data suhu, kelembapan udara, dan tanah secara real-time ke dasbor Blynk.
- **🛡️ Fail-Safe Protection**: Sistem keamanan pompa otomatis (Max Run Time 15s & Cooldown 30s) untuk mencegah banjir dan pompa terbakar.
- **⚖️ Hysteresis Algorithm**: Mencegah jitter relay pada aktuasi pompa air dan kipas.

---

## 📂 Struktur Sub-Sistem
Proyek ini terbagi menjadi dua cabang arsitektur yang bisa Anda pilih sesuai kebutuhan modul hardware Anda:

### 1. 💡 Sistem Lampu Grow Light (LED)
Menggunakan Lampu **HPL Full Spectrum (400-840nm)** untuk stimulasi pertumbuhan vegetatif secara nirkabel.
- **Detail Teknis:** Silakan baca [PENJELASAN_PROYEK_LED.md](./PENJELASAN_PROYEK_LED.md)
- **Aktuasi:** Kontrol 100% Remote Cloud (Blynk Write).
- **Pin Relay:** GPIO 18.

### 2. ❄️ Sistem Pendinginan Kipas (Fan)
Menggunakan Kipas otomatis untuk menurunkan suhu saat termometer mendeteksi panas berlebih.
- **Detail Teknis:** Silakan baca [PENJELASAN_PROYEK_FAN.md](./PENJELASAN_PROYEK_FAN.md)
- **Logika:** Menyala otomatis jika suhu > 30.0°C, dan langsung mati jika <= 30.0°C.
- **Pin Relay:** GPIO 18.

---

## 🛠️ Persiapan & Instalasi

### Prasyarat
1. Akun Blynk Cloud & Aplikasi Blynk IoT di Mobile.
2. **Ekstensi VSCode yang Wajib Diinstal:**
   *   🔌 **`PlatformIO IDE`** (Untuk manajemen pustaka, kompilasi program, dan unggah firmware ke ESP32 fisik).
   *   🎮 **`Wokwi Simulator`** (Untuk menjalankan dan memvisualisasikan ekosistem sirkuit virtual).

### Konfigurasi
Sebelum melakukan upload kode, buka file sumber pada direktori `src/` dan lengkapi baris kredensial teratas dengan data Anda dari Blynk Console:
```cpp
#define BLYNK_TEMPLATE_ID "TEMPEL_DI_SINI"
#define BLYNK_TEMPLATE_NAME "NAMA_DEVICE"
#define BLYNK_AUTH_TOKEN "TOKEN_AUTH_ANDA"
```

### 📤 Perintah Build & Upload (PlatformIO CLI)

Silakan eksekusi environment spesifik di terminal proyek Anda berdasarkan sistem operasi dan perangkat target:

#### 1. Skenario Lampu Grow Light (LED)
*   **🖥️ Simulasi (Wokwi):**
    ```bash
    # Membangun binary simulasi LED saja
    pio run -e simulation
    ```
*   **🔌 Hardware Fisik (Upload ke ESP32):**
    ```bash
    # Mengunggah firmware ke ESP32 asli
    pio run -e hardware -t upload
    ```

#### 2. Skenario Pendinginan Otomatis (Kipas / Fan)
*   **🖥️ Simulasi (Wokwi):**
    ```bash
    # Membangun binary simulasi Kipas saja
    pio run -e simulation_fan
    ```
*   **🔌 Hardware Fisik (Upload ke ESP32):**
    ```bash
    # Mengunggah firmware ke ESP32 asli
    pio run -e hardware_fan -t upload
    ```

> [!TIP]
> **Pengguna macOS/Linux:** Jika perintah `pio` tidak dikenali, Anda bisa menggunakan jalur mutlak berikut:
> *   `~/.platformio/penv/bin/pio run -e hardware -t upload` (Contoh Upload LED)
>
> **Pengguna Windows (PowerShell):** Gunakan:
> *   `& "$env:USERPROFILE\.platformio\penv\Scripts\pio" run -e hardware -t upload`

---

## 🎮 Menjalankan Simulasi (Wokwi)
Repositori ini memuat template konfigurasi mandiri untuk simulasi yang berbeda. Ikuti prosedur standarisasi ini sebelum menyalakan simulator:

### 🔄 Langkah Ganti Profil Simulasi:

> [!IMPORTANT]
> **Cara Mengedit File `diagram.json` di VSCode:** 
> Dikarenakan ekstensi Wokwi secara default akan langsung merender diagram visual, Anda wajib mengklik kanan file `diagram.json` pada Explorer VSCode -> Pilih **"Open With..."** -> Pilih **"Text Editor"** agar Anda dapat mengedit dan menempelkan (paste) kode JSON teks di dalamnya.

#### 🕯️ Cara Masuk ke Skenario LED:
1. **Sirkuit:** Salin seluruh teks dari file `diagram_led.json`, lalu timpa/ganti semua isi file utama `diagram.json`.
2. **Firmware:** Salin seluruh isi file `wokwi_led.toml`, lalu timpa isi file utama `wokwi.toml`.
3. **Kompilasi:** Jalankan `pio run -e simulation`.
4. **Jalankan:** Tekan tombol `F1` -> Klik **`Wokwi: Start Simulator`**.

#### 🌀 Cara Masuk ke Skenario KIPAS:
1. **Sirkuit:** Salin seluruh teks dari file `diagram_fan.json`, lalu timpa/ganti semua isi file utama `diagram.json`.
2. **Firmware:** Salin seluruh isi file `wokwi_fan.toml`, lalu timpa isi file utama `wokwi.toml`.
3. **Kompilasi:** Jalankan `pio run -e simulation_fan`.
4. **Jalankan:** Tekan tombol `F1` -> Klik **`Wokwi: Start Simulator`**.

---

## 🗺️ Pemetaan Pin Hardware Fisik

| Komponen Fisik | Pin ESP32 LED | Pin ESP32 FAN | Keterangan Kelistrikan |
| --- | --- | --- | --- |
| **Sensor Udara DHT22** | `GPIO 4` | `GPIO 16` | Protokol Data Bus Satu Kabel |
| **Kelembapan Tanah** | `GPIO 34` (ADC6) | `GPIO 35` (ADC7) | Input Tegangan Analog |
| **Relay Pompa Air** | `GPIO 5` | `GPIO 5` | Sinyal Kontrol Katup Pompa |
| **Relay Aktuator** | `GPIO 18` (Lampu HPL) | `GPIO 18` (Kipas DC) | Switch Daya Utama Aktuator |
| **Layar LCD 16x2** | `GPIO 21 (SDA) / 22 (SCL)` | `GPIO 21 (SDA) / 22 (SCL)` | Bus Komunikasi I2C Serial |

---
**Dibuat Dengan Dedikasi untuk Pertanian Digital Masa Depan.** ✨

