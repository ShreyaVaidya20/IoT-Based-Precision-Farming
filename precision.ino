#include <SoftwareSerial.h>
#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include <Arduino.h>
#include <DHT.h>
#include <WiFi.h>
#include <Adafruit_Sensor.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

const int SoilSensorPin = 25;
const int wet = 4095;
const int dry = 1500;

const int RE = 12;
const int DE = 14;

const int DHT_SENSOR_PIN = 2;
#define DHT_SENSOR_TYPE DHT11

DHT dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);

const byte nitro[] = {0x01,0x03, 0x00, 0x1e, 0x00, 0x01, 0xe4, 0x0c};
const byte phos[] = {0x01,0x03, 0x00, 0x1f, 0x00, 0x01, 0xb5, 0xcc};
const byte pota[] = {0x01,0x03, 0x00, 0x20, 0x00, 0x01, 0x85, 0xc0};

byte values[11];


#define WIFI_SSID "YOUR_WIFI_NAME"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define API_KEY "YOUR_FIREBASE_API_KEY"
#define DATABASE_URL "YOUR_FIREBASE_DATABASE_URL"
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
SoftwareSerial mod(13, 27);

bool signupOK = false;
unsigned long sendDataPrevMillis = 0;
const unsigned long UPLOAD_INTERVAL = 1000;

void setup() {
  Serial.begin(115200);
  dht_sensor.begin();
  mod.begin(9600);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  pinMode(RE, OUTPUT);
  pinMode(DE, OUTPUT);

  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("ok");
    signupOK = true;
  } else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  float temperature = dht_sensor.readTemperature();
  float humidity = dht_sensor.readHumidity();

 int SoilSensorVal = analogRead(SoilSensorPin); // Read analog value from the top layer moisture sensor
  
  // Map the sensor values to percentage values
  int SoilMoisture = map(SoilSensorVal, wet, dry, 0, 100);
  SoilMoisture = constrain(SoilMoisture, 0, 100);

  byte val1 = nitrogen();
  delay(250);
  byte val2 = phosphorous();
  delay(250);
  byte val3 = potassium();
  delay(250);

  if (Firebase.ready() && signupOK &&
    (millis() - sendDataPrevMillis > UPLOAD_INTERVAL || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    if (Firebase.RTDB.setFloat(&fbdo, "DHT_11/Temperature", temperature)) {
      Serial.print("Temperature: ");
      Serial.println(temperature);
    } else {
      Serial.println("Failed to write temperature to Firebase");
      Serial.println("Reason: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setFloat(&fbdo, "DHT_11/Humidity", humidity)) {
      Serial.print("Humidity: ");
      Serial.println(humidity);
    } else {
      Serial.println("Failed to write humidity to Firebase");
      Serial.println("Reason: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setInt(&fbdo, "Soil Moisture", SoilMoisture)) {
      Serial.print("Soil Moisture: ");
Serial.print(SoilMoisture);
Serial.println("%");
    } else {
      Serial.println("Failed to write soil moisture to Firebase");
      Serial.println("Reason: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setInt(&fbdo, "Nitrogen", val1)) {
      Serial.print("Nitrogen: ");
      Serial.println(val1);
    } else {
      Serial.println("Failed to write nitrogen value to Firebase");
      Serial.println("Reason: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setInt(&fbdo, "Phosphorus", val2)) {
      Serial.print("Phosphorus: ");
      Serial.println(val2);
    } else {
      Serial.println("Failed to write phosphorus value to Firebase");
      Serial.println("Reason: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setInt(&fbdo, "Potassium", val3)) {
      Serial.print("Potassium: ");
      Serial.println(val3);
    } else {
      Serial.println("Failed to write potassium value to Firebase");
      Serial.println("Reason: " + fbdo.errorReason());
    }
  }
  delay(2000);
}

byte nitrogen() {
  return readSensorData(nitro);
}

byte phosphorous() {
  return readSensorData(phos);
}

byte potassium() {
  return readSensorData(pota);
}

byte readSensorData(const byte* command) {
  digitalWrite(DE, HIGH);
  digitalWrite(RE, HIGH);
  delay(10);
  if (mod.write(command, 8) == 8) {
    digitalWrite(DE, LOW);
    digitalWrite(RE, LOW);
    delay(10); // Add a small delay to allow data to be received
    for (byte i = 0; i < 7; i++) {
      values[i] = mod.read();
    }
    return values[4];
  }
  return 0;
}