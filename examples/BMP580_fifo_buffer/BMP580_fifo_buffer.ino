/*
  BMP580 FIFO Example - Voltino Labs Edition

  The BMP580 has an internal FIFO that can store up to 16 pressure+temperature
  frames (or 32 pressure-only / temperature-only frames). The MCU can sleep or
  do other work and read all samples at once in a single burst.

  Here the sensor samples at 25 Hz and we read the FIFO once per 0.5 s.
*/

#include <Wire.h>
#include "BMP580.h"

BMP580 bmp;

float pressure[16];
float temperature[16];

void setup() {
  Serial.begin(115200);

  if (!bmp.begin()) {
    Serial.println("Error: BMP580 not detected!");
    while (1);
  }

  bmp.setOversampling(BMP580_OSR_x8, BMP580_OSR_x1);
  bmp.setODR(BMP580_ODR_25Hz);

  // Store pressure + temperature, no decimation, streaming mode (oldest
  // frames are overwritten), watermark at 10 frames.
  bmp.configureFIFO(BMP580_FIFO_PRESS_AND_TEMP, 0, false, 10);
  bmp.setInterruptSource(BMP580_INT_FIFO_THS | BMP580_INT_FIFO_FULL);
}

void loop() {
  uint8_t status = bmp.getInterruptStatus();
  if (status & BMP580_INT_FIFO_FULL) {
    Serial.println("FIFO overflow - samples were lost, read it faster!");
  }

  uint8_t n = bmp.readFIFO(pressure, temperature, 16);
  Serial.print(n);
  Serial.println(" frames read from FIFO:");
  for (uint8_t i = 0; i < n; i++) {
    Serial.print("  ");
    Serial.print(pressure[i] / 100.0f, 3);
    Serial.print(" hPa  ");
    Serial.print(temperature[i], 2);
    Serial.println(" degC");
  }

  delay(500);
}
