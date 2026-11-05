/*
  BMP580 Interrupt Example - Voltino Labs Edition

  Demonstrates the INT pin of the BMP580:
    1. Data-ready interrupt: the MCU reads the sensor only when new data exists.
    2. Pressure out-of-range (OOR) interrupt: the sensor itself watches the
       pressure and raises an alarm when it leaves a +/- window around a
       reference value (e.g. door opened, altitude change, pressure drop).

  Wiring: BMP580 INT -> MCU pin INT_PIN (must support external interrupts).
*/

#include <Wire.h>
#include "BMP580.h"

#define INT_PIN 2

BMP580 bmp;
volatile bool intFlag = false;

void onBmpInterrupt() {
  intFlag = true;
}

void setup() {
  Serial.begin(115200);

  if (!bmp.begin()) {
    Serial.println("Error: BMP580 not detected!");
    while (1);
  }

  bmp.setOversampling(BMP580_OSR_x16, BMP580_OSR_x1);
  bmp.setODR(BMP580_ODR_10Hz);
  bmp.setIIRFilter(BMP580_IIR_3, BMP580_IIR_OFF);

  // Use the current pressure as reference, alarm when it changes by > 30 Pa
  // (~2.5 m altitude) for 3 consecutive samples.
  delay(200);
  uint32_t reference = (uint32_t)bmp.readPressure();
  bmp.setPressureOutOfRange(reference, 30, BMP580_OOR_COUNT_3);

  // INT pin: enabled, latched, active high, push-pull
  bmp.configureInterrupt(true, true, true, false);
  bmp.setInterruptSource(BMP580_INT_DRDY | BMP580_INT_OOR_P);

  pinMode(INT_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(INT_PIN), onBmpInterrupt, RISING);
  bmp.getInterruptStatus();  // clear pending flags

  Serial.print("Reference pressure: ");
  Serial.print(reference);
  Serial.println(" Pa");
}

void loop() {
  if (!intFlag) return;
  intFlag = false;

  // Reading INT_STATUS clears it and releases the latched INT pin
  uint8_t status = bmp.getInterruptStatus();

  if (status & BMP580_INT_DRDY) {
    Serial.print("New data: ");
    Serial.print(bmp.readPressure() / 100.0f, 3);
    Serial.println(" hPa");
  }
  if (status & BMP580_INT_OOR_P) {
    Serial.println("!!! Pressure out of range !!!");
  }
}
