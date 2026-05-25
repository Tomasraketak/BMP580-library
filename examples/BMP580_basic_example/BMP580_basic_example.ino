/*
  BMP580 Basic Example - Voltino Labs Edition

  This example demonstrates using the BMP580 sensor with its optimal out-of-the-box defaults.
  Thanks to smart caching, calling readTemperature() and readPressure() in immediate succession
  only executes a single I2C burst transaction, heavily optimizing loop speed.

  Default settings applied automatically:
    - I2C Clock Speed: 100 kHz
    - Oversampling: 4x (Pressure & Temperature)
    - Output Data Rate (ODR): 240 Hz
    - Power Mode: NORMAL
    - Internal IIR Filter: OFF (0)
*/

#include <Wire.h>
#include "BMP580.h"  // Include our optimized BMP580 library

BMP580 bmp;  // Create a BMP580 sensor object

void setup() {
  Serial.begin(115200);  // Initialize serial communication
  
  // Initialize BMP580. 
  // It automatically handles the 100kHz I2C speed setup and probes for 
  // both primary (0x46) and secondary (0x47) addresses autonomously.
  if (!bmp.begin()) {
    Serial.println("Error: BMP580 not detected on the I2C bus!");
    while (1);  // Halt execution if sensor initialization fails
  }

  Serial.println("BMP580 initialized successfully with standard VoltinoLabs defaults.");
  delay(100);  // Short delay for sensor stabilization
}

void loop() {
  // Read sensor values. 
  // The library checks the data age against the 240Hz timeout (4.16ms).
  // The first call triggers a hardware I2C burst read, while the second call 
  // instantly serves data from the ultra-fast local RAM cache.
  float temperature = bmp.readTemperature();       // Temperature in °C
  float pressure = bmp.readPressure();             // Pressure in Pascals (Pa)
  float altitude = bmp.readAltitude(101325.0f);    // Altitude in meters (assuming standard sea-level pressure)

  // Print temperature
  Serial.print("Temperature: ");
  Serial.print(temperature, 2);
  Serial.print(" °C | ");

  // Print pressure converted to hPa for easier human reading
  Serial.print("Pressure: ");
  Serial.print(pressure / 100.0f, 2);
  Serial.print(" hPa | ");

  // Print calculated altitude
  Serial.print("Altitude: ");
  Serial.print(altitude, 2);
  Serial.println(" m");

  // Feel free to delay or run at full speed; the smart cache protects the I2C bus!
  delay(50); 
}