#include "BMP580.h"

// Default values applied by begin().
// NOTE: OSR_P x4 / OSR_T x1 is Bosch's "standard resolution" setting
// (0.41 Pa RMS). According to datasheet table 7 the maximum ODR for this
// combination is 220 Hz, therefore 199.1 Hz is used. (The previous default
// OSR x4/x4 @ 240 Hz was invalid - the sensor silently fell back to OSR x1.)
#define DEFAULT_OSR_P BMP580_OSR_x4
#define DEFAULT_OSR_T BMP580_OSR_x1
#define DEFAULT_ODR   BMP580_ODR_199p1Hz
#define DEFAULT_MODE  BMP580_MODE_NORMAL
#define DEFAULT_IIR   BMP580_IIR_OFF

// Timings from datasheet table 4 (with margin)
#define BMP580_T_POWERUP_MS   2   // t_powup
#define BMP580_T_SOFTRES_MS   3   // tsoft_res = 2 ms
#define BMP580_T_STANDBY_MS   3   // tstandby  = 2.5 ms
#define BMP580_T_STARTUP_MS   4   // tstartup_deep = 4 ms

// Max bytes per I2C read (AVR Wire buffer is 32 bytes)
#define BMP580_I2C_CHUNK      30

// Bit fields
#define ODR_CONFIG_DEEP_DIS   0x80
#define ODR_CONFIG_ODR_MASK   0x7C
#define ODR_CONFIG_MODE_MASK  0x03
#define OSR_CONFIG_PRESS_EN   0x40
#define DSP_CONFIG_SHDW_T     0x08
#define DSP_CONFIG_FIFO_T     0x10
#define DSP_CONFIG_SHDW_P     0x20
#define DSP_CONFIG_FIFO_P     0x40
#define STATUS_NVM_RDY        0x02
#define STATUS_NVM_ERR        0x04
#define STATUS_NVM_CMD_ERR    0x08

BMP580::BMP580()
    : _wire(&Wire),
      _i2cAddr(BMP580_PRIMARY_I2C_ADDR),
      _mode(BMP580_MODE_STANDBY),
      _odr(BMP580_ODR_1Hz),
      _osrP(BMP580_OSR_x1),
      _osrT(BMP580_OSR_x1),
      _pressEn(false),
      _deepDis(true),
      _fifoFrame(BMP580_FIFO_DISABLED),
      _cachedTemp(0.0f),
      _cachedPress(0.0f),
      _lastReadTimeUs(0),
      _cacheTimeoutUs(0),
      _cacheValid(false) {}

// ---------------------------------------------------------------------------
// Initialisation
// ---------------------------------------------------------------------------

bool BMP580::begin(uint8_t addr, TwoWire &wire) {
  _wire = &wire;
  _wire->begin();
  delay(BMP580_T_POWERUP_MS);
  _i2cAddr = addr;

  uint8_t chipId = readRegister(BMP580_CHIP_ID);

  // Smart failover: if default primary fails, try secondary 0x47
  if (chipId != BMP580_CHIP_ID_VALUE && addr == BMP580_PRIMARY_I2C_ADDR) {
    _i2cAddr = BMP580_SECONDARY_I2C_ADDR;
    chipId = readRegister(BMP580_CHIP_ID);
  }
  if (chipId != BMP580_CHIP_ID_VALUE) {
    return false;
  }

  // Bring the device into a known state and run the recommended
  // post-power-up checks (datasheet 4.3.9).
  if (!softReset()) {
    return false;
  }

  // After reset the device is in DEEP STANDBY. Leave it to STANDBY and write
  // the configuration there (recommended by datasheet 4.3.7 / 4.3.8).
  _odr = DEFAULT_ODR;
  writeRegister(BMP580_ODR_CONFIG, ODR_CONFIG_DEEP_DIS | (_odr << 2) | BMP580_MODE_STANDBY);
  delay(BMP580_T_STANDBY_MS);

  setOversampling(DEFAULT_OSR_P, DEFAULT_OSR_T, true);
  setIIRFilter(DEFAULT_IIR, DEFAULT_IIR);
  setPowerMode(DEFAULT_MODE);

  // Wait for start-up + first conversion, then pre-load the cache
  delay(BMP580_T_STARTUP_MS + getMeasurementTimeUs() / 1000 + 2);
  updateCache();
  return true;
}

