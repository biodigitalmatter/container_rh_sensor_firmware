#include <Arduino.h>
#include <ArduinoJson.h>
#include <ArduinoMqttClient.h>
#include <M5Stack.h>
#include <M5UnitENV.h>
#include <WiFi.h>
#include "secrets.h"

#define MQTT_REQUIRED  // Retry wifi and mqtt connection until connected

struct SensorData {
  uint16_t co2_ppm;
  float temperature_c;
  float humidity_percent;
};

const char *DEVICE_NAME = "m5_co2_1";
const uint8_t WIFI_CONN_TRIES = 3;

const String MQTT_TOPIC_ROOT = "iot/device/m5co2";
const String MQTT_TOPIC_CONF = MQTT_TOPIC_ROOT + "/config";
const String MQTT_TOPIC_AVAIL = MQTT_TOPIC_ROOT + "/available";
const String MQTT_TOPIC_STATE = MQTT_TOPIC_ROOT + "/state";

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

SCD4X scd4x;
JsonDocument doc;

void initWiFi(const char *hostname, const char *ssid, const char *pwd,
              const uint8_t conn_tries = 5) {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(hostname);
  WiFi.begin(ssid, pwd);
  Serial.print("Connecting to WiFi using ");
  Serial.print(conn_tries);
  Serial.println(" tries");

  uint8_t i = 0;

#ifdef MQTT_REQUIRED
  while (true) {
#else
  for (i = 0; i < conn_tries; i++) {
#endif
    Serial.print("Connecting try ");
    Serial.println(i + 1);

    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("Connected with IP: ");
      Serial.println(WiFi.localIP());
      return;
    }
    delay(1000);
  }
  Serial.println("Failed to connect. Continuing.");
}

void sendAvailabilityToMQTT() {

  const char *MQTT_AVAILABLE = "online";
  const char *MQTT_NOT_AVAILABLE = "offline";

  mqttClient.beginWill(MQTT_TOPIC_AVAIL, strlen(MQTT_NOT_AVAILABLE), true, 1);
  mqttClient.print(MQTT_NOT_AVAILABLE);
  mqttClient.endWill();

  mqttClient.beginMessage(MQTT_TOPIC_AVAIL, strlen(MQTT_AVAILABLE), true, 1);
  mqttClient.print(MQTT_AVAILABLE);
  mqttClient.endMessage();
}

void sendJsonToMQTT(const String topic, JsonDocument &doc) {
  mqttClient.beginMessage(topic, (unsigned long)measureJson(doc));
  serializeJson(doc, mqttClient);
  mqttClient.endMessage();
  Serial.println("Sent following payload to: " + topic);
  serializeJsonPretty(doc, Serial);
}
void sendConfigToMQTT() {
  JsonDocument doc;

  doc["dev"]["ids"] = "m5co2_1";
  doc["dev"]["name"] = "Container M5 climate sensor";
  doc["o"]["name"] = "m5co2";
  doc["o"]["url"] = "https://github.com/biodigitalmatter/container_rh_sensor_firmware";
  doc["state_topic"] = MQTT_TOPIC_STATE;
  doc["avty_t"] = MQTT_TOPIC_AVAIL;

  doc["cmps"]["CO2"]["p"] = "sensor";
  doc["cmps"]["CO2"]["device_class"] = "carbon_dioxide";
  doc["cmps"]["CO2"]["unit_of_meas"] = "ppm";
  doc["cmps"]["CO2"]["value_template"] = "{{ value_json.co2_ppm }}";
  doc["cmps"]["CO2"]["uniq_id"] = "m5co2c_c";

  doc["cmps"]["temperature"]["uniq_id"] = "m5co2c_t";
  doc["cmps"]["temperature"]["p"] = "sensor";
  doc["cmps"]["temperature"]["device_class"] = "temperature";
  doc["cmps"]["temperature"]["unit_of_meas"] = "°C";
  doc["cmps"]["temperature"]["value_template"] = "{{ value_json.temperature_c }}";
  doc["cmps"]["temperature"]["uniq_id"] = "m5co2c_t";

  doc["cmps"]["humidity"]["p"] = "sensor";
  doc["cmps"]["humidity"]["device_cla"] = "humidity";
  doc["cmps"]["humidity"]["unit_of_meas"] = "%";
  doc["cmps"]["humidity"]["value_template"] = "{{ value_json.humidity_percent }}";
  doc["cmps"]["humidity"]["uniq_id"] = "m5co2c_h";

  sendJsonToMQTT(MQTT_TOPIC_CONF, doc);
}

