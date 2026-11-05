# BMP580 Arduino Library (Voltino Labs Edition)

Optimized Arduino library for the **Bosch BMP580** barometric pressure and temperature sensor,
written according to the datasheet **BST-BMP580-DS004**. Memory-efficient enough for 8-bit AVR
(Arduino Uno) and fast on 32-bit MCUs (ESP32, RP2040/RP2350, STM32, ...).

**Note:** This library supports **I2C communication only**.

## Features
- **Smart I2C caching** – temperature and pressure are read in a single burst (required by the
  datasheet for data consistency) and cached for one ODR period / conversion time.
- **Overflow-safe timing** – unsigned `micros()` arithmetic, no 71-minute overflow glitch.
- **Automatic I2C address fallback** – probes `0x46` and falls back to `0x47`.
- **Soft reset + power-up checks** in `begin()` (CHIP_ID, NVM ready, POR flag – datasheet 4.3.9).
- **All power modes** – NORMAL, FORCED (one-shot, low power), CONTINUOUS, STANDBY, optional DEEP STANDBY.
- **Hardware IIR filter** – correctly written in STANDBY and routed to the data registers.
- **OSR/ODR validity check** – detect combinations the sensor silently replaces by fallback values.
- **FIFO** – up to 16 P+T frames or 32 P/T frames, decimation, watermark.
- **Interrupts** – data ready, FIFO full / watermark, pressure out-of-range window; INT pin configuration.
- **Unique device ID** (48 bit) and user NVM access.
- Selectable `TwoWire` bus (e.g. `Wire1` on ESP32/RP2040).

---

## Default settings (after `begin()`)
| Setting | Value |
|---|---|
| Power mode | NORMAL |
| ODR | 199.1 Hz |
| Oversampling | Pressure x4, Temperature x1 (Bosch "standard resolution", 0.41 Pa RMS) |
| IIR filter | OFF |
| Deep standby | disabled |
| I2C clock | core default (usually 100 kHz) |

> Versions ≤ 1.1.0 used OSR x4/x4 @ 240 Hz. According to datasheet table 7 that combination is
> **invalid** (max. 200 Hz), so the sensor silently ran with OSR x1/x1.

---

## API reference

### Setup
| Function | Description |
|---|---|
| `bool begin(uint8_t addr = 0x46, TwoWire &wire = Wire)` | Probe, soft reset, power-up check, apply defaults |
| `bool softReset()` | Soft reset (CMD 0xB6) + power-up check |
| `void setI2CSpeed(uint32_t speed)` | `100000`, `400000` or `1000000` |

### Configuration
| Function | Description |
|---|---|
| `void setOversampling(BMP580_OSR osr_p, BMP580_OSR osr_t, bool pressureEnable = true)` | OSR; `pressureEnable=false` = temperature only |
| `void setODR(BMP580_ODR odr)` | Output data rate (NORMAL mode) |
| `void setPowerMode(BMP580_Mode mode)` | STANDBY / NORMAL / FORCED / CONTINUOUS (always via STANDBY) |
| `BMP580_Mode getPowerMode()` | Current mode |
| `void setIIRFilter(BMP580_IIR iir_p, BMP580_IIR iir_t)` | Hardware low-pass filter |
| `void setDeepStandby(bool enable)` | Allow ~0.55 µA deep standby (ODR < 5 Hz, FIFO off, IIR off) |
| `bool isConfigValid()` | `false` if the OSR/ODR combination is invalid (NORMAL mode) |
| `void getEffectiveOSR(BMP580_OSR &p, BMP580_OSR &t)` | OSR actually used by the sensor |
| `uint32_t getMeasurementTimeUs()` | Expected conversion time for the current OSR |

### Reading data
| Function | Description |
|---|---|
| `float readTemperature()` | °C |
| `float readPressure()` | Pa |
| `bool read(float &t, float &p)` | Both values, returns `false` on I2C error |
| `float readAltitude(float seaLevelPressure = 101325.0f)` | m (international barometric formula) |
| `float seaLevelPressure(float altitude)` | Sea-level pressure (Pa) for a known altitude |
| `bool takeForcedMeasurement()` | Trigger one FORCED measurement and wait for it |

In FORCED mode `readTemperature()` / `readPressure()` trigger a new measurement automatically.

### Interrupts
| Function | Description |
|---|---|
| `void configureInterrupt(bool enable, bool latched = true, bool activeHigh = true, bool openDrain = false)` | INT pin |
| `void setInterruptSource(uint8_t mask)` | `BMP580_INT_DRDY`, `BMP580_INT_FIFO_FULL`, `BMP580_INT_FIFO_THS`, `BMP580_INT_OOR_P` |
| `uint8_t getInterruptStatus()` | Read & clear status flags |
| `bool dataReady()` | New data available (needs `BMP580_INT_DRDY` source) |
| `void setPressureOutOfRange(uint32_t refPa, uint8_t windowPa, BMP580_OORCount count)` | Alarm when pressure leaves ±window |

### FIFO
| Function | Description |
|---|---|
| `void configureFIFO(BMP580_FIFOFrame frame, uint8_t decimation = 0, bool stopOnFull = false, uint8_t threshold = 0)` | Enable/disable FIFO |
| `uint8_t getFIFOCount()` | Number of stored frames |
| `uint8_t readFIFO(float *p, float *t, uint8_t maxFrames)` | Read frames (pass `nullptr` for unused data) |
| `void flushFIFO()` | Empty the FIFO |

### Identification / NVM
| Function | Description |
|---|---|
| `uint8_t getChipID()` / `uint8_t getRevisionID()` | 0x50 / ASIC revision |
| `uint64_t getUniqueID()` | 48-bit unique ID |
| `bool readNVM(uint8_t addr, uint16_t &data)` | Read any NVM row |
| `bool writeUserNVM(uint8_t addr, uint16_t data)` | Write user rows 0x20–0x22 (max. 10 000 cycles!) |

---

## Valid OSR / ODR combinations (NORMAL mode, datasheet table 7)
Maximum ODR in Hz:

| OSR_P \ OSR_T | x1 | x2 | x4 | x8 | x16 | x32 | x64 | x128 |
|---|---|---|---|---|---|---|---|---|
| **x1**   | 240 | 240 | 240 | 240 | 200 | 130 | 80 | 40 |
| **x2**   | 240 | 240 | 240 | 220 | 180 | 120 | 70 | 40 |
| **x4**   | 220 | 220 | 200 | 180 | 140 | 100 | 70 | 40 |
| **x8**   | 140 | 140 | 130 | 120 | 100 | 80 | 50 | 35 |
| **x16**  | 80 | 80 | 80 | 70 | 70 | 50 | 45 | 30 |
| **x32**  | 45 | 45 | 40 | 40 | 40 | 35 | 30 | 20 |
| **x64**  | 20 | 20 | 20 | 20 | 20 | 20 | 15 | 15 |
| **x128** | 10 | 10 | 10 | 10 | 10 | 10 | 10 | 5 |

## IIR filter (datasheet table 10)
| Enum | Coefficient | Normalized bandwidth (-3 dB) |
|---|---|---|
| `BMP580_IIR_OFF` | bypass | – |
| `BMP580_IIR_1` | 1 | 0.1147 × ODR |
| `BMP580_IIR_3` | 3 | 0.0459 × ODR |
| `BMP580_IIR_7` | 7 | 0.0212 × ODR |
| `BMP580_IIR_15` | 15 | 0.01025 × ODR |
| `BMP580_IIR_31` | 31 | 0.005041 × ODR |
| `BMP580_IIR_63` | 63 | 0.00250 × ODR |
| `BMP580_IIR_127` | 127 | 0.00125 × ODR |

---

## Examples
- `BMP580_basic_example` – default settings
- `BMP580_mode_configuration` – I2C speed, OSR, ODR, IIR, validity check
- `BMP580_forced_mode` – low-power one-shot measurements
- `BMP580_fifo_buffer` – FIFO with watermark
- `BMP580_interrupts` – data-ready and pressure out-of-range interrupt on the INT pin
- `BMP580_sensor_info` – chip ID, revision, unique ID, OSR/ODR validity

---
License: MIT