bool BMP580::softReset() {
  // The sensor does NOT acknowledge this write over I2C (datasheet 7.36),
  // so the return value of endTransmission() is ignored.
  writeRegister(BMP580_CMD, 0xB6);
  delay(BMP580_T_SOFTRES_MS);

  // Post-power-up checks (datasheet 4.3.9)
  if (readRegister(BMP580_CHIP_ID) == 0x00) return false;
  uint8_t status = readRegister(BMP580_STATUS);
  if (!(status & STATUS_NVM_RDY) || (status & STATUS_NVM_ERR)) return false;
  if (!(readRegister(BMP580_INT_STATUS) & BMP580_INT_POR)) return false;

  // Register defaults after reset
  _mode = BMP580_MODE_STANDBY;
  _odr = BMP580_ODR_1Hz;
  _osrP = BMP580_OSR_x1;
  _osrT = BMP580_OSR_x1;
  _pressEn = false;
  _deepDis = true;  // the library keeps deep standby disabled unless requested
  _fifoFrame = BMP580_FIFO_DISABLED;
  _cacheValid = false;
  updateTimeoutUs();
  return true;
}

void BMP580::setI2CSpeed(uint32_t speed) {
  // Standard (100000), Fast (400000) or Fast Mode Plus (1000000)
  _wire->setClock(speed);
}

// ---------------------------------------------------------------------------
// Measurement configuration
// ---------------------------------------------------------------------------

void BMP580::setOversampling(BMP580_OSR osr_p, BMP580_OSR osr_t, bool pressureEnable) {
  _osrP = osr_p & 0x07;
  _osrT = osr_t & 0x07;
  _pressEn = pressureEnable;
  // OSR_CONFIG may be changed in any mode (datasheet 4.4.3)
  writeRegister(BMP580_OSR_CONFIG, (pressureEnable ? OSR_CONFIG_PRESS_EN : 0) | (_osrP << 3) | _osrT);
  _cacheValid = false;
  updateTimeoutUs();
}

void BMP580::setODR(BMP580_ODR odr) {
  _odr = odr & 0x1F;
  // Preserve deep_dis and pwr_mode
  uint8_t reg = readRegister(BMP580_ODR_CONFIG);
  reg = (reg & (ODR_CONFIG_DEEP_DIS | ODR_CONFIG_MODE_MASK)) | (_odr << 2);
  writeRegister(BMP580_ODR_CONFIG, reg);
  _cacheValid = false;
  updateTimeoutUs();
}

void BMP580::setPowerMode(BMP580_Mode mode) {
  // Mode transitions are only allowed via STANDBY (datasheet 4.3 / 4.3.7)
  enterStandby();
  _mode = mode & ODR_CONFIG_MODE_MASK;

  if (_mode == BMP580_MODE_NORMAL || _mode == BMP580_MODE_CONTINUOUS) {
    writeRegister(BMP580_ODR_CONFIG, (_deepDis ? ODR_CONFIG_DEEP_DIS : 0) | (_odr << 2) | _mode);
  } else {
    // STANDBY, or FORCED: measurements are triggered on demand by
    // takeForcedMeasurement() / readTemperature() / readPressure().
    writeRegister(BMP580_ODR_CONFIG, (_deepDis ? ODR_CONFIG_DEEP_DIS : 0) | (_odr << 2) | BMP580_MODE_STANDBY);
  }
  _cacheValid = false;
  updateTimeoutUs();
}

