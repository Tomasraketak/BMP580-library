/*
  BMP580 Advanced Example

  This example demonstrates how to read data from the BMP580 sensor
  and how to configure the sensor's settings:

  Oversampling (OSR) options:
    BMP580_OSR_x1, BMP580_OSR_x2, BMP580_OSR_x4, BMP580_OSR_x8,
    BMP580_OSR_x16, BMP580_OSR_x32, BMP580_OSR_x64, BMP580_OSR_x128

  Output Data Rate (ODR) options (Hz):
    BMP580_ODR_240Hz, BMP580_ODR_218p5Hz, BMP580_ODR_199p1Hz, BMP580_ODR_179p2Hz,
    BMP580_ODR_160Hz, BMP580_ODR_149p3Hz, BMP580_ODR_140Hz, BMP580_ODR_129p9Hz,
    BMP580_ODR_120Hz, BMP580_ODR_110p2Hz, BMP580_ODR_100p3Hz, BMP580_ODR_89p6Hz,
    BMP580_ODR_80Hz, BMP580_ODR_70Hz, BMP580_ODR_60Hz, BMP580_ODR_50p1Hz,
    BMP580_ODR_45Hz, BMP580_ODR_40Hz, BMP580_ODR_35Hz, BMP580_ODR_30Hz,
    BMP580_ODR_25Hz, BMP580_ODR_20Hz, BMP580_ODR_15Hz, BMP580_ODR_10Hz,
    BMP580_ODR_5Hz, BMP580_ODR_4Hz, BMP580_ODR_3Hz, BMP580_ODR_2Hz,
    BMP580_ODR_1Hz, BMP580_ODR_0p5Hz, BMP580_ODR_0p25Hz, BMP580_ODR_0p125Hz

  Power modes:
    BMP580_MODE_STANDBY, BMP580_MODE_NORMAL,
    BMP580_MODE_FORCED, BMP580_MODE_CONTINUOUS
*/

#include <Wire.h>
#include "BMP580.h"

BMP580 bmp;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  if (!bmp.begin(0x46)) {
    Serial.println("Error: BMP580 not detected!");
    while (1);
  }

  Serial.println("BMP580 initialized successfully.");

  // Example: set custom oversampling (8x for pressure, 4x for temperature)
  bmp.setOversampling(BMP580_OSR_x8, BMP580_OSR_x4);

  // Example: set Output Data Rate to 100 Hz
  bmp.setODR(BMP580_ODR_100p3Hz);

  // Example: set power mode to CONTINUOUS for constant measurement
  bmp.setPowerMode(BMP580_MODE_CONTINUOUS);

  Serial.println("Custom settings applied:");
  Serial.println("- Oversampling: P=8x, T=4x");
  Serial.println("- Output Data Rate: 100 Hz");
  Serial.println("- Power Mode: CONTINUOUS");
}

void loop() {
  float temperature = bmp.readTemperature();
  float pressure = bmp.readPressure();
  float altitude = bmp.readAltitude(101325.0);

  Serial.print("Temperature: ");
  Serial.print(temperature, 2);
  Serial.println(" °C");

  Serial.print("Pressure: ");
  Serial.print(pressure / 100.0, 2);
  Serial.println(" hPa");

  Serial.print("Altitude: ");
  Serial.print(altitude, 2);
  Serial.println(" m");

  Serial.println("---");
  delay(500);
}
