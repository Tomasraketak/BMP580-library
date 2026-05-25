/*
  BMP580 Advanced Configuration Example - Voltino Labs Edition

  This example demonstrates how to configure advanced features of the BMP580,
  specifically boosting the I2C communication speed and configuring the internal 
  hardware Infinite Impulse Response (IIR) filter.

  IIR Filter impact notice:
    - Activating the internal hardware filter smooths out rapid pressure spikes 
      (such as wind gusts from drone propellers or atmospheric noise).
    - It does NOT alter the physical ODR frequency, but it introduces step latency 
      to the measurements (higher coefficients = smoother lines but slower response).
*/

#include <Wire.h>
#include "BMP580.h"

BMP580 bmp;

void setup() {
  Serial.begin(115200);

  // Initialize the sensor with explicit address if desired, or leave empty for auto-probing
  if (!bmp.begin(BMP580_PRIMARY_I2C_ADDR)) {
    Serial.println("Error: BMP580 sensor could not be initialized!");
    while (1);
  }

  Serial.println("BMP580 base system online.");

  // 1. Boost I2C Bus Speed to Fast Mode (400 kHz)
  // Modern MCUs like ESP32 or RP2350 can go up to 1000000 (1 MHz), while AVR Uno safely supports 400000.
  bmp.setI2CSpeed(400000);
  Serial.println("I2C speed boosted to 400 kHz (Fast Mode).");

  // 2. Adjust Oversampling Configuration (e.g., 8x for pressure to get cleaner data, 2x for temperature)
  bmp.setOversampling(BMP580_OSR_x8, BMP580_OSR_x2);

  // 3. Set Output Data Rate (ODR) to 50 Hz 
  // Our internal smart cache timeout will automatically and safely adapt to this new rate.
  bmp.setODR(BMP580_ODR_50p1Hz);

  // 4. Configure Internal Hardware IIR Filter
  // Let's set both pressure and temperature hardware filtering to a coefficient of 15.
  // This heavily filters ambient environmental noise natively inside the Bosch silicon.
  bmp.setIIRFilter(BMP580_IIR_15, BMP580_IIR_15);
  Serial.println("Internal hardware IIR filter activated (Coefficient: 15).");

  // 5. Enforce Continuous power mode for stable background updates
  bmp.setPowerMode(BMP580_MODE_CONTINUOUS);

  Serial.println("Advanced configurations successfully applied.");
  delay(100); 
}

void loop() {
  // Data reading remains exactly the same, but the internal management is completely transformed.
  // Readings here are refreshed strictly at 50Hz, fully guarded against micros() overflow conditions.
  float temp = bmp.readTemperature();
  float press = bmp.readPressure();

  Serial.print("Filtered Pressure: ");
  Serial.print(press / 100.0f, 2);
  Serial.print(" hPa | Temp: ");
  Serial.print(temp, 2);
  Serial.println(" °C");

  // Loop runs at 100Hz, but I2C burst reads only happen at 50Hz due to automated cache timing!
  delay(10); 
}