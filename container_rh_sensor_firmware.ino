#include <Arduino.h>
#include <ArduinoJson.h>
#include <M5Stack.h>
#include <M5UnitENV.h>
#include <WiFi.h>
#include "WiFi_secrets.h" // WIFI_SSID & WIFI_PASSWORD

const uint16_t WIFI_CONN_TRIES = 8;
const char[] WIFI_HOSTNAME = "container_rh_sensor";

SCD4X scd4x;
JsonDocument doc;

uint16_t CO2_PPM;
float TEMPERATURE_C;
float HUMIDITY_PERCENT;

void initWiFi(char[] hostname, char[] ssid, char[] pwd, int conn_tries=5) {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(hostname);
  WiFi.begin(ssid, pwd);
  Serial.print("Connecting to WiFi using ");
  Serial.print(conn_tries);
  Serial.println(" tries");
  for (int i = 0; i < conn_tries; i++) {
    Serial.print("Connecting try ");
    Serial.println(i);

    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("Connected with IP: ");
      Serial.println(WiFi.localIP());
      return true;
      }
    delay(1000);
  }
  Serial.println("Failed to connect. Continuing.")
  return false;
}

void displayReading(char[] desc, String val) {
    M5.Lcd.setTextSize(5);
    M5.Lcd.println(desc);
    M5.Lcd.setTextSize(7);
    M5.Lcd.println(val);
}

void setup() {
  Serial.begin(115200);

  doc["sensor"] = "container_rh";

  M5.begin();
  M5.Power.begin();
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(7);

  initWiFi(WIFI_HOSTNAME, WIFI_SSID, WIFI_PASSWORD, WIFI_CONN_TRIES);

  if (!scd4x.begin(&Wire, SCD4X_I2C_ADDR, 21, 22, 400000U)) {
    Serial.println("Couldn't find SCD4X");
    while (1) delay(1);
  }

  uint16_t error;
  // stop potentially previously started measurement
  error = scd4x.stopPeriodicMeasurement();
  if (error) {
    Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
  }

  // Start Measurement
  error = scd4x.startPeriodicMeasurement();
  if (error) {
    Serial.print("Error trying to execute startPeriodicMeasurement(): ");
  }

  Serial.println("Waiting for first measurement... (5 sec)");
}

void loop() {
  if (scd4x.update()) {
    CO2_PPM = scd4x.getCO2();
    TEMPERATURE_C = scd4x.getTemperature();
    HUMIDITY_PERCENT = scd4x.getHumidity();

    doc["co2_ppm"] = CO2_PPM;
    doc["temperature_c"] = TEMPERATURE_C;
    doc["humidity_percent"] = HUMIDITY_PERCENT;

    serializeJson(doc, Serial);

    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.println();

    displayReading("T(C):", String(TEMPERATURE_C));
    displayReading("RH%:", String(HUMIDITY_PERCENT));

    char[] ip;

    if (WiFi.status() != WL_CONNECTED) {
      ip = WiFi.localIP();
    } else {
      ip = "Not connected";
    }

    displayReading("IP:", String(ip));

    M5.Lcd.setCursor(0, 0);

  } else {
    M5.Lcd.setTextSize(5);
    M5.Lcd.print(".");
  }

  delay(1000);
}
