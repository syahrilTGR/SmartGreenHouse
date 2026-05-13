#define BLYNK_TEMPLATE_ID "YOUR_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "YOUR_TEMPLATE_NAME"
#define BLYNK_AUTH_TOKEN "YOUR_AUTH_TOKEN"

#define BLYNK_PRINT Serial // Aktifkan debugging Blynk ke serial monitor

#include <Arduino.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h> // Library Blynk untuk ESP32
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Definisi Pinout ESP32 (Sama dengan Hardware)
#define DHTPIN 16
#define DHTTYPE DHT22
#define SOIL_MOISTURE_PIN 35
#define RELAY_PUMP_PIN 5
#define RELAY_FAN_PIN 18     // Menggantikan RELAY_LAMP_PIN untuk aktuator Kipas (Fan)
#define BUTTON_FAN_PIN 19    // Menggantikan BUTTON_LAMP_PIN untuk tombol manual Kipas

// Inisialisasi Sensor dan Layar
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2); // Alamat I2C default umumnya 0x27

// Pengaturan Interval Non-blocking
unsigned long previousMillis = 0;
const long interval = 2000; // Siklus baca setiap 2 detik

// State tracker untuk melacak kondisi terakhir aktuator (State Change Detection)
bool lastPumpState = false; // false = mati, true = menyala
bool lastFanState = false;  // false = mati, true = menyala (Menggantikan lastLampState)
bool lastButtonState = HIGH; // Menyimpan status tombol terakhir (INPUT_PULLUP bawaan HIGH)
unsigned long lastToggleTime = 0; // Untuk debouncing tombol manual yang sangat andal

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
  // Catatan: Anda dapat menyesuaikan nilai airValue dan waterValue saat riset nanti.
  int airValue = 4095;   // Nilai analog paling kering
  int waterValue = 1500; // Nilai analog paling basah (dalam air)
  
  int moistureMin = 0;
  int moistureMax = 100;
  
  int calibratedValue = map(sensorReading, airValue, waterValue, moistureMin, moistureMax);
  calibratedValue = constrain(calibratedValue, moistureMin, moistureMax);
  
  return calibratedValue;
}

// ----------------------------------------------------
// CALLBACK BLYNK: Pemantauan Jarak Jauh (Read-Only)
// ----------------------------------------------------

// Konfirmasi koneksi ke Blynk Server
BLYNK_CONNECTED() {
  Serial.println("=> [WOKWI SIM] Blynk Connected! Memulai pemantauan sensor...");
}

void setup() {
  Serial.begin(115200);
  
  // Inisialisasi Relay (Asumsi relay modul berlogika Active High)
  pinMode(RELAY_PUMP_PIN, OUTPUT);
  pinMode(RELAY_FAN_PIN, OUTPUT);
  digitalWrite(RELAY_PUMP_PIN, LOW); 
  digitalWrite(RELAY_FAN_PIN, LOW);

  // Inisialisasi Tombol Kipas Manual
  pinMode(BUTTON_FAN_PIN, INPUT_PULLUP);

  // Inisialisasi LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("SmartGreenHouse");
  lcd.setCursor(0, 1);
  lcd.print("Wokwi Sim Mode");

  lcd.clear();
  lcd.print("Connecting Wi-Fi");
  Serial.println("=> Memulai Koneksi WiFi & Blynk...");

  // Menggunakan Blynk.begin standar dengan IP server blynk.cloud untuk membypass DNS Wokwi Web
  // IP 128.199.144.129 adalah IP blynk.cloud saat ini
  Blynk.begin(BLYNK_AUTH_TOKEN, "Wokwi-GUEST", "", IPAddress(128, 199, 144, 129), 80);

  lcd.clear();
  lcd.print("Blynk Connected!");
  delay(1000);

  // Inisialisasi Sensor DHT
  dht.begin();
  lcd.clear();
}

