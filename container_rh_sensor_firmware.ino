#include <Arduino.h>
#include <ArduinoJson.h>
#include <M5Stack.h>
#include <M5UnitENV.h>
#include <WiFi.h>
#include <ArduinoMqttClient.h>
#include "secrets.h"

const uint16_t WIFI_CONN_TRIES = 8;
const char *WIFI_HOSTNAME = "container_co2_sensor";
const char *MQTT_TOPIC = "iot/container/climate";

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

SCD4X scd4x;
JsonDocument doc;

uint16_t co2_ppm;
float temperature_c;
float humidity_percent;

void initWiFi(const char *hostname, const char *ssid, const char *pwd, const uint8_t conn_tries = 5) {
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
      return;
    }
    delay(1000);
  }
  Serial.println("Failed to connect. Continuing.");
}

bool connectMQTT(MqttClient &client, const char *broker_url, const uint16_t broker_port) {
  if (client.connect(broker_url, broker_port)) {
    Serial.println("MQTT connected!");
    return true;
  } else {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(client.connectError());
    return false;
  }
}

void initMQTT(MqttClient &client, const char *broker_url, const uint16_t broker_port, const char *username, const char *password) {
  client.setUsernamePassword(username, password);

  connectMQTT(client, broker_url, broker_port);
}

void displayReading(const char *desc, const String &val) {
  M5.Lcd.setTextSize(5);
  M5.Lcd.println(desc);
  M5.Lcd.setTextSize(7);
  M5.Lcd.println(val);
}

void initSensor() {
  if (!scd4x.begin(&Wire, SCD4X_I2C_ADDR, 21, 22, 400000U)) {
    Serial.println("Couldn't find SCD4X");
    while (1)
      delay(1);
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

  Serial.println("Waiting for first measurement...");
}

void resetLcdScreen() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
}

void initM5() {
  M5.begin();
  M5.Power.begin();
  resetLcdScreen();
}

void setup() {
  Serial.begin(115200);

  doc["sensor"] = "container_rh";

  initM5();
  initSensor();
  initWiFi(WIFI_HOSTNAME, WIFI_SSID, WIFI_PASSWORD, WIFI_CONN_TRIES);

  if (WiFi.status() == WL_CONNECTED) {
    initMQTT(mqttClient, MQTT_BROKER_URL, MQTT_BROKER_PORT, MQTT_BROKER_USERNAME, MQTT_BROKER_PASSWORD);
  }
}

void sendJsonToMQTT(MqttClient &client, const char *topic, JsonDocument &doc) {
  client.beginMessage(topic, (unsigned long)measureJson(doc));
  serializeJson(doc, client);
  client.endMessage();
  Serial.println("Sent MQTT message!");
}

void loop() {
  mqttClient.poll();  // keep the mqtt client alive
  if (scd4x.update()) {
    co2_ppm = scd4x.getCO2();
    temperature_c = scd4x.getTemperature();
    humidity_percent = scd4x.getHumidity();

    doc["co2_ppm"] = co2_ppm;
    doc["temperature_c"] = temperature_c;
    doc["humidity_percent"] = humidity_percent;

    serializeJson(doc, Serial);
    Serial.println();

    resetLcdScreen();

    displayReading("T(C):", String(temperature_c));
    displayReading("RH%:", String(humidity_percent));

    if (WiFi.status() == WL_CONNECTED) {
      displayReading("IP:", WiFi.localIP().toString());
      Serial.println("WiFi (still) connected!");

      if (connectMQTT(mqttClient, MQTT_BROKER_URL, MQTT_BROKER_PORT)) {
        sendJsonToMQTT(mqttClient, MQTT_TOPIC, doc);
      }
    } else {
      displayReading("IP:", "Not connected");
      Serial.println("Not connected.");
    }
  }
}
