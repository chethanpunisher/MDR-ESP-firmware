#ifndef EEPROM_H
#define EEPROM_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

// Standardized EEPROM layout structure
typedef struct {
    // RTD calibration offsets (2 floats)
    float rtd_offset_dev1;
    float rtd_offset_dev2;
    
    // RTD temperature setpoints (2 floats)
    float rtd_temp_setpoint_dev1;
    float rtd_temp_setpoint_dev2;
    
    // MDR calibration constants (2 floats)
    float mdr_adc_zero;
    float mdr_k_t;
    
    // Reserved for future use (2 floats)
    float reserved1;
    float reserved2;
} eeprom_calibration_data_t;

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

// Standardized calibration data functions
esp_err_t EEPROM_SaveAllCalibrationData(const eeprom_calibration_data_t *data);
esp_err_t EEPROM_LoadAllCalibrationData(eeprom_calibration_data_t *data, uint8_t *isValid);

// Individual component save/load functions
esp_err_t EEPROM_SaveRTDCalibration(float offset_dev1, float offset_dev2);
esp_err_t EEPROM_SaveRTDTemperatureSetpoints(float setpoint_dev1, float setpoint_dev2);
esp_err_t EEPROM_SaveMDRCalibration(float adc_zero, float k_t);

#ifdef __cplusplus
}
#endif

#endif // EEPROM_H