void BMP580::setIIRFilter(BMP580_IIR iir_p, BMP580_IIR iir_t) {
  // DSP_IIR and DSP_CONFIG can only be written in STANDBY (datasheet 4.3.8),
  // writes in other modes are silently lost.
  uint8_t prev = enterStandby();

  // DSP_IIR (0x31): set_iir_p = bits 5:3, set_iir_t = bits 2:0, bits 7:6 = 0
  writeRegister(BMP580_DSP_IIR, ((iir_p & 0x07) << 3) | (iir_t & 0x07));

  // DSP_CONFIG (0x30): route the filtered values to the data registers and
  // FIFO. By default (reset value) the data registers show UNFILTERED data!
  uint8_t dsp = readRegister(BMP580_DSP_CONFIG);
  dsp &= ~(DSP_CONFIG_SHDW_T | DSP_CONFIG_FIFO_T | DSP_CONFIG_SHDW_P | DSP_CONFIG_FIFO_P);
  if (iir_t != BMP580_IIR_OFF) dsp |= DSP_CONFIG_SHDW_T | DSP_CONFIG_FIFO_T;
  if (iir_p != BMP580_IIR_OFF) dsp |= DSP_CONFIG_SHDW_P | DSP_CONFIG_FIFO_P;
  writeRegister(BMP580_DSP_CONFIG, dsp);

  restoreMode(prev);
}

void BMP580::setDeepStandby(bool enable) {
  uint8_t prev = enterStandby();
  _deepDis = !enable;
  restoreMode(prev);
}

bool BMP580::isConfigValid() {
  return (readRegister(BMP580_OSR_EFF) & 0x80) != 0;
}

void BMP580::getEffectiveOSR(BMP580_OSR &osr_p, BMP580_OSR &osr_t) {
  uint8_t reg = readRegister(BMP580_OSR_EFF);
  osr_p = (BMP580_OSR)((reg >> 3) & 0x07);
  osr_t = (BMP580_OSR)(reg & 0x07);
}

uint32_t BMP580::getMeasurementTimeUs() const {
  // Linear fit of datasheet table 4 (tconv_p / tconv_t), +5 % tolerance:
  //   tconv_p ~ 0.4 ms + 0.625 ms * OSR_P
  //   tconv_t ~ 0.8 ms + 0.156 ms * OSR_T
  uint32_t nP = 1UL << _osrP;
  uint32_t nT = 1UL << _osrT;
  uint32_t t = 800UL + 156UL * nT;
  if (_pressEn || _fifoFrame >= BMP580_FIFO_PRESSURE) {
    t += 400UL + 625UL * nP;
  }
  return t + t / 20;  // +5 %
}

// ---------------------------------------------------------------------------
// Cache logic & reading
// ---------------------------------------------------------------------------

void BMP580::updateTimeoutUs() {
  if (_mode == BMP580_MODE_CONTINUOUS || _mode == BMP580_MODE_FORCED) {
    // CONTINUOUS ignores the ODR and runs as fast as the OSR allows
    _cacheTimeoutUs = getMeasurementTimeUs();
    return;
  }

  float frequency = 240.0f;
  // Using a switch avoids storing a float table in RAM on 8-bit AVRs
  switch (_odr) {
    case BMP580_ODR_240Hz:   frequency = 240.0f;   break;
    case BMP580_ODR_218p5Hz: frequency = 218.537f; break;
    case BMP580_ODR_199p1Hz: frequency = 199.111f; break;
    case BMP580_ODR_179p2Hz: frequency = 179.2f;   break;
    case BMP580_ODR_160Hz:   frequency = 160.0f;   break;
    case BMP580_ODR_149p3Hz: frequency = 149.333f; break;
    case BMP580_ODR_140Hz:   frequency = 140.0f;   break;
    case BMP580_ODR_129p9Hz: frequency = 129.855f; break;
    case BMP580_ODR_120Hz:   frequency = 120.0f;   break;
    case BMP580_ODR_110p2Hz: frequency = 110.164f; break;
    case BMP580_ODR_100p3Hz: frequency = 100.299f; break;
    case BMP580_ODR_89p6Hz:  frequency = 89.6f;    break;
    case BMP580_ODR_80Hz:    frequency = 80.0f;    break;
    case BMP580_ODR_70Hz:    frequency = 70.0f;    break;
    case BMP580_ODR_60Hz:    frequency = 60.0f;    break;
    case BMP580_ODR_50p1Hz:  frequency = 50.056f;  break;
    case BMP580_ODR_45Hz:    frequency = 45.025f;  break;
    case BMP580_ODR_40Hz:    frequency = 40.0f;    break;
    case BMP580_ODR_35Hz:    frequency = 35.0f;    break;
    case BMP580_ODR_30Hz:    frequency = 30.0f;    break;
    case BMP580_ODR_25Hz:    frequency = 25.005f;  break;
    case BMP580_ODR_20Hz:    frequency = 20.0f;    break;
    case BMP580_ODR_15Hz:    frequency = 15.0f;    break;
    case BMP580_ODR_10Hz:    frequency = 10.0f;    break;
    case BMP580_ODR_5Hz:     frequency = 5.0f;     break;
    case BMP580_ODR_4Hz:     frequency = 4.0f;     break;
    case BMP580_ODR_3Hz:     frequency = 3.0f;     break;
    case BMP580_ODR_2Hz:     frequency = 2.0f;     break;
    case BMP580_ODR_1Hz:     frequency = 1.0f;     break;
    case BMP580_ODR_0p5Hz:   frequency = 0.5f;     break;
    case BMP580_ODR_0p25Hz:  frequency = 0.25f;    break;
    case BMP580_ODR_0p125Hz: frequency = 0.125f;   break;
  }
  // 1000000UL prevents 16-bit int overflow on AVR
  _cacheTimeoutUs = (unsigned long)(1000000UL / frequency);
}

float BMP580::convertTemperature(const uint8_t *raw) {
  // signed 24 bit, 16 fractional bits [degC]
  int32_t t = (int32_t)((uint32_t)raw[0] | ((uint32_t)raw[1] << 8) | ((uint32_t)raw[2] << 16));
  if (t & 0x800000L) t |= (int32_t)0xFF000000UL;  // sign extension
  return t / 65536.0f;
}

float BMP580::convertPressure(const uint8_t *raw) {
  // signed 24 bit, 6 fractional bits [Pa]
  int32_t p = (int32_t)((uint32_t)raw[0] | ((uint32_t)raw[1] << 8) | ((uint32_t)raw[2] << 16));
  if (p & 0x800000L) p |= (int32_t)0xFF000000UL;  // sign extension
  return p / 64.0f;
}

bool BMP580::updateCache() {
  uint8_t data[6];
  // A single burst read is required for data consistency (shadowing, 4.5.1)
  if (!readBurst(BMP580_TEMP_DATA_XLSB, data, 6)) {
    return false;
  }
  _cachedTemp = convertTemperature(&data[0]);
  _cachedPress = convertPressure(&data[3]);
  _lastReadTimeUs = micros();
  _cacheValid = true;
  return true;
}

bool BMP580::refreshIfStale() {
  // Unsigned subtraction is safe against the 71-minute micros() overflow
  if (_cacheValid && (micros() - _lastReadTimeUs) < _cacheTimeoutUs) {
    return true;
  }
  if (_mode == BMP580_MODE_FORCED) {
    return takeForcedMeasurement();
  }
  return updateCache();
}

