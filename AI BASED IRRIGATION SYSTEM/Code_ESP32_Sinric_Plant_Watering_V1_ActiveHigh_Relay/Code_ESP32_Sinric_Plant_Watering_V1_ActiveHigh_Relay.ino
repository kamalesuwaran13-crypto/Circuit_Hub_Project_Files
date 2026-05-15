/**********************************************************************************
 *  TITLE: Plant Watering system using ESP32 Sinric Pro, Moisture Sensor (For Active-HIGH Relay module)
 *  Click on the following links to learn more. 
 *  YouTube Video: https://youtu.be/MmbmNIKxfEI
 *  Related Blog : https://iotcircuithub.com/esp32-projects/
 *  
 *  This code is provided free for project purpose and fair use only.
 *  Please do mail us to techstudycell@gmail.com if you want to use it commercially.
 *  Copyrighted © by Tech StudyCell
 *  
 *  Preferences--> Aditional boards Manager URLs : 
 *  https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_dev_index.json, http://arduino.esp8266.com/stable/package_esp8266com_index.json
 *  
 *  Download Board ESP32 (2.0.3) : https://github.com/espressif/arduino-esp32
 *
 *  Download the libraries 
 *  SinricPro Library (3.5.2): https://github.com/sinricpro/esp8266-esp32-sdk/
 *  ArduinoJson by Benoit Blanchon (minimum Version 7.0.3)
 *  WebSockets by Markus Sattler (minimum Version 2.4.0)
 **********************************************************************************/

#include <Arduino.h>
#include <WiFi.h>
#include <SinricPro.h>
#include <SinricProSwitch.h>
#include "CapacitiveSoilMoistureSensor.h"

// ---- Credentials ----
#define APP_KEY    "YOUR-APP-KEY"
#define APP_SECRET "YOUR-APP-SECRET"

#define SOIL_DEVICE_ID  "YOUR-SOIL-SENSOR-DEVICE-ID"   // Capacitive Soil Moisture Sensor
#define PUMP_DEVICE_ID  "YOUR-PUMP-SWITCH-DEVICE-ID"   // Switch (for pump)

#define SSID       "YOUR_WIFI"
#define PASS       "YOUR_PASSWORD"

// ---- Hardware Pins ----
const int RELAY_PIN = 25;   // Relay for pump (active HIGH)
const int SOIL_PIN  = 34;   // Soil sensor ADC pin

// ---- Calibration values (adjust for your sensor) ----
const int VERY_DRY  = 2910;
const int NEITHER_DRY_OR_WET = 2098;
const int VERY_WET  = 925;
const int DRY_PUSH_NOTIFICATION_THRESHHOLD = 2850;
const int UNPLUGGED = 3000;

// ---- Globals ----
int lastSoilMoisture = 0;
String lastSoilState = "";

CapacitiveSoilMoistureSensor &soilSensor = SinricPro[SOIL_DEVICE_ID];
SinricProSwitch &pumpSwitch = SinricPro[PUMP_DEVICE_ID];

// ---- Pump Control ----
bool onPowerState(const String& deviceId, bool &state) {
  if (deviceId == PUMP_DEVICE_ID) {
    digitalWrite(RELAY_PIN, state ? HIGH : LOW); // active HIGH relay
    Serial.printf("Pump %s\r\n", state ? "ON" : "OFF");
  }
  return true;
}

// ---- Soil Sensor Handler ----
void handleSoilMoisture() {
  if (!SinricPro.isConnected()) return;

  static unsigned long lastMillis = 0;
  if (millis() - lastMillis < 60000) return;  // every 60 sec 
  lastMillis = millis();

  int rawValue = analogRead(SOIL_PIN);
  int percentage = map(rawValue, VERY_WET, VERY_DRY, 100, 0);
  percentage = constrain(percentage, 1, 100);

  Serial.printf("Soil ADC: %d | Moisture: %d%%\r\n", rawValue, percentage);

  if (rawValue == lastSoilMoisture) {
    Serial.println("No change in soil moisture, skipping update...");
    return;
  }

  // Update Mode: Wet / Dry
  String soilState = (rawValue > NEITHER_DRY_OR_WET) ? "Dry" : "Wet";
  if (soilState != lastSoilState) {
    soilSensor.sendModeEvent("modeInstance1", soilState, "PHYSICAL_INTERACTION");
    lastSoilState = soilState;
  }

  // Update Range: % value
  soilSensor.sendRangeValueEvent("rangeInstance1", percentage);

  // Push Notifications
  if (rawValue > DRY_PUSH_NOTIFICATION_THRESHHOLD) {
    soilSensor.sendPushNotification("Plants are too dry. Please water them!");
  }
  if (rawValue > UNPLUGGED) {
    soilSensor.sendPushNotification("Soil sensor may be unplugged!");
  }

  lastSoilMoisture = rawValue;
}

// ---- Setup WiFi ----
void setupWiFi() {
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  WiFi.begin(SSID, PASS);
  Serial.printf("[WiFi]: Connecting to %s", SSID);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println(" connected!");
}

// ---- Setup Sinric Pro ----
void setupSinricPro() {
  pumpSwitch.onPowerState(onPowerState);

  SinricPro.onConnected([] { Serial.println("[SinricPro]: Connected"); });
  SinricPro.onDisconnected([] { Serial.println("[SinricPro]: Disconnected"); });
  SinricPro.begin(APP_KEY, APP_SECRET);
}

// ---- Arduino Setup ----
void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Pump OFF at start
  pinMode(SOIL_PIN, INPUT);

  setupWiFi();
  setupSinricPro();
}

// ---- Arduino Loop ----
void loop() {
  SinricPro.handle();
  handleSoilMoisture();
}
