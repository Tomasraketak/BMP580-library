# BMP580 Arduino Library

## Description
This Arduino library provides basic functionality for the Bosch BMP580 sensor. 
It allows you to read pressure, temperature, and calculate altitude. 
**Note:** This library supports **only I2C communication**.

Default settings used by the library:
- Oversampling: 4x (pressure & temperature)
- Output Data Rate (ODR): 240 Hz
- Power Mode: NORMAL

---

## Features
- Read temperature (°C)
- Read pressure (Pa or hPa)
- Calculate altitude from pressure (m)
- Set oversampling for temperature and pressure
- Set output data rate (ODR)
- Set power mode (STANDBY, NORMAL, FORCED, CONTINUOUS)

---

## Supported Settings

### Oversampling (OSR)
BMP580_OSR_x1, BMP580_OSR_x2, BMP580_OSR_x4, BMP580_OSR_x8,
BMP580_OSR_x16, BMP580_OSR_x32, BMP580_OSR_x64, BMP580_OSR_x128

shell
Zkopírovat kód

### Output Data Rate (ODR)
BMP580_ODR_240Hz, BMP580_ODR_218p5Hz, BMP580_ODR_199p1Hz, BMP580_ODR_179p2Hz,
BMP580_ODR_160Hz, BMP580_ODR_149p3Hz, BMP580_ODR_140Hz, BMP580_ODR_129p9Hz,
BMP580_ODR_120Hz, BMP580_ODR_110p2Hz, BMP580_ODR_100p3Hz, BMP580_ODR_89p6Hz,
BMP580_ODR_80Hz, BMP580_ODR_70Hz, BMP580_ODR_60Hz, BMP580_ODR_50p1Hz,
BMP580_ODR_45Hz, BMP580_ODR_40Hz, BMP580_ODR_35Hz, BMP580_ODR_30Hz,
BMP580_ODR_25Hz, BMP580_ODR_20Hz, BMP580_ODR_15Hz, BMP580_ODR_10Hz,
BMP580_ODR_5Hz, BMP580_ODR_4Hz, BMP580_ODR_3Hz, BMP580_ODR_2Hz,
BMP580_ODR_1Hz, BMP580_ODR_0p5Hz, BMP580_ODR_0p25Hz, BMP580_ODR_0p125Hz


### Power Modes
BMP580_MODE_STANDBY
BMP580_MODE_NORMAL
BMP580_MODE_FORCED
BMP580_MODE_CONTINUOUS

---

## Installation
1. Copy the `BMP580` folder into your Arduino `libraries` directory.
2. Include the library in your sketch:

#include <BMP580.h>
Create a BMP580 object:

BMP580 bmp;
Initialize the sensor in setup():


if (!bmp.begin()) {
  Serial.println("BMP580 initialization failed!");
  while (1);
}