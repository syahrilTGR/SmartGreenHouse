#define BLYNK_TEMPLATE_ID "YOUR_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "YOUR_DEVICE_NAME"
#define BLYNK_AUTH_TOKEN "YOUR_AUTH_TOKEN"

#define BLYNK_PRINT Serial // Aktifkan debugging Blynk ke serial monitor

#include <Arduino.h>
#include <WiFiManager.h>
#include <BlynkSimpleEsp32.h> // Library Blynk untuk ESP32
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Definisi Pinout ESP32
#define DHTPIN 4
#define DHTTYPE DHT22
#define SOIL_MOISTURE_PIN 34
#define RELAY_PUMP_PIN 5
#define RELAY_LAMP_PIN 18

// Inisialisasi Sensor dan Layar
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2); // Alamat I2C default umumnya 0x27

// Pengaturan Interval Non-blocking
unsigned long previousMillis = 0;
const long interval = 2000; // Siklus baca setiap 2 detik

// State tracker untuk melacak kondisi terakhir aktuator (State Change Detection)
bool lastPumpState = false; // false = mati, true = menyala
bool lastLampState = false; // false = mati, true = menyala

// ----------------------------------------------------
// SISTEM FAIL-SAFE, TIMEOUT, & COOLDOWN (Penyiraman Intermiten)
// ----------------------------------------------------
unsigned long pumpStartTime = 0; // Menyimpan waktu awal pompa menyala
const unsigned long MAX_PUMP_RUN_TIME = 15000; // Batas durasi maksimal pompa menyala terus (15 detik)
bool pumpSafetyCutoff = false; // Flag pengaman aktif jika pompa melebihi batas waktu (terkunci SAFE)

unsigned long safetyTriggerTime = 0; // Menyimpan waktu saat fail-safe aktif
const unsigned long SAFETY_COOLDOWN = 30000; // Durasi istirahat pompa: 30 detik (agar air meresap ke tanah)

// Fungsi Kalibrasi ADC ESP32 (Resolusi 12-bit: 0-4095)
int calibrateMoisture(int sensorReading) {
  int airValue = 4095;   // Nilai analog paling kering
  int waterValue = 1500; // Nilai analog paling basah (dalam air)
  
  int moistureMin = 0;
  int moistureMax = 100;
  
  int calibratedValue = map(sensorReading, airValue, waterValue, moistureMin, moistureMax);
  calibratedValue = constrain(calibratedValue, moistureMin, moistureMax);
  
  return calibratedValue;
}

// ----------------------------------------------------
// CALLBACK BLYNK: Konfigurasi cloud (Two-Way Cloud Control)
// ----------------------------------------------------

// Konfirmasi koneksi ke Blynk Server
BLYNK_CONNECTED() {
  Serial.println("=> [HARDWARE] Blynk Connected! Memulai pemantauan cloud...");
  Blynk.syncVirtual(V4); // Tarik status lampu terakhir dari dashboard Blynk
}

// Fungsi dipanggil otomatis ketika tombol (Datastream V4) di aplikasi Blynk ditekan
BLYNK_WRITE(V4) {
  int pinValue = param.asInt(); // Ambil nilai (0 atau 1) dari aplikasi
  lastLampState = (pinValue == 1); // Perbarui status
  digitalWrite(RELAY_LAMP_PIN, lastLampState ? LOW : HIGH); // Aktifkan Relay
  Serial.printf("=> Aksi Blynk: Lampu HPL %s\r\n", lastLampState ? "Dinyalakan" : "Dimatikan");
}

void setup() {
  Serial.begin(115200);
  
  // Inisialisasi Relay (Modul Fisik Berlogika Active Low: LOW=Menyala, HIGH=Mati)
  pinMode(RELAY_PUMP_PIN, OUTPUT);
  pinMode(RELAY_LAMP_PIN, OUTPUT);
  // Wajib set HIGH di awal agar Pompa & Lampu TETAP MATI selama proses blocking autoConnect WiFiManager
  digitalWrite(RELAY_PUMP_PIN, HIGH); 
  digitalWrite(RELAY_LAMP_PIN, HIGH);

  // Inisialisasi LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("SmartGreenHouse");
  lcd.setCursor(0, 1);
  lcd.print("Connecting WiFi");

  // Inisialisasi WiFiManager (Captive Portal dinamis)
  WiFiManager wm;
  // wm.resetSettings(); // Uncomment jika ingin melupakan WiFi yang tersimpan
  bool res = wm.autoConnect("GreenHouse-AP"); 
  if(!res) {
    Serial.println("Gagal menyambung ke Wi-Fi\r\n");
  } else {
    Serial.println("Wi-Fi Berhasil Tersambung!\r\n");
    lcd.clear();
    lcd.print("WiFi Connected!");
    delay(2000);
  }

  // Inisialisasi Blynk secara Non-Blocking (Menunggu koneksi Wi-Fi matang)
  Blynk.config(BLYNK_AUTH_TOKEN);
  Blynk.connect();

  // Inisialisasi Sensor DHT
  dht.begin();
  lcd.clear();
}

void loop() {
  // Jalankan engine Blynk terus-menerus untuk pemantauan realtime
  Blynk.run();

  unsigned long currentMillis = millis();

  // ----------------------------------------------------
  // SISTEM FAIL-SAFE: Proteksi Pompa (Dijalankan real-time setiap loop)
  // ----------------------------------------------------
  if (lastPumpState && (currentMillis - pumpStartTime >= MAX_PUMP_RUN_TIME)) {
    lastPumpState = false;
    pumpSafetyCutoff = true;
    safetyTriggerTime = currentMillis;
    digitalWrite(RELAY_PUMP_PIN, LOW);
    Serial.println("=> [WARNING] FAIL-SAFE: Pompa mati paksa (batas 15s). Masa istirahat 30 detik...\r\n");
    
    // Informasikan ke Blynk
    Blynk.virtualWrite(V3, 0);
  }

  // ----------------------------------------------------
  // AUTO-RESET COOLDOWN
  // ----------------------------------------------------
  if (pumpSafetyCutoff && (currentMillis - safetyTriggerTime >= SAFETY_COOLDOWN)) {
    pumpSafetyCutoff = false; 
    Serial.println("=> Info: Masa istirahat selesai. Mencoba baca ulang sensor...\r\n");
  }

  // Siklus pembacaan berkala
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    int moistureRaw = analogRead(SOIL_MOISTURE_PIN);
    int moisturePercent = calibrateMoisture(moistureRaw);

    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("Gagal membaca DHT22!\r\n");
    } else {
      Serial.printf("Suhu: %.1fC | Lembap: %.1f%% | Tanah: %d%%\r\n", 
                    temperature, humidity, moisturePercent);
      
      // Display LCD Lokal (Solusi 4: Reset Init berkala untuk sembuhkan Karakter Acak akibat Noise)
      lcd.init();
      lcd.backlight();
      lcd.setCursor(0,0);
      lcd.print("T:"); lcd.print(temperature, 1);
      lcd.print("C S:"); lcd.print(moisturePercent); lcd.print("%");
      
      lcd.setCursor(0,1);
      lcd.print("H:"); lcd.print((int)humidity); lcd.print("% ");
      
      if (pumpSafetyCutoff) {
        lcd.print("P:WT ");
      } else {
        lcd.print(lastPumpState ? "P:ON " : "P:OFF ");
      }
      lcd.print(lastLampState ? "L:ON" : "L:OFF");
      
      // Kirim Telemetri ke Dashboard Blynk
      if (Blynk.connected()) {
        Blynk.virtualWrite(V0, temperature);
        Blynk.virtualWrite(V1, humidity);
        Blynk.virtualWrite(V2, moisturePercent);
        Blynk.virtualWrite(V3, lastPumpState ? 1 : 0);
        Blynk.virtualWrite(V4, lastLampState ? 1 : 0);
      }

      // Hitung mundur keamanan
      unsigned long cooldownRemaining = 0;
      if (pumpSafetyCutoff) {
        cooldownRemaining = (SAFETY_COOLDOWN - (currentMillis - safetyTriggerTime)) / 1000;
      }

      // LOGIKA HYSTERESIS POMPA
      if (moisturePercent < 60) {
        if (pumpSafetyCutoff) {
          Serial.printf("=> Cooldown sisa %ds...\r\n", cooldownRemaining);
        } else {
          if (!lastPumpState) {
            lastPumpState = true;
            pumpStartTime = currentMillis;
            digitalWrite(RELAY_PUMP_PIN, LOW);
            Serial.println("=> Aksi: Pompa Dinyalakan\r\n");
            Blynk.virtualWrite(V3, 1);
          }
        }
      } 
      else if (moisturePercent > 80) {
        if (lastPumpState) {
          lastPumpState = false;
          digitalWrite(RELAY_PUMP_PIN, HIGH);
          Serial.println("=> Aksi: Pompa Dimatikan\r\n");
          Blynk.virtualWrite(V3, 0);
        }
        if (pumpSafetyCutoff) {
          pumpSafetyCutoff = false; // Reset cepat jika sensor kembali basah
        }
      }
      else {
        if (pumpSafetyCutoff) {
          pumpSafetyCutoff = false; // Toleransi dilepas jika kelembapan membaik
        }
      }
    }
  }
}