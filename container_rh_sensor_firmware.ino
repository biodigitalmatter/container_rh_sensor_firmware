#include <Arduino.h>
#include <M5Stack.h>
#include <M5UnitENV.h>

SCD4X scd4x;

void setup()
{
  M5.begin();
  M5.Power.begin();
  M5.Lcd.fillScreen(WHITE);
  M5.Lcd.setTextWrap(true, true);

  if (!scd4x.begin(&Wire, SCD4X_I2C_ADDR, 32, 33, 400000U))
  {
    M5.Lcd.print("Couldn't find sensor");
    while (1)
      delay(1);
  }

  uint16_t error;
  // stop potentially previously started measurement
  error = scd4x.stopPeriodicMeasurement();
  if (error)
  {
    M5.Lcd.print("Error trying to execute stopPeriodicMeasurement(): ");
  }

  // Start Measurement
  error = scd4x.startPeriodicMeasurement();
  if (error)
  {
    M5.Lcd.print("Error trying to execute startPeriodicMeasurement(): ");
  }

  M5.Lcd.print("Waiting for first measurement... (5 sec)");
}

void loop()
{

  if (scd4x.update()) // readMeasurement will return true when
                      // fresh data is available
  {
    M5.Lcd.clear(WHITE);
    M5.Lcd.setCursor(0,0);

    M5.Lcd.print("CO2(ppm):");
    M5.Lcd.print(scd4x.getCO2());

    M5.Lcd.print("\tTemperature(C):");
    M5.Lcd.print(scd4x.getTemperature(), 1);

    M5.Lcd.print("\tHumidity(%RH):");
    M5.Lcd.print(scd4x.getHumidity(), 1);

    M5.Lcd.println();
  }
  else
  {
    M5.Lcd.print(".");
  }

  delay(1000);
}
