/*
  BMP580 Forced Mode Example - Voltino Labs Edition

  FORCED mode performs exactly one measurement on request and then returns
  to STANDBY (~1 uA). This is ideal for battery powered loggers that only
  need a sample every few seconds.

  In FORCED mode, readTemperature()/readPressure() automatically trigger a new
  measurement when the cached value is older than one conversion time, so
  calling both in succession still costs only ONE measurement.
*/

#include <Wire.h>
#include "BMP580.h"

BMP580 bmp;

void setup() {
  Serial.begin(115200);

  if (!bmp.begin()) {
    Serial.println("Error: BMP580 not detected!");
    while (1);
  }

  // Highest resolution: OSR_P x128 / OSR_T x8 (0.08 Pa RMS noise)
  bmp.setOversampling(BMP580_OSR_x128, BMP580_OSR_x8);
  bmp.setPowerMode(BMP580_MODE_FORCED);

  Serial.print("One measurement takes ~");
  Serial.print(bmp.getMeasurementTimeUs() / 1000.0f, 1);
  Serial.println(" ms");
}

void loop() {
  float temperature, pressure;

  // Explicit trigger + read in one call
  if (bmp.takeForcedMeasurement() && bmp.read(temperature, pressure)) {
    Serial.print("T = ");
    Serial.print(temperature, 2);
    Serial.print(" degC | p = ");
    Serial.print(pressure / 100.0f, 3);
    Serial.println(" hPa");
  } else {
    Serial.println("Measurement failed!");
  }

  delay(2000);  // the sensor sleeps in STANDBY in between
}
