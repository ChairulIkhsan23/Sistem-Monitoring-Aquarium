#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP32Servo.h>

// Definisi PIN
#define ONE_WIRE_BUS 4     // DS18B20 
#define PH_PIN 34          // Potentiometer (Simulasi pH)
#define TURBIDITY_PIN 35   // Sensor Kejernihan Air
#define SERVO_PIN 15       // Servo Feeder

// Indikator PIN
#define LED_PH_ABNORMAL 27   // Merah = pH Abnormal
#define LED_PH_NORMAL 26     // Hijau = pH Normal
#define LED_TEMP_COLD 33     // Biru = Suhu Dingin
#define LED_TEMP_HOT 32      // Oranye = Suhu Panas

// Inisialisasi Objek
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
Servo feeder;

// Variabel
float phValue = 7.0;
float temperature = 25.0;
float clarity = 0.0;  // Kejernihan (Presentase)
float lastTemp = 25.0;

// Treshold
const float PH_LOW = 6.8;
const float PH_HIGH = 7.8;

const float TEMP_LOW = 24.0;
const float TEMP_HIGH = 28.0;

const float CLARITY_LOW = 40.0; // Dibawah Nilai ini Keruh
const float CLARITY_HIGH = 70.0; // Diatas Nilai ini Jernih

unsigned long lastFeed = 0;
const unsigned long FEED_INTERVAL = 20000UL; // 20 Detik Waktu Feeding (Simulasi)


// Fungsi Pembacaan Suhu
float getStableTemperature(int sampleCount = 5) {
  float total = 0;
  for (int i = 0; i < sampleCount; i++) {
    sensors.requestTemperatures();
    float t = sensors.getTempCByIndex(0);
    if (t > -50 && t < 125) total += t;
    else total += lastTemp;
    delay(150);
  }
  float avg = total / sampleCount;
  if (abs(avg - lastTemp) > 2.0) avg = (avg + lastTemp) / 2.0;
  lastTemp = avg;
  return avg;
}

// Fungsi Setup
void setup() {
  Serial.begin(115200);
  Serial.println("\n========================================");
  Serial.println("   SISTEM IoT AQUARIUM MONITORING & CONTROL");
  Serial.println("   (pH, Suhu, Kejernihan Air, dan Feeding Otomatis)");
  Serial.println("========================================\n");

  sensors.begin();
  feeder.attach(SERVO_PIN);

  // Inisialisasi PIN
  pinMode(LED_PH_NORMAL, OUTPUT);
  pinMode(LED_PH_ABNORMAL, OUTPUT);
  pinMode(LED_TEMP_COLD, OUTPUT);
  pinMode(LED_TEMP_HOT, OUTPUT);
  pinMode(TURBIDITY_PIN, INPUT);

  // Matikan Semua LED (Default)
  digitalWrite(LED_PH_NORMAL, LOW);
  digitalWrite(LED_PH_ABNORMAL, LOW);
  digitalWrite(LED_TEMP_COLD, LOW);
  digitalWrite(LED_TEMP_HOT, LOW);

  feeder.write(0);
  delay(500);
  Serial.println("✅ Sistem siap dijalankan!\n");
}

// Fungsi Loop
void loop() {
  // Baca Sensoe DS18B20
  temperature = getStableTemperature();

  // Baca Pontentiometer (Simulasi Sensor pH - Probe/Amplifier)
  int rawPh = analogRead(PH_PIN);
  phValue = map(rawPh, 0, 4095, 0, 140) / 10.0;

  // Baca Kejernihan Air LDR (Simulasi Sensor Kejernihan Air - Turbidity)
  int rawClarity = analogRead(TURBIDITY_PIN);

  // Ubah ke Presentase
  clarity = map(rawClarity, 0, 4095, 0, 100);

  // Cetak ke Serial Monitor
  Serial.println("========================================");
  Serial.print("📊 Nilai pH Air       : ");
  Serial.println(phValue, 2);
  Serial.print("🌡️  Suhu Air          : ");
  Serial.print(temperature, 2);
  Serial.println(" °C");
  Serial.print("💧 Kejernihan Air     : ");
  Serial.print(clarity, 1);
  Serial.println(" %");

  // Logika Kontrol pH
  if (phValue < PH_LOW) {
    digitalWrite(LED_PH_ABNORMAL, HIGH);
    digitalWrite(LED_PH_NORMAL, LOW);
    Serial.println("⚠️  Status pH         : ABNORMAL (Terlalu Rendah)");
    Serial.println("🚰 Aksi Sistem        : Pompa pH UP aktif");
  } else if (phValue > PH_HIGH) {
    digitalWrite(LED_PH_ABNORMAL, HIGH);
    digitalWrite(LED_PH_NORMAL, LOW);
    Serial.println("⚠️  Status pH         : ABNORMAL (Terlalu Tinggi)");
    Serial.println("🚰 Aksi Sistem        : Pompa pH DOWN aktif");
  } else {
    digitalWrite(LED_PH_ABNORMAL, LOW);
    digitalWrite(LED_PH_NORMAL, HIGH);
    Serial.println("✅ Status pH          : NORMAL (6.8–7.8)");
  }

  // Logika Kontrol Suhu
  if (temperature < TEMP_LOW) {
    digitalWrite(LED_TEMP_COLD, HIGH);
    digitalWrite(LED_TEMP_HOT, LOW);
    Serial.println("🌡️  Status Suhu       : ABNORMAL (Terlalu Dingin)");
  } else if (temperature > TEMP_HIGH) {
    digitalWrite(LED_TEMP_COLD, LOW);
    digitalWrite(LED_TEMP_HOT, HIGH);
    Serial.println("🌡️  Status Suhu       : ABNORMAL (Terlalu Panas)");
  } else {
    digitalWrite(LED_TEMP_COLD, LOW);
    digitalWrite(LED_TEMP_HOT, LOW);
    Serial.println("🌡️  Status Suhu       : NORMAL (24–28°C)");
  }

  // Logika Kejernihan Air
  if (clarity < CLARITY_LOW) {
    Serial.println("💧 Status Kejernihan  : Keruh (Perlu Filtrasi)");
  } else if (clarity > CLARITY_HIGH) {
    Serial.println("💧 Status Kejernihan  : Jernih (Optimal)");
  } else {
    Serial.println("💧 Status Kejernihan  : Normal (Masih Aman)");
  }

  // Logika Feeding Otomatis
  if (millis() - lastFeed >= FEED_INTERVAL) {
    lastFeed = millis();
    Serial.println("Feeding System     : Aktif - Servo Membuka Wadah Pakan...");
    feeder.write(90);
    delay(1500);
    feeder.write(0);
    Serial.println("✅ Feeding System     : Selesai - Servo Kembali ke Posisi Awal...");
  } else {
    unsigned long sisa = (FEED_INTERVAL - (millis() - lastFeed)) / 1000;
    Serial.print("Feeding berikut dalam : ");
    Serial.print(sisa);
    Serial.println(" detik");
  }

  Serial.println("========================================\n");
  delay(1000);
}