void loop() {
  // Selalu panggil Blynk.run() sesering mungkin agar komunikasi IoT lancar
  Blynk.run();

  unsigned long currentMillis = millis();

  // ----------------------------------------------------
  // LOGIKA MANUAL: Membaca Tombol Kipas (Diluar non-blocking interval agar responsif)
  // ----------------------------------------------------
  int buttonState = digitalRead(BUTTON_FAN_PIN);
  if (buttonState == LOW && lastButtonState == HIGH && (currentMillis - lastToggleTime > 250)) {
    // Tombol ditekan dengan kunci debouncing 250ms untuk menghindari double-triggering
    lastToggleTime = currentMillis;
    lastFanState = !lastFanState;
    digitalWrite(RELAY_FAN_PIN, lastFanState ? HIGH : LOW);
    Serial.printf("=> Aksi Manual: Kipas %s\r\n", lastFanState ? "Dinyalakan" : "Dimatikan");
    
    // Kirim feedback status terbaru ke Blynk
    Blynk.virtualWrite(V4, lastFanState ? 1 : 0);
  }
  lastButtonState = buttonState;

  // ----------------------------------------------------
  // SISTEM FAIL-SAFE: Proteksi Pompa (Dijalankan real-time setiap loop)
  // ----------------------------------------------------
  if (lastPumpState && (currentMillis - pumpStartTime >= MAX_PUMP_RUN_TIME)) {
    lastPumpState = false;
    pumpSafetyCutoff = true; // Kunci sistem pengaman aktif
    safetyTriggerTime = currentMillis; // Catat waktu mulai istirahat (cooldown)
    digitalWrite(RELAY_PUMP_PIN, LOW);
    Serial.println("=> [WARNING] FAIL-SAFE AKTIF: Pompa mati paksa (mencapai batas 15s). Memulai masa istirahat (cooldown) 30 detik agar air meresap...\r\n");
    
    // Kirim update status ke Blynk
    Blynk.virtualWrite(V3, 0);
  }

  // ----------------------------------------------------
  // AUTO-RESET COOLDOWN: Membuka kunci pengaman setelah masa istirahat selesai
  // ----------------------------------------------------
  if (pumpSafetyCutoff && (currentMillis - safetyTriggerTime >= SAFETY_COOLDOWN)) {
    pumpSafetyCutoff = false; // Reset flag pengaman secara otomatis
    Serial.println("=> Info: Masa istirahat (cooldown) selesai. Mencoba membaca sensor kembali...\r\n");
  }

  // Eksekusi siklus setiap 'interval' ms (Hysteresis & Monitoring)
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // Baca data DHT22
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    // Baca data Kelembapan Tanah (ADC1)
    int moistureRaw = analogRead(SOIL_MOISTURE_PIN);
    int moisturePercent = calibrateMoisture(moistureRaw);

    // Validasi pembacaan DHT
    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("Peringatan: Gagal membaca data dari DHT22!\r\n");
    } else {
      // Tampilkan Log Serial untuk Debugging
      Serial.printf("[SIM] Suhu: %.1fC | Lembap Udara: %.1f%% | Lembap Tanah: %d%% | Kipas: %s\r\n", 
                    temperature, humidity, moisturePercent, lastFanState ? "ON" : "OFF");
      
      // Update Layar LCD 16x2 (Solusi 4: Anti Karakter Acak)
      lcd.init();
      lcd.backlight();
      lcd.setCursor(0, 0);
      lcd.print("T:");
      lcd.print(temperature, 1);
      lcd.print("C S:");
      lcd.print(moisturePercent);
      lcd.print("%");
      
      lcd.setCursor(0, 1);
      lcd.print("H:");
      lcd.print((int)humidity); // Cast ke int agar lebih ringkas dan muat di LCD
      lcd.print("%");
      
      // Tampilkan status Pompa di LCD
      if (pumpSafetyCutoff) {
        lcd.print(" P:WT");
      } else {
        lcd.print(lastPumpState ? " P:ON" : " P:OFF");
      }

      // Tampilkan status Kipas di LCD
      lcd.print(lastFanState ? " F:ON" : " F:OFF");
      
      // Kirim data secara real-time ke Blynk Dashboard
      if (Blynk.connected()) {
        Blynk.virtualWrite(V0, temperature);
        Blynk.virtualWrite(V1, humidity);
        Blynk.virtualWrite(V2, moisturePercent);
        Blynk.virtualWrite(V3, lastPumpState ? 1 : 0);
        Blynk.virtualWrite(V4, lastFanState ? 1 : 0);
      }

      // Calculate remaining cooldown time
      unsigned long cooldownRemaining = 0;
      if (pumpSafetyCutoff) {
        cooldownRemaining = (SAFETY_COOLDOWN - (currentMillis - safetyTriggerTime)) / 1000;
      }

      // ----------------------------------------------------
      // Logika Hysteresis dengan Fitur Fail-safe & Cooldown untuk Pompa Air
      // ----------------------------------------------------
      if (moisturePercent < 40) {
        if (pumpSafetyCutoff) {
          // Pengaman aktif, kunci pompa selama masa cooldown
          Serial.printf("=> Info: Pompa istirahat (cooldown) sisa %ds agar air meresap ke sensor.\r\n", cooldownRemaining);
        } else {
          if (!lastPumpState) {
            lastPumpState = true;
            pumpStartTime = currentMillis; // Catat waktu pompa mulai menyala
            digitalWrite(RELAY_PUMP_PIN, HIGH);
            Serial.println("=> Aksi: Pompa Dinyalakan\r\n");
            
            // Kirim status baru ke Blynk
            Blynk.virtualWrite(V3, 1);
          }
        }
      } 
      else if (moisturePercent > 70) {
        if (lastPumpState) {
          lastPumpState = false;
          digitalWrite(RELAY_PUMP_PIN, LOW);
          Serial.println("=> Aksi: Pompa Dimatikan\r\n");
          
          // Kirim status baru ke Blynk
          Blynk.virtualWrite(V3, 0);
        }
        // Jika kelembapan tanah akhirnya terbaca basah, pastikan kunci pengaman bersih
        if (pumpSafetyCutoff) {
          pumpSafetyCutoff = false;
          Serial.println("=> Info: Kunci Fail-safe dilepas karena sensor kembali normal membaca kondisi basah.\r\n");
        }
      }
      else {
        // Berada di dalam batas toleransi (Hysteresis zone: 40% - 70%)
        if (pumpSafetyCutoff) {
          // Jika tanah mulai lembap (>40%), kita juga bisa membuka kunci pengaman secara otomatis
          pumpSafetyCutoff = false;
          Serial.println("=> Info: Kunci Fail-safe dilepas karena kelembapan tanah mulai naik di atas batas kritis.\r\n");
        }
      }

      // ----------------------------------------------------
      // Logika Hysteresis untuk Kipas (Fan) berdasarkan Suhu DHT22
      // Kipas menyala jika Suhu > 30.0C, dan mati jika Suhu < 27.0C
      // ----------------------------------------------------
      if (temperature > 30.0) {
        if (!lastFanState) {
          lastFanState = true;
          digitalWrite(RELAY_FAN_PIN, HIGH);
          Serial.println("=> Aksi Otomatis: Kipas Dinyalakan (Suhu Panas > 30C)\r\n");
          
          // Kirim status baru ke Blynk
          Blynk.virtualWrite(V4, 1);
        }
      } 
      else {
        if (lastFanState) {
          lastFanState = false;
          digitalWrite(RELAY_FAN_PIN, LOW);
          Serial.println("=> Aksi Otomatis: Kipas Dimatikan (Suhu sudah menyentuh <= 30C)\r\n");
          
          // Kirim status baru ke Blynk
          Blynk.virtualWrite(V4, 0);
        }
      }
    }
  }
}
