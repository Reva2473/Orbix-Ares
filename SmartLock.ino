#define BLYNK_TEMPLATE_ID "TMPL3m9HO0q5x"
#define BLYNK_TEMPLATE_NAME "Smart Lock"
#define BLYNK_AUTH_TOKEN "7YZw5YsQSgXfz-xkMlwe_zuip0y3tGOj"


#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <SPI.h>
#include <MFRC522.h>

char ssid[] = "GUEST-N";
char pass[] = "hello@2026";

// ---------------- DHT SETUP ----------------
#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ---------------- RFID SETUP ----------------
#define SS_PIN 14
#define RST_PIN 33
MFRC522 rfid(SS_PIN, RST_PIN);

// ---------------- PINS ----------------
const int RELAY_PIN = 12;
const int VIBRATION_PIN = 27;

BlynkTimer timer;

// ---------------- BLYNK BUTTON ----------------
BLYNK_WRITE(V0) {
  if (param.asInt() == 1) {
    unlockDoor();
  }
}

// ---------------- UNLOCK FUNCTION ----------------
void unlockDoor() {
  digitalWrite(RELAY_PIN, LOW);  // Unlock (active LOW relay)
  Blynk.virtualWrite(V1, "OPEN");
  Blynk.setProperty(V1, "color", "#D34336");

  timer.setTimeout(5000L, []() {
    digitalWrite(RELAY_PIN, HIGH); // Lock again
    Blynk.virtualWrite(V1, "SECURED");
    Blynk.setProperty(V1, "color", "#23C48E");
    Blynk.virtualWrite(V0, 0);
  });
}

// ---------------- SENSOR CHECK ----------------
void checkSafetySensors() {
  float temperature = dht.readTemperature();

  if (isnan(temperature)) {
    // Serial.println("Failed to read from DHT!");
    return;
  }

  Serial.print("Temp: ");
  Serial.println(temperature);

  Blynk.virtualWrite(V2, temperature);

  // Temperature alert
  if (temperature >= 30) {
    Blynk.logEvent("temp_alert", "Temperature crossed 30°C! Unlocking.");
    unlockDoor();
  }

  // Vibration alert
  if (digitalRead(VIBRATION_PIN) == HIGH) {
    Blynk.logEvent("tamper_alert", "Vibration detected!");
  }

  // Real-time pin status
  Blynk.virtualWrite(V4, digitalRead(RELAY_PIN));
  Blynk.virtualWrite(V5, digitalRead(VIBRATION_PIN));
}

// ---------------- RFID CHECK ----------------
void checkRFID() {
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  String uid = "";
  Serial.print("Card UID: ");

  for (byte i = 0; i < rfid.uid.size; i++) {
    uid += String(rfid.uid.uidByte[i], HEX);
  }

  Serial.println(uid);

  // Send UID to app
  Blynk.virtualWrite(V3, uid);

  // Unlock door
  unlockDoor();

  Blynk.logEvent("rfid_access", "RFID card used to unlock");

  rfid.PICC_HaltA();
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(9600);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(VIBRATION_PIN, INPUT);

  digitalWrite(RELAY_PIN, HIGH); // Start LOCKED

  dht.begin();

  SPI.begin();
  rfid.PCD_Init();
  Serial.println("RFID Ready");

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  timer.setInterval(2000L, checkSafetySensors);
}

// ---------------- LOOP ----------------
void loop() {
  Blynk.run();
  timer.run();
  checkRFID();
}