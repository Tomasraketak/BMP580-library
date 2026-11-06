#ifndef BMP580_H
#define BMP580_H

#include <Arduino.h>
#include <Wire.h>

// ---------------------------------------------------------------------------
// I2C addresses (datasheet BST-BMP580-DS004-02, chapter 5.6)
//   SDO = GND -> 0x46, SDO = VDDIO -> 0x47
// ---------------------------------------------------------------------------
#define BMP580_PRIMARY_I2C_ADDR   0x46
#define BMP580_SECONDARY_I2C_ADDR 0x47

#define BMP580_CHIP_ID_VALUE      0x50

// ---------------------------------------------------------------------------
// Register map (datasheet chapter 7)
// ---------------------------------------------------------------------------
#define BMP580_CHIP_ID         0x01
#define BMP580_REV_ID          0x02
#define BMP580_CHIP_STATUS     0x11
#define BMP580_DRIVE_CONFIG    0x13
#define BMP580_INT_CONFIG      0x14
#define BMP580_INT_SOURCE      0x15
#define BMP580_FIFO_CONFIG     0x16
#define BMP580_FIFO_COUNT      0x17
#define BMP580_FIFO_SEL        0x18
#define BMP580_TEMP_DATA_XLSB  0x1D  // Burst read start (T XLSB..MSB, P XLSB..MSB)
#define BMP580_PRESS_DATA_XLSB 0x20
#define BMP580_INT_STATUS      0x27  // clear-on-read
#define BMP580_STATUS          0x28
#define BMP580_FIFO_DATA       0x29
#define BMP580_NVM_ADDR        0x2B
#define BMP580_NVM_DATA_LSB    0x2C
#define BMP580_NVM_DATA_MSB    0x2D
#define BMP580_DSP_CONFIG      0x30  // IIR routing (data regs / FIFO / OOR) + IIR flush
#define BMP580_DSP_IIR         0x31  // IIR filter coefficients
#define BMP580_OOR_THR_P_LSB   0x32
#define BMP580_OOR_THR_P_MSB   0x33
#define BMP580_OOR_RANGE       0x34
#define BMP580_OOR_CONFIG      0x35
#define BMP580_OSR_CONFIG      0x36
#define BMP580_ODR_CONFIG      0x37
#define BMP580_OSR_EFF         0x38
#define BMP580_CMD             0x7E

// Interrupt source / status bit masks (INT_SOURCE 0x15, INT_STATUS 0x27)
#define BMP580_INT_DRDY        0x01  // Data ready
#define BMP580_INT_FIFO_FULL   0x02  // FIFO full
#define BMP580_INT_FIFO_THS    0x04  // FIFO threshold (watermark) reached
#define BMP580_INT_OOR_P       0x08  // Pressure out of range
#define BMP580_INT_POR         0x10  // Power-on / soft reset complete (status only)

// IIR Filter coefficients (DSP_IIR 0x31)
enum BMP580_IIR {
  BMP580_IIR_OFF = 0x00,
  BMP580_IIR_1   = 0x01,
  BMP580_IIR_3   = 0x02,
  BMP580_IIR_7   = 0x03,
  BMP580_IIR_15  = 0x04,
  BMP580_IIR_31  = 0x05,
  BMP580_IIR_63  = 0x06,
  BMP580_IIR_127 = 0x07
};

// Oversampling rates (OSR_CONFIG 0x36)
enum BMP580_OSR {
  BMP580_OSR_x1   = 0x0,
  BMP580_OSR_x2   = 0x1,
  BMP580_OSR_x4   = 0x2,
  BMP580_OSR_x8   = 0x3,
  BMP580_OSR_x16  = 0x4,
  BMP580_OSR_x32  = 0x5,
  BMP580_OSR_x64  = 0x6,
  BMP580_OSR_x128 = 0x7
};

// Output data rates (ODR_CONFIG 0x37) - only used in NORMAL mode
enum BMP580_ODR {
  BMP580_ODR_240Hz   = 0x00,
  BMP580_ODR_218p5Hz = 0x01,
  BMP580_ODR_199p1Hz = 0x02,
  BMP580_ODR_179p2Hz = 0x03,
  BMP580_ODR_160Hz   = 0x04,
  BMP580_ODR_149p3Hz = 0x05,
  BMP580_ODR_140Hz   = 0x06,
  BMP580_ODR_129p9Hz = 0x07,
  BMP580_ODR_120Hz   = 0x08,
  BMP580_ODR_110p2Hz = 0x09,
  BMP580_ODR_100p3Hz = 0x0A,
  BMP580_ODR_89p6Hz  = 0x0B,
  BMP580_ODR_80Hz    = 0x0C,
  BMP580_ODR_70Hz    = 0x0D,
  BMP580_ODR_60Hz    = 0x0E,
  BMP580_ODR_50p1Hz  = 0x0F,
  BMP580_ODR_45Hz    = 0x10,
  BMP580_ODR_40Hz    = 0x11,
  BMP580_ODR_35Hz    = 0x12,
  BMP580_ODR_30Hz    = 0x13,
  BMP580_ODR_25Hz    = 0x14,
  BMP580_ODR_20Hz    = 0x15,
  BMP580_ODR_15Hz    = 0x16,
  BMP580_ODR_10Hz    = 0x17,
  BMP580_ODR_5Hz     = 0x18,
  BMP580_ODR_4Hz     = 0x19,
  BMP580_ODR_3Hz     = 0x1A,
  BMP580_ODR_2Hz     = 0x1B,
  BMP580_ODR_1Hz     = 0x1C,
  BMP580_ODR_0p5Hz   = 0x1D,
  BMP580_ODR_0p25Hz  = 0x1E,
  BMP580_ODR_0p125Hz = 0x1F
};

// Power modes (ODR_CONFIG.pwr_mode)
enum BMP580_Mode {
  BMP580_MODE_STANDBY    = 0x00,
  BMP580_MODE_NORMAL     = 0x01,
  BMP580_MODE_FORCED     = 0x02,
  BMP580_MODE_CONTINUOUS = 0x03
};

// FIFO frame selection (FIFO_SEL.fifo_frame_sel)
enum BMP580_FIFOFrame {
  BMP580_FIFO_DISABLED         = 0x00,
  BMP580_FIFO_TEMPERATURE      = 0x01,  // max 32 frames
  BMP580_FIFO_PRESSURE         = 0x02,  // max 32 frames
  BMP580_FIFO_PRESS_AND_TEMP   = 0x03   // max 16 frames
};

// Out-of-range interrupt: number of consecutive samples (OOR_CONFIG.cnt_lim)
enum BMP580_OORCount {
  BMP580_OOR_COUNT_1  = 0x00,
  BMP580_OOR_COUNT_3  = 0x01,
  BMP580_OOR_COUNT_7  = 0x02,
  BMP580_OOR_COUNT_15 = 0x03
};

class BMP580 {
 public:
  BMP580();

  // --- Initialisation -----------------------------------------------------
  // Probes the sensor (with 0x46 -> 0x47 fallback), performs a soft reset,
  // verifies the power-up status and applies the default configuration.
  bool begin(uint8_t addr = BMP580_PRIMARY_I2C_ADDR, TwoWire &wire = Wire);
  bool softReset();

  void setI2CSpeed(uint32_t speed);  // 100000, 400000 or 1000000 (Fast Mode Plus)

  // --- Measurement configuration ------------------------------------------
  // pressureEnable = false -> temperature-only measurements (lower power)
  void setOversampling(BMP580_OSR osr_p, BMP580_OSR osr_t, bool pressureEnable = true);
  void setODR(BMP580_ODR odr);
  void setPowerMode(BMP580_Mode mode);
  BMP580_Mode getPowerMode() const { return (BMP580_Mode)_mode; }
  void setIIRFilter(BMP580_IIR iir_p, BMP580_IIR iir_t);
  // Allows the sensor to use DEEP STANDBY (~0.55 uA) between NORMAL-mode
  // measurements. Only active when ODR < 5 Hz, FIFO disabled and IIR off.
  void setDeepStandby(bool enable);

  // Configuration check (datasheet 4.3.4). Only evaluated by the sensor in
  // NORMAL mode. If false, the sensor silently runs with OSR x1 (ODR >= 160 Hz)
  // or OSR x2 (ODR < 160 Hz) instead of the requested values.
  bool isConfigValid();
  void getEffectiveOSR(BMP580_OSR &osr_p, BMP580_OSR &osr_t);
  // Expected duration of one P+T conversion in microseconds (datasheet table 4)
  uint32_t getMeasurementTimeUs() const;

  // --- Data ----------------------------------------------------------------
  float readTemperature();                                // [degC]
  float readPressure();                                   // [Pa]
  float readAltitude(float seaLevelPressure = 101325.0f); // [m]
  // Sea-level pressure [Pa] for a known altitude [m] (QNH calibration)
  float seaLevelPressure(float altitude);
  // Reads both values with a single burst. Returns false on I2C error.
  bool read(float &temperature, float &pressure);
  // Triggers a single FORCED measurement and waits for it to finish.
  bool takeForcedMeasurement();

  // --- Interrupts ------------------------------------------------------------
  // Configures the INT pin (INT_CONFIG). latched=false -> 105 us pulses.
  void configureInterrupt(bool enable, bool latched = true, bool activeHigh = true, bool openDrain = false);
  // Bitmask of BMP580_INT_DRDY | BMP580_INT_FIFO_FULL | BMP580_INT_FIFO_THS | BMP580_INT_OOR_P
  void setInterruptSource(uint8_t sources);
  // Reads (and clears) INT_STATUS - returns a bitmask of BMP580_INT_* flags
  uint8_t getInterruptStatus();
  // True if new data is ready. Requires BMP580_INT_DRDY in setInterruptSource().
  // Note: reading INT_STATUS clears all other status flags as well.
  bool dataReady();

  // Pressure out-of-range window: triggers when |p - referencePa| > windowPa
  // for 'count' consecutive samples. referencePa: 0..131071 Pa, windowPa: 0..255 Pa.
  // useFilteredPressure = true -> compare the IIR-filtered pressure (DSP_CONFIG.oor_sel_iir_p)
  void setPressureOutOfRange(uint32_t referencePa, uint8_t windowPa, BMP580_OORCount count = BMP580_OOR_COUNT_1,
                             bool useFilteredPressure = false);

  // --- FIFO --------------------------------------------------------------------
  // decimation: store only every 2^decimation sample (0..7)
  // threshold : FIFO watermark in frames (0 = disabled, max 15 in P+T mode, 31 otherwise)
  void configureFIFO(BMP580_FIFOFrame frame, uint8_t decimation = 0, bool stopOnFull = false, uint8_t threshold = 0);
  uint8_t getFIFOCount();
  // Reads up to maxFrames frames. Pass nullptr for values not stored in the FIFO.
  // Returns the number of frames actually read.
  uint8_t readFIFO(float *pressure, float *temperature, uint8_t maxFrames);
  void flushFIFO();

  // --- Identification / NVM ------------------------------------------------
  uint8_t getChipID();
  uint8_t getRevisionID();
  bool readNVM(uint8_t nvmAddr, uint16_t &data);
  // User NVM range 0x20..0x22 only. Max. 10 000 write cycles, stable supply
  // and 0..65 degC required. Use with care!
  bool writeUserNVM(uint8_t nvmAddr, uint16_t data);
  // 48-bit unique device identifier (datasheet 4.8.1.3). Returns 0 on error.
  uint64_t getUniqueID();

  // --- Low level -----------------------------------------------------------
  void writeRegister(uint8_t reg, uint8_t value);
  uint8_t readRegister(uint8_t reg);

 private:
  TwoWire *_wire;
  uint8_t _i2cAddr;

  // Shadow copy of configuration (avoids extra I2C reads)
  uint8_t _mode;
  uint8_t _odr;
  uint8_t _osrP;
  uint8_t _osrT;
  bool _pressEn;
  bool _deepDis;
  uint8_t _fifoFrame;

  // Smart caching variables
  float _cachedTemp;
  float _cachedPress;
  unsigned long _lastReadTimeUs;
  unsigned long _cacheTimeoutUs;
  bool _cacheValid;

  // Internal helpers
  bool readBurst(uint8_t reg, uint8_t *buffer, uint8_t length);
  uint8_t enterStandby();
  void restoreMode(uint8_t mode);
  bool waitNVMReady(uint16_t timeoutMs);

  bool updateCache();
  bool refreshIfStale();
  void updateTimeoutUs();

  static float convertTemperature(const uint8_t *raw);
  static float convertPressure(const uint8_t *raw);
};

#endif
