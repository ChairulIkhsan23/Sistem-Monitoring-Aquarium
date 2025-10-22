#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP32Servo.h>

// === Pin Definition ===
#define ONE_WIRE_BUS 4     // DS18B20 Data pin
#define PH_PIN 34          // Potentiometer (simulasi pH)
#define TURBIDITY_PIN 35   // Sensor kejernihan air (LDR AO)
#define SERVO_PIN 15       // Servo Feeder

// === LED Indicators ===
#define LED_PH_ABNORMAL 27   // Merah = pH abnormal
#define LED_PH_NORMAL 26     // Hijau = pH normal
#define LED_TEMP_COLD 33     // Biru = suhu dingin
#define LED_TEMP_HOT 32      // Oranye = suhu panas

// === Object Initialization ===
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
Servo feeder;

// === Variables ===
float phValue = 7.0;
float temperature = 25.0;
float clarity = 0.0;  // kejernihan dalam persen
float lastTemp = 25.0;

// === Thresholds ===
const float PH_LOW = 6.8;
const float PH_HIGH = 7.8;

const float TEMP_LOW = 24.0;
const float TEMP_HIGH = 28.0;

const float CLARITY_LOW = 40.0; // di bawah ini dianggap keruh
const float CLARITY_HIGH = 70.0; // di atas ini dianggap sangat jernih

unsigned long lastFeed = 0;
const unsigned long FEED_INTERVAL = 20000UL; // 20 detik (simulasi feeding)

// ------------------------------------------
// Fungsi pembacaan suhu yang stabil
// ------------------------------------------
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

// ------------------------------------------
// SETUP
// ------------------------------------------
void setup() {
  Serial.begin(115200);
  Serial.println("\n========================================");
  Serial.println("   SISTEM IoT AQUARIUM MONITORING & CONTROL");
  Serial.println("   (pH, Suhu, Kejernihan Air, dan Feeding Otomatis)");
  Serial.println("========================================\n");

  sensors.begin();
  feeder.attach(SERVO_PIN);

  // Inisialisasi pin
  pinMode(LED_PH_NORMAL, OUTPUT);
  pinMode(LED_PH_ABNORMAL, OUTPUT);
  pinMode(LED_TEMP_COLD, OUTPUT);
  pinMode(LED_TEMP_HOT, OUTPUT);
  pinMode(TURBIDITY_PIN, INPUT);

  // Matikan semua LED di awal
  digitalWrite(LED_PH_NORMAL, LOW);
  digitalWrite(LED_PH_ABNORMAL, LOW);
  digitalWrite(LED_TEMP_COLD, LOW);
  digitalWrite(LED_TEMP_HOT, LOW);

  feeder.write(0);
  delay(500);
  Serial.println("✅ Sistem siap dijalankan!\n");
}

// ------------------------------------------
// LOOP
// ------------------------------------------
void loop() {
  // ==== Baca suhu dari DS18B20 ====
  temperature = getStableTemperature();

  // ==== Baca potensiometer sebagai simulasi pH ====
  int rawPh = analogRead(PH_PIN);
  phValue = map(rawPh, 0, 4095, 0, 140) / 10.0;

  // ==== Baca kejernihan air dari LDR (AO) ====
  int rawClarity = analogRead(TURBIDITY_PIN);
  clarity = map(rawClarity, 0, 4095, 0, 100); // ubah ke persen (0–100%)

  // ==== Cetak data di Serial Monitor ====
  Serial.println("========================================");
  Serial.print("📊 Nilai pH Air       : ");
  Serial.println(phValue, 2);
  Serial.print("🌡️  Suhu Air          : ");
  Serial.print(temperature, 2);
  Serial.println(" °C");
  Serial.print("💧 Kejernihan Air     : ");
  Serial.print(clarity, 1);
  Serial.println(" %");

  // ==== Logika kontrol pH ====
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

  // ==== Logika kontrol suhu ====
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

  // ==== Logika kejernihan air ====
  if (clarity < CLARITY_LOW) {
    Serial.println("💧 Status Kejernihan  : KERUH (perlu filtrasi ulang)");
  } else if (clarity > CLARITY_HIGH) {
    Serial.println("💧 Status Kejernihan  : JERNIH (optimal)");
  } else {
    Serial.println("💧 Status Kejernihan  : NORMAL (masih aman)");
  }

  // ==== Feeding Otomatis ====
  if (millis() - lastFeed >= FEED_INTERVAL) {
    lastFeed = millis();
    Serial.println("🐟 Feeding System     : AKTIF - Servo membuka wadah pakan...");
    feeder.write(90);
    delay(1500);
    feeder.write(0);
    Serial.println("✅ Feeding System     : SELESAI - Servo kembali ke posisi awal");
  } else {
    unsigned long sisa = (FEED_INTERVAL - (millis() - lastFeed)) / 1000;
    Serial.print("⏱️  Feeding berikut dalam : ");
    Serial.print(sisa);
    Serial.println(" detik");
  }

  Serial.println("========================================\n");
  delay(1000);
}
