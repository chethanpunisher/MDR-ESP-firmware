#ifndef EEPROM_H
#define EEPROM_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

// Public API (ESP-IDF)
// Initializes the EEPROM device (address 0x50) on a given I2C master bus.
// Must be called after the I2C master bus is created.
esp_err_t EEPROM_Init(i2c_master_bus_handle_t bus_handle);
esp_err_t EEPROM_Deinit(void);

esp_err_t EEPROM_ReadConfigReg(uint8_t *data, uint16_t len);
esp_err_t EEPROM_WriteConfigReg(uint8_t wpr, uint8_t har, uint8_t sendHar);

esp_err_t EEPROM_WriteData(uint8_t *addr, const uint8_t *data, uint16_t addr_len, uint16_t data_len);
esp_err_t EEPROM_ReadData(uint8_t *addr, uint8_t *data, uint16_t addr_len, uint16_t data_len);

esp_err_t Calibration_Save(const float constants[6]);
esp_err_t Calibration_Load(float constants[6], uint8_t *isValid);

// Variable-length calibration helpers (counted format)
esp_err_t Calibration_SaveN(const float *constants, uint8_t count);
esp_err_t Calibration_LoadN(float *constants, uint8_t maxCount, uint8_t *outCount, uint8_t *isValid);

// Returns 1 if calibration is known-present (after a successful load/save), else 0
uint8_t EEPROM_CalibrationPresent(void);

#ifdef __cplusplus
}
#endif

#endif // EEPROM_H




