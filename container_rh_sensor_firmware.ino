#include <Arduino.h>
#include <ArduinoJson.h>
#include <M5Stack.h>
#include <M5UnitENV.h>

SCD4X scd4x;
JsonDocument doc;

uint16_t CO2_PPM;
float TEMPERATURE_C;
float HUMIDITY_PERCENT;

void setup() {
  doc["sensor"] = "container_rh";
  Serial.begin(115200);

  M5.begin();
  M5.Power.begin();
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(7);

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


    M5.Lcd.setTextSize(5);
    M5.Lcd.println(F("T(C):"));
    M5.Lcd.setTextSize(7);
    M5.Lcd.println(TEMPERATURE_C, 1);

    M5.Lcd.setTextSize(5);
    M5.Lcd.println(F("RH%:"));
    M5.Lcd.setTextSize(7);
    M5.Lcd.println(HUMIDITY_PERCENT, 1);
    M5.Lcd.setCursor(0, 0);

    Serial.println();
  } else {
    M5.Lcd.setTextSize(5);
    M5.Lcd.print(F("."));
  }

  delay(1000);
}
