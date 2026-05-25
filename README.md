# BMP580 Arduino Library (Voltino Labs Edition)

## Description
This highly optimized Arduino library provides comprehensive functionality for the Bosch BMP580 barometric pressure and temperature sensor. It is engineered to be exceptionally memory-efficient for 8-bit architectures like the Arduino Uno, while delivering maximum performance on modern 32-bit microcontrollers such as the ESP32 and RP2350.

**Note:** This library supports **only I2C communication**.

### Premium Optimization Features:
- **Smart I2C Caching:** Eliminates redundant I2C bus transactions. Reading temperature and pressure simultaneously fetches data in a single burst read and caches it according to the selected ODR.
- **Overflow-Safe Timing:** Uses specialized unsigned subtraction math to prevent the 71-minute `micros()` overflow glitch from locking up or corrupting data tracking.
- **Automatic I2C Address Fallback:** Automatically probes the primary address (`0x46`), and seamlessly switches to the secondary address (`0x47`) if the sensor is detected there instead.
- **Hardware IIR Filtering:** Configurable internal Infinite Impulse Response (IIR) filter to suppress high-frequency noise (e.g., drone propeller wind) directly inside the sensor hardware.
- **Explicit I2C Clock Control:** Supports tailoring the I2C speed from standard 100 kHz up to 1 MHz (Fast Mode Plus) for low-latency loops.

---

## Default Settings (Upon calling `begin()`)
- **I2C Clock Speed:** 100 kHz
- **Output Data Rate (ODR):** 240 Hz
- **Oversampling (OSR):** 4x (for both Pressure & Temperature)
- **Power Mode:** NORMAL
- **Internal IIR Filter:** OFF (0)

---

## Features & API Reference
- `bool begin(uint8_t addr = BMP580_PRIMARY_I2C_ADDR)` - Initializes the sensor, applies defaults, and performs automatic dual-address verification.
- `void setI2CSpeed(uint32_t speed)` - Changes I2C clock frequency (e.g., `100000`, `400000`, `1000000`).
- `void setOversampling(BMP580_OSR osr_p, BMP580_OSR osr_t)` - Set oversampling configuration.
- `void setODR(BMP580_ODR odr)` - Dynamically sets output data rate and updates cache timeouts seamlessly.
- `void setPowerMode(BMP580_Mode mode)` - Set power state.
- `void setIIRFilter(BMP580_IIR iir_p, BMP580_IIR iir_t)` - Configures internal hardware low-pass filtering.
- `float readTemperature()` - Returns cached or freshly sampled temperature (°C).
- `float readPressure()` - Returns cached or freshly sampled pressure (Pa).
- `float readAltitude(float seaLevelPressure = 101325.0f)` - Calculates precise altitude (m) based on current pressure.

---

## Supported Configurations

### Hardware IIR Filter Coefficients (`BMP580_IIR`)
`BMP580_IIR_OFF`, `BMP580_IIR_1`, `BMP580_IIR_3`, `BMP580_IIR_7`, `BMP580_IIR_15`, `BMP580_IIR_31`, `BMP580_IIR_63`, `BMP580_IIR_127`

*Note: Increasing the IIR coefficient smooths out noise but introduces measurement latency (lowers data responsiveness).*

### Oversampling Rates (`BMP580_OSR`)
`BMP580_OSR_x1`, `BMP580_OSR_x2`, `BMP580_OSR_x4`, `BMP580_OSR_x8`, `BMP580_OSR_x16`, `BMP580_OSR_x32`, `BMP580_OSR_x64`, `BMP580_OSR_x128`

### Output Data Rates (`BMP580_ODR`)
From `BMP580_ODR_240Hz` down to `BMP580_ODR_0p125Hz` (consult `BMP580.h` for full enum list).

---
License: MIT