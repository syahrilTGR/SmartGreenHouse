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
1. PlatformIO Core / IDE
2. Akun Blynk Cloud & Mobile App

### Konfigurasi
Sebelum melakukan upload kode, buka file sumber pada direktori `src/` dan lengkapi baris kredensial teratas dengan data Anda dari Blynk Console:
```cpp
#define BLYNK_TEMPLATE_ID "TEMPEL_DI_SINI"
#define BLYNK_TEMPLATE_NAME "NAMA_DEVICE"
#define BLYNK_AUTH_TOKEN "TOKEN_AUTH_ANDA"
```

### Build & Upload Command
Eksekusi environment spesifik menggunakan PlatformIO CLI:

```bash
# Upload firmware Kipas ke Hardware Fisik
pio run -e hardware_fan -t upload

# Bangun binary simulasi (sesuaikan target di platformio.ini default_envs)
pio run -e simulation
```

## 🎮 Menjalankan Simulasi (Wokwi)
Repositori ini memuat template terpisah untuk skenario simulasi yang berbeda. Ikuti langkah ini sebelum menekan **Play** di simulator Wokwi VSCode Anda:

### Pilih Profil Skenario
1. **Pilih Mode:** Tentukan apakah Anda ingin mensimulasikan LED atau Kipas (Fan).
2. **Ubah Diagram:** Salin isi file `diagram_led.json` **ATAU** `diagram_fan.json`, lalu paste/ganti seluruh isi file pusat `diagram.json`.
3. **Ubah Konfigurasi:** Salin isi file `wokwi_led.toml` **ATAU** `wokwi_fan.toml`, lalu paste/ganti isi file pusat `wokwi.toml`.
4. **Jalankan Simulator:** Tekan tombol `F1` di VSCode, cari **Wokwi: Start Simulator**!

## 🗺️ Pin Mapping Fisik
| Komponen | Pin ESP32 | Keterangan |
| --- | --- | --- |
| Sensor DHT22 | `GPIO 4` (LED) / `16` (Fan) | Suhu & Udara |
| Soil Moisture | `GPIO 34` (LED) / `35` (Fan) | ADC Kelembapan Tanah |
| Relay Pompa | GPIO 5 | Aktuator Air |
| Relay Aktuator | GPIO 18 | Lampu / Kipas (Tergantung Mode) |
| LCD SDA/SCL | GPIO 21 / 22 | Komunikasi Layar 16x2 |

---
**Dibuat Dengan Dedikasi untuk Pertanian Digital Masa Depan.** ✨
