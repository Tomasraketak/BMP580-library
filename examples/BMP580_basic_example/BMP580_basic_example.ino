/*
  BMP580 Basic Example

  This example shows how to use the BMP580 sensor with default settings.
  It reads temperature, pressure, and calculates altitude from the sensor.
  Default settings:
    - Oversampling: 4x
    - Output Data Rate (ODR): 240 Hz
    - Power Mode: NORMAL

  Values are printed to the Serial Monitor for observation.
*/

#include <Wire.h>
#include "BMP580.h"  // Include our BMP580 library

BMP580 bmp;  // Create a BMP580 object

void setup() {
  Serial.begin(115200);  // Initialize serial communication
  Wire.begin();          // Initialize I2C bus

  // Initialize BMP580 with default I2C address (0x46)
  if (!bmp.begin(0x46)) {
    Serial.println("Error: BMP580 not detected!");
    while (1);  // Stop execution if sensor is not found
  }

  Serial.println("BMP580 initialized with default settings (no modifications).");
  delay(100);  // Short delay for sensor stabilization
}

void loop() {
  // Read sensor values
  float temperature = bmp.readTemperature();       // °C
  float pressure = bmp.readPressure();             // Pa
  float altitude = bmp.readAltitude(101325.0);     // meters (sea-level pressure = 101325 Pa)

  // Print temperature
  Serial.print("Temperature: ");
  Serial.print(temperature, 2);
  Serial.println(" °C");

  // Print pressure in hPa for easier reading
  Serial.print("Pressure: ");
  Serial.print(pressure / 100.0, 2);
  Serial.println(" hPa");

  // Print altitude
  Serial.print("Altitude: ");
  Serial.print(altitude, 2);
  Serial.println(" m");

  Serial.println("---");  // Separator for each reading

  delay(500);  // Pause before next reading
}