bool BMP580::takeForcedMeasurement() {
  uint8_t reg = readRegister(BMP580_ODR_CONFIG);
  if ((reg & ODR_CONFIG_MODE_MASK) != BMP580_MODE_STANDBY) {
    writeRegister(BMP580_ODR_CONFIG, reg & ~ODR_CONFIG_MODE_MASK);
    delay(BMP580_T_STANDBY_MS);
  }
  writeRegister(BMP580_ODR_CONFIG, (reg & ~ODR_CONFIG_MODE_MASK) | BMP580_MODE_FORCED);

  uint32_t expectedMs = getMeasurementTimeUs() / 1000UL + 1;
  unsigned long start = millis();
  delay(expectedMs);

  // pwr_mode reads back the actual mode and returns to STANDBY when done
  while ((readRegister(BMP580_ODR_CONFIG) & ODR_CONFIG_MODE_MASK) != BMP580_MODE_STANDBY) {
    if (millis() - start > expectedMs + BMP580_T_STARTUP_MS + 20) {
      return false;
    }
    delayMicroseconds(200);
  }
  return updateCache();
}

bool BMP580::read(float &temperature, float &pressure) {
  bool ok = refreshIfStale();
  temperature = _cachedTemp;
  pressure = _cachedPress;
  return ok;
}

float BMP580::readTemperature() {
  refreshIfStale();
  return _cachedTemp;
}

float BMP580::readPressure() {
  refreshIfStale();
  return _cachedPress;
}

float BMP580::readAltitude(float seaLevelPressure) {
  float pressure = readPressure();  // uses smart cache
  // International barometric formula (ISA troposphere)
  return 44330.0f * (1.0f - pow(pressure / seaLevelPressure, 0.190295f));
}

float BMP580::seaLevelPressure(float altitude) {
  float pressure = readPressure();
  return pressure / pow(1.0f - altitude / 44330.0f, 5.255f);
}

// ---------------------------------------------------------------------------
// Interrupts
// ---------------------------------------------------------------------------

void BMP580::configureInterrupt(bool enable, bool latched, bool activeHigh, bool openDrain) {
  // Switching latched/pulsed must be done with all sources disabled
  // (datasheet 4.7.3.2): disable sources, clear status, set mode, restore.
  uint8_t sources = readRegister(BMP580_INT_SOURCE) & 0x0F;
  writeRegister(BMP580_INT_SOURCE, 0x00);
  readRegister(BMP580_INT_STATUS);

  uint8_t cfg = readRegister(BMP580_INT_CONFIG) & 0xF0;  // keep pad_int_drv
  if (latched)    cfg |= 0x01;
  if (activeHigh) cfg |= 0x02;
  if (openDrain)  cfg |= 0x04;
  if (enable)     cfg |= 0x08;
  writeRegister(BMP580_INT_CONFIG, cfg);

  writeRegister(BMP580_INT_SOURCE, sources);
}

void BMP580::setInterruptSource(uint8_t sources) {
  writeRegister(BMP580_INT_SOURCE, sources & 0x0F);
}

uint8_t BMP580::getInterruptStatus() {
  return readRegister(BMP580_INT_STATUS) & 0x1F;
}

bool BMP580::dataReady() {
  return (getInterruptStatus() & BMP580_INT_DRDY) != 0;
}

void BMP580::setPressureOutOfRange(uint32_t referencePa, uint8_t windowPa, BMP580_OORCount count) {
  // cnt_lim cannot be written during a conversion -> use STANDBY
  uint8_t prev = enterStandby();
  writeRegister(BMP580_OOR_THR_P_LSB, referencePa & 0xFF);
  writeRegister(BMP580_OOR_THR_P_MSB, (referencePa >> 8) & 0xFF);
  writeRegister(BMP580_OOR_RANGE, windowPa);
  writeRegister(BMP580_OOR_CONFIG, ((count & 0x03) << 6) | ((referencePa >> 16) & 0x01));
  restoreMode(prev);
}

// ---------------------------------------------------------------------------
// FIFO
// ---------------------------------------------------------------------------

void BMP580::configureFIFO(BMP580_FIFOFrame frame, uint8_t decimation, bool stopOnFull, uint8_t threshold) {
  // FIFO_SEL must only be changed in STANDBY (datasheet 4.6.4)
  uint8_t prev = enterStandby();
  _fifoFrame = frame & 0x03;

  uint8_t maxThs = (_fifoFrame == BMP580_FIFO_PRESS_AND_TEMP) ? 0x0F : 0x1F;
  if (threshold > maxThs) threshold = maxThs;

  writeRegister(BMP580_FIFO_CONFIG, (stopOnFull ? 0x20 : 0x00) | (threshold & 0x1F));
  writeRegister(BMP580_FIFO_SEL, ((decimation & 0x07) << 2) | _fifoFrame);

  restoreMode(prev);
  updateTimeoutUs();
}

uint8_t BMP580::getFIFOCount() {
  return readRegister(BMP580_FIFO_COUNT) & 0x3F;
}

uint8_t BMP580::readFIFO(float *pressure, float *temperature, uint8_t maxFrames) {
  if (_fifoFrame == BMP580_FIFO_DISABLED) return 0;

  uint8_t frameSize = (_fifoFrame == BMP580_FIFO_PRESS_AND_TEMP) ? 6 : 3;
  uint8_t available = getFIFOCount();
  if (available > maxFrames) available = maxFrames;

  uint8_t buffer[BMP580_I2C_CHUNK];
  uint8_t framesPerChunk = BMP580_I2C_CHUNK / frameSize;
  uint8_t done = 0;

  while (done < available) {
    uint8_t n = available - done;
    if (n > framesPerChunk) n = framesPerChunk;
    // The FIFO is read by bursting from the FIFO_DATA port; read whole frames
    if (!readBurst(BMP580_FIFO_DATA, buffer, n * frameSize)) break;

    for (uint8_t i = 0; i < n; i++) {
      const uint8_t *f = &buffer[i * frameSize];
      uint8_t idx = done + i;
      switch (_fifoFrame) {
        case BMP580_FIFO_PRESS_AND_TEMP:
          if (temperature) temperature[idx] = convertTemperature(f);
          if (pressure) pressure[idx] = convertPressure(f + 3);
          break;
        case BMP580_FIFO_TEMPERATURE:
          if (temperature) temperature[idx] = convertTemperature(f);
          break;
        case BMP580_FIFO_PRESSURE:
          if (pressure) pressure[idx] = convertPressure(f);
          break;
      }
    }
    done += n;
  }
  return done;
}

void BMP580::flushFIFO() {
  // A change of FIFO_SEL flushes the FIFO (datasheet 4.6.4)
  uint8_t prev = enterStandby();
  uint8_t sel = readRegister(BMP580_FIFO_SEL);
  writeRegister(BMP580_FIFO_SEL, sel & ~0x03);
  writeRegister(BMP580_FIFO_SEL, sel);
  restoreMode(prev);
}

// ---------------------------------------------------------------------------
// Identification / NVM
// ---------------------------------------------------------------------------

uint8_t BMP580::getChipID() {
  return readRegister(BMP580_CHIP_ID);
}

uint8_t BMP580::getRevisionID() {
  return readRegister(BMP580_REV_ID);
}

bool BMP580::waitNVMReady(uint16_t timeoutMs) {
  unsigned long start = millis();
  while (!(readRegister(BMP580_STATUS) & STATUS_NVM_RDY)) {
    if (millis() - start > timeoutMs) return false;
    delayMicroseconds(100);
  }
  return true;
}

bool BMP580::readNVM(uint8_t nvmAddr, uint16_t &data) {
  // Procedure from datasheet 4.8.1.1 (STANDBY, deep standby disabled)
  uint8_t prev = enterStandby();
  bool ok = false;

  if (waitNVMReady(10)) {
    writeRegister(BMP580_NVM_ADDR, nvmAddr & 0x3F);  // nvm_prog_en = 0
    writeRegister(BMP580_CMD, 0x5D);                  // individual transactions
    writeRegister(BMP580_CMD, 0xA5);
    if (waitNVMReady(10)) {
      uint8_t lsb = readRegister(BMP580_NVM_DATA_LSB);
      uint8_t msb = readRegister(BMP580_NVM_DATA_MSB);
      uint8_t status = readRegister(BMP580_STATUS);
      if (!(status & (STATUS_NVM_ERR | STATUS_NVM_CMD_ERR))) {
        data = ((uint16_t)msb << 8) | lsb;
        ok = true;
      }
    }
  }
  restoreMode(prev);
  return ok;
}