bool connectMQTT(const char *broker_url, const uint16_t broker_port,
                 const uint8_t conn_tries = 5) {
  Serial.print("Connecting to MQTT broker using ");
#ifdef MQTT_REQUIRED
  Serial.println("infinite tries");
#else
  Serial.print(conn_tries);
  Serial.println(" tries");
#endif

  uint8_t i = 0;

#ifdef MQTT_REQUIRED
  while (true) {
#else
  for (i = 0; i < conn_tries; i++) {
#endif
    Serial.print("Connecting try ");
    Serial.println(i + 1);

    if (mqttClient.connect(broker_url, broker_port)) {
      sendAvailabilityToMQTT();
      sendConfigToMQTT();
      Serial.println("MQTT connected!");
      return true;
    } else {
      Serial.print("MQTT connection failed! Error code = ");
      Serial.println(mqttClient.connectError());
    }
    delay(1000);
  }
  Serial.println("Failed to connect to MQTT broker. Continuing.");
  return false;
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

  doc["sensor"] = DEVICE_NAME;

  initM5();
  initSensor();
  initWiFi(DEVICE_NAME, WIFI_SSID, WIFI_PASSWORD, WIFI_CONN_TRIES);

  mqttClient.setUsernamePassword(MQTT_BROKER_USERNAME, MQTT_BROKER_PASSWORD);
  mqttClient.setId(DEVICE_NAME);

  if (WiFi.status() == WL_CONNECTED) {
    connectMQTT(MQTT_BROKER_URL, MQTT_BROKER_PORT);
  }
}

void displayReading(const char *desc, const String &val) {
  M5.Lcd.setTextSize(5);
  M5.Lcd.print(desc);
  M5.Lcd.print(": ");
  M5.Lcd.println(val);
}

void plotReadings(const SensorData &data) {
  Serial.print("co2_ppm:");
  Serial.print(data.co2_ppm);
  Serial.print(",");
  Serial.print("temperature_c:");
  Serial.print(data.temperature_c);
  Serial.print(",");
  Serial.print("humidity_percent:");
  Serial.print(data.humidity_percent);
  Serial.println();
}

void sendReadingsToMQTT(const SensorData &data) {
  JsonDocument doc;

  doc["co2_ppm"] = data.co2_ppm;
  doc["temperature_c"] = data.temperature_c;
  doc["humidity_percent"] = data.humidity_percent;

  sendJsonToMQTT(MQTT_TOPIC_STATE, doc);
};

void loop() {
  mqttClient.poll();  // keep the mqtt client alive
  if (scd4x.update()) {
    SensorData data;
    data.co2_ppm = scd4x.getCO2();
    data.temperature_c = scd4x.getTemperature();
    data.humidity_percent = scd4x.getHumidity();

    plotReadings(data);

    resetLcdScreen();

    displayReading("T(C)", String(data.temperature_c));
    displayReading("RH%", String(data.humidity_percent));

    if (WiFi.status() == WL_CONNECTED) {
      displayReading("IP", WiFi.localIP().toString());
      Serial.println("WiFi (still) connected!");
      sendReadingsToMQTT(data);
    } else {
      displayReading("IP", "Not connected");
      Serial.println("Not connected.");
    }
  }
}
