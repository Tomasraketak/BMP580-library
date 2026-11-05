/*
  BMP580 Sensor Info Example - Voltino Labs Edition

  Prints the chip ID, ASIC revision, the 48-bit unique device ID stored in the
  sensor NVM, and checks whether the active OSR/ODR combination is valid.
*/

#include <Wire.h>
#include "BMP580.h"

BMP580 bmp;

void printHex(uint32_t value, uint8_t digits) {
  for (int8_t i = digits - 1; i >= 0; i--) {
    Serial.print((value >> (i * 4)) & 0x0F, HEX);
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  if (!bmp.begin()) {
    Serial.println("Error: BMP580 not detected!");
    while (1);
  }

  Serial.print("Chip ID     : 0x");
  printHex(bmp.getChipID(), 2);
  Serial.println();

  Serial.print("Revision ID : 0x");
  printHex(bmp.getRevisionID(), 2);
  Serial.println();

  uint64_t uid = bmp.getUniqueID();
  Serial.print("Unique ID   : 0x");
  printHex((uint32_t)(uid >> 32), 4);
  printHex((uint32_t)uid, 8);
  Serial.println();

  // Try an invalid combination on purpose: OSR_P x128 cannot run at 240 Hz
  bmp.setOversampling(BMP580_OSR_x128, BMP580_OSR_x8);
  bmp.setODR(BMP580_ODR_240Hz);
  delay(10);

  BMP580_OSR effP, effT;
  bmp.getEffectiveOSR(effP, effT);
  Serial.print("x128 @ 240 Hz valid? ");
  Serial.print(bmp.isConfigValid() ? "yes" : "NO");
  Serial.print(" -> effective OSR_P x");
  Serial.print(1 << effP);
  Serial.print(", OSR_T x");
  Serial.println(1 << effT);

  // A valid combination (datasheet table 7: max 10 Hz)
  bmp.setODR(BMP580_ODR_10Hz);
  delay(10);
  Serial.print("x128 @ 10 Hz valid?  ");
  Serial.println(bmp.isConfigValid() ? "yes" : "NO");
}

void loop() {}