bool BMP580::writeUserNVM(uint8_t nvmAddr, uint16_t data) {
  if (nvmAddr < 0x20 || nvmAddr > 0x22) return false;  // user range only

  // Procedure from datasheet 4.8.1.2
  uint8_t prev = enterStandby();
  bool ok = false;

  if (waitNVMReady(10)) {
    writeRegister(BMP580_NVM_ADDR, 0x40 | nvmAddr);  // nvm_prog_en = 1
    writeRegister(BMP580_NVM_DATA_LSB, data & 0xFF);
    writeRegister(BMP580_NVM_DATA_MSB, data >> 8);
    writeRegister(BMP580_CMD, 0x5D);
    writeRegister(BMP580_CMD, 0xA0);
    if (waitNVMReady(50)) {  // ~10 ms
      uint8_t status = readRegister(BMP580_STATUS);
      ok = !(status & (STATUS_NVM_ERR | STATUS_NVM_CMD_ERR));
    }
    writeRegister(BMP580_NVM_ADDR, nvmAddr);  // nvm_prog_en = 0
  }
  restoreMode(prev);
  return ok;
}

uint64_t BMP580::getUniqueID() {
  uint16_t r23, r24, r25, r26;
  if (!readNVM(0x23, r23) || !readNVM(0x24, r24) || !readNVM(0x25, r25) || !readNVM(0x26, r26)) {
    return 0;
  }
  // Formula from datasheet 4.8.1.3
  return ((uint64_t)(r26 & 0xFF) << 40) | ((uint64_t)r25 << 24) | ((uint64_t)r24 << 8) |
         ((uint64_t)(r23 & 0xFF00) >> 8);
}

// ---------------------------------------------------------------------------
// Private helper functions
// ---------------------------------------------------------------------------

// Switches to STANDBY with deep standby disabled (so that all registers are
// writable) and returns the previous power mode.
uint8_t BMP580::enterStandby() {
  uint8_t reg = readRegister(BMP580_ODR_CONFIG);
  uint8_t mode = reg & ODR_CONFIG_MODE_MASK;
  if (mode != BMP580_MODE_STANDBY || !(reg & ODR_CONFIG_DEEP_DIS)) {
    writeRegister(BMP580_ODR_CONFIG, ODR_CONFIG_DEEP_DIS | (reg & ODR_CONFIG_ODR_MASK));
    delay(BMP580_T_STANDBY_MS);
  }
  return mode;
}

void BMP580::restoreMode(uint8_t mode) {
  // FORCED mode finishes in STANDBY - never re-trigger a measurement here
  if (mode == BMP580_MODE_FORCED) mode = BMP580_MODE_STANDBY;
  writeRegister(BMP580_ODR_CONFIG, (_deepDis ? ODR_CONFIG_DEEP_DIS : 0) | (_odr << 2) | mode);
  _cacheValid = false;
}

void BMP580::writeRegister(uint8_t reg, uint8_t value) {
  _wire->beginTransmission(_i2cAddr);
  _wire->write(reg);
  _wire->write(value);
  _wire->endTransmission();
}

uint8_t BMP580::readRegister(uint8_t reg) {
  uint8_t value = 0;
  readBurst(reg, &value, 1);
  return value;
}

bool BMP580::readBurst(uint8_t reg, uint8_t *buffer, uint8_t length) {
  _wire->beginTransmission(_i2cAddr);
  _wire->write(reg);
  if (_wire->endTransmission(false) != 0) {  // repeated start
    return false;
  }
  if (_wire->requestFrom(_i2cAddr, length) != length) {
    while (_wire->available()) _wire->read();
    return false;
  }
  for (uint8_t i = 0; i < length; i++) {
    buffer[i] = _wire->read();
  }
  return true;
}
