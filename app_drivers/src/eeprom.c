/* Reference implementation restored */
#include "eeprom.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "sdkconfig.h"
#ifndef CONFIG_I2C_MASTER_FREQUENCY
#define CONFIG_I2C_MASTER_FREQUENCY 100000
#endif

// ESP-IDF uses 7-bit I2C address
#define EEPROM_I2C_ADDR          (0x50U)

// Calibration storage layout (variable length)
// 0x0400: flag (0x01 = present)
// 0x0401: count (N)
// 0x0402.. : N floats
#define CALIB_FLAG_ADDR          (0x0400U)
#define CALIB_COUNT_ADDR         (CALIB_FLAG_ADDR + 1U)
#define CALIB_DATA_ADDR          (CALIB_FLAG_ADDR + 2U)
#define CALIB_DONE_IDENTIFIER    0x02

// Conservative EEPROM page size for paged writes (adjust to your EEPROM)
#define EEPROM_PAGE_SIZE         (32U)

#define I2C_MASTER_TIMEOUT_MS    1000

static const char *TAG = "eeprom";

static i2c_master_dev_handle_t s_eeprom_dev = NULL;

esp_err_t EEPROM_Init(i2c_master_bus_handle_t bus_handle)
{
    if (s_eeprom_dev != NULL) {
        return ESP_OK;
    }
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = EEPROM_I2C_ADDR,
        .scl_speed_hz = CONFIG_I2C_MASTER_FREQUENCY,
    };
    return i2c_master_bus_add_device(bus_handle, &dev_config, &s_eeprom_dev);
}

esp_err_t EEPROM_Deinit(void)
{
    if (s_eeprom_dev != NULL) {
        i2c_master_bus_rm_device(s_eeprom_dev);
        s_eeprom_dev = NULL;
    }
    return ESP_OK;
}

esp_err_t EEPROM_ReadConfigReg(uint8_t *data, uint16_t len)
{
    uint8_t addr_bytes[2];
    addr_bytes[0] = 0x80;   // Bit7=1 selects config register (device-specific)
    addr_bytes[1] = 0x00;   // Don't care
    return i2c_master_transmit_receive(s_eeprom_dev,
                                       addr_bytes, 2,
                                       data, len,
                                       I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

esp_err_t EEPROM_WriteConfigReg(uint8_t wpr, uint8_t har, uint8_t sendHar)
{
    uint8_t buffer[4];
    uint8_t len = 0;
    buffer[len++] = 0x80;   // config space
    buffer[len++] = 0x00;
    buffer[len++] = wpr;
    if (sendHar) {
        buffer[len++] = har;
    }
    return i2c_master_transmit(s_eeprom_dev, buffer, len, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

esp_err_t EEPROM_WriteData(uint8_t *addr, const uint8_t *data, uint16_t addr_len, uint16_t data_len)
{
    uint8_t buffer[2 + EEPROM_PAGE_SIZE];
    (void)addr_len; // we only support 2-byte addressing here
    buffer[0] = addr[0];
    buffer[1] = addr[1];

    // If data fits in one page, send directly; otherwise delegate to paged
    if (data_len <= EEPROM_PAGE_SIZE)
    {
        memcpy(&buffer[2], data, data_len);
        esp_err_t err = i2c_master_transmit(s_eeprom_dev, buffer, (size_t)(2 + data_len), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
        if (err == ESP_OK) {
            vTaskDelay(pdMS_TO_TICKS(5));
        } else {
            ESP_LOGE(TAG, "EEPROM_WriteData: TX fail %d", (int)err);
        }
        return err;
    }

    // Fallback to paged write
    uint16_t base = ((uint16_t)addr[0] << 8) | addr[1];
    uint16_t remaining = data_len;
    const uint8_t *p = data;
    while (remaining)
    {
        uint16_t page_off = base % EEPROM_PAGE_SIZE;
        uint16_t space = EEPROM_PAGE_SIZE - page_off;
        uint16_t chunk = (remaining < space) ? remaining : space;

        buffer[0] = (uint8_t)((base >> 8) & 0xFF);
        buffer[1] = (uint8_t)(base & 0xFF);
        memcpy(&buffer[2], p, chunk);

        esp_err_t err = i2c_master_transmit(s_eeprom_dev, buffer, (size_t)(2 + chunk), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "EEPROM_WriteData (paged): TX fail %d", (int)err);
            return err;
        }
        vTaskDelay(pdMS_TO_TICKS(5));

        base += chunk; p += chunk; remaining -= chunk;
    }
    return ESP_OK;
}

esp_err_t EEPROM_ReadData(uint8_t *addr, uint8_t *data, uint16_t addr_len, uint16_t data_len)
{
    uint8_t buffer[2];
    (void)addr_len; // 2-byte addressing
    buffer[0] = addr[0];
    buffer[1] = addr[1];
    return i2c_master_transmit_receive(s_eeprom_dev,
                                       buffer, 2,
                                       data, data_len,
                                       I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

// --- Calibration helpers ---

// --- 64-byte helper implementations ---
esp_err_t EEPROM_Write64(uint8_t *addr, const uint8_t *data)
{
    // Write 32 + 32 bytes starting at addr and addr+32
    esp_err_t err = EEPROM_WriteData(addr, &data[0], 2, 32);
    if (err != ESP_OK) return err;
    uint16_t base = ((uint16_t)addr[0] << 8) | addr[1];
    uint16_t base2 = (uint16_t)(base + 32);
    uint8_t addr2[2] = { (uint8_t)(base2 >> 8), (uint8_t)(base2 & 0xFF) };
    return EEPROM_WriteData(addr2, &data[32], 2, 32);
}

esp_err_t EEPROM_Read64(uint8_t *addr, uint8_t *data)
{
    esp_err_t err = EEPROM_ReadData(addr, &data[0], 2, 32);
    if (err != ESP_OK) return err;
    uint16_t base = ((uint16_t)addr[0] << 8) | addr[1];
    uint16_t base2 = (uint16_t)(base + 32);
    uint8_t addr2[2] = { (uint8_t)(base2 >> 8), (uint8_t)(base2 & 0xFF) };
    return EEPROM_ReadData(addr2, &data[32], 2, 32);
}

esp_err_t EEPROM_Write64Split(uint8_t *addr_first, uint8_t *addr_second, const uint8_t *data)
{
    esp_err_t err = EEPROM_WriteData(addr_first, &data[0], 2, 32);
    if (err != ESP_OK) return err;
    vTaskDelay(pdMS_TO_TICKS(10));
    return EEPROM_WriteData(addr_second, &data[32], 2, 32);
}

esp_err_t EEPROM_Read64Split(uint8_t *addr_first, uint8_t *addr_second, uint8_t *data)
{
    esp_err_t err = EEPROM_ReadData(addr_first, &data[0], 2, 32);
    if (err != ESP_OK) return err;
    vTaskDelay(pdMS_TO_TICKS(10));
    return EEPROM_ReadData(addr_second, &data[32], 2, 32);
}

static esp_err_t EEPROM_WriteBytesPaged(uint16_t start_addr, const uint8_t *data, uint16_t len)
{
    uint16_t remaining = len;
    uint16_t current_addr = start_addr;
    const uint8_t *current_ptr = data;
    uint8_t buffer[2 + EEPROM_PAGE_SIZE];

    while (remaining > 0)
    {
        uint16_t page_offset = current_addr % EEPROM_PAGE_SIZE;
        uint16_t space_in_page = EEPROM_PAGE_SIZE - page_offset;
        uint16_t chunk = (remaining < space_in_page) ? remaining : space_in_page;

        buffer[0] = (uint8_t)((current_addr >> 8) & 0xFF);
        buffer[1] = (uint8_t)(current_addr & 0xFF);
        memcpy(&buffer[2], current_ptr, chunk);

        esp_err_t err = i2c_master_transmit(s_eeprom_dev, buffer, (size_t)(2 + chunk), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
        if (err != ESP_OK)
        {
            return err;
        }
        vTaskDelay(pdMS_TO_TICKS(5));

        current_addr += chunk;
        current_ptr  += chunk;
        remaining    -= chunk;
    }
    return ESP_OK;
}

esp_err_t Calibration_Save(const float constants[6])
{
    // Back-compat fixed-6 API delegates to N=6 implementation
    return Calibration_SaveN(constants, 6);
}

esp_err_t Calibration_Load(float constants[6], uint8_t *isValid)
{
    uint8_t n = 0;
    return Calibration_LoadN(constants, 6, &n, isValid);
}

uint8_t EEPROM_CalibrationPresent(void)
{
    uint8_t flag_addr[2] = { (uint8_t)((CALIB_FLAG_ADDR >> 8) & 0xFF), (uint8_t)(CALIB_FLAG_ADDR & 0xFF) };
    uint8_t flag_val = 0x00;
    if (EEPROM_ReadData(flag_addr, &flag_val, 2, 1) != ESP_OK) {
        return 0;
    }
    ESP_LOGD(TAG, "EEPROM_CalibrationPresent: flag_val = %d", flag_val);
    return (flag_val == CALIB_DONE_IDENTIFIER) ? 1U : 0U;
}

esp_err_t Calibration_SaveN(const float *constants, uint8_t count)
{
    // Build 64-byte blob: [0]=flag, [1]=count, [2..] floats (little endian)
    uint8_t blob[64] = {0};
    if (count > 14) { count = 14; } // 14 floats fit in remaining 62 bytes
    blob[0] = CALIB_DONE_IDENTIFIER;
    blob[1] = count;
    memcpy(&blob[2], constants, (size_t)count * sizeof(float));

    // Split addresses like working example
    uint8_t addr_first[2]  = { 0x00, 0x00 };
    uint8_t addr_second[2] = { 0x3F, 0x00 };
    return EEPROM_Write64Split(addr_first, addr_second, blob);
}

esp_err_t Calibration_LoadN(float *constants, uint8_t maxCount, uint8_t *outCount, uint8_t *isValid)
{
    if (isValid) { *isValid = 0; }
    if (outCount) { *outCount = 0; }

    uint8_t blob[64] = {0};
    uint8_t addr_first[2]  = { 0x00, 0x00 };
    uint8_t addr_second[2] = { 0x3F, 0x00 };
    esp_err_t err = EEPROM_Read64Split(addr_first, addr_second, blob);
    if (err != ESP_OK) { return err; }
    if (blob[0] != CALIB_DONE_IDENTIFIER) { return ESP_OK; }
    uint8_t count = blob[1];
    if (count == 0) { return ESP_OK; }

    uint8_t toCopy = (count <= maxCount) ? count : maxCount;
    memcpy(constants, &blob[2], (size_t)toCopy * sizeof(float));
    if (outCount) { *outCount = count; }
    if (isValid) { *isValid = 1; }
    return ESP_OK;
}

// --- Standardized calibration data functions ---

esp_err_t EEPROM_SaveAllCalibrationData(const eeprom_calibration_data_t *data)
{
    // Convert struct to byte array for 64-split write
    uint8_t blob[64] = {0};
    blob[0] = CALIB_DONE_IDENTIFIER;
    blob[1] = 8; // 8 floats in the structure
    
    // Copy the struct data (8 floats = 32 bytes)
    memcpy(&blob[2], data, sizeof(eeprom_calibration_data_t));
    
    // Use the working 64-split method
    uint8_t addr_first[2]  = { 0x00, 0x00 };
    uint8_t addr_second[2] = { 0x3F, 0x00 };
    return EEPROM_Write64Split(addr_first, addr_second, blob);
}

esp_err_t EEPROM_LoadAllCalibrationData(eeprom_calibration_data_t *data, uint8_t *isValid)
{
    if (isValid) { *isValid = 0; }
    if (!data) { return ESP_ERR_INVALID_ARG; }

    uint8_t blob[64] = {0};
    uint8_t addr_first[2]  = { 0x00, 0x00 };
    uint8_t addr_second[2] = { 0x3F, 0x00 };
    esp_err_t err = EEPROM_Read64Split(addr_first, addr_second, blob);
    if (err != ESP_OK) { return err; }
    
    // Check if calibration data is present
    if (blob[0] != CALIB_DONE_IDENTIFIER) { return ESP_OK; }
    uint8_t count = blob[1];
    if (count != 8) { return ESP_OK; } // Expect exactly 8 floats
    
    // Copy the data back to struct
    memcpy(data, &blob[2], sizeof(eeprom_calibration_data_t));
    if (isValid) { *isValid = 1; }
    return ESP_OK;
}

esp_err_t EEPROM_SaveRTDCalibration(float offset_dev1, float offset_dev2)
{
    eeprom_calibration_data_t data = {0};
    uint8_t isValid = 0;
    
    // Load existing data first
    EEPROM_LoadAllCalibrationData(&data, &isValid);
    
    // Update RTD calibration fields
    data.rtd_offset_dev1 = offset_dev1;
    data.rtd_offset_dev2 = offset_dev2;
    
    // Save updated data
    return EEPROM_SaveAllCalibrationData(&data);
}

esp_err_t EEPROM_SaveRTDTemperatureSetpoints(float setpoint_dev1, float setpoint_dev2)
{
    eeprom_calibration_data_t data = {0};
    uint8_t isValid = 0;
    
    // Load existing data first
    EEPROM_LoadAllCalibrationData(&data, &isValid);
    
    // Update RTD temperature setpoint fields
    data.rtd_temp_setpoint_dev1 = setpoint_dev1;
    data.rtd_temp_setpoint_dev2 = setpoint_dev2;
    
    // Save updated data
    return EEPROM_SaveAllCalibrationData(&data);
}

esp_err_t EEPROM_SaveMDRCalibration(float adc_zero, float k_t)
{
    eeprom_calibration_data_t data = {0};
    uint8_t isValid = 0;
    
    // Load existing data first
    EEPROM_LoadAllCalibrationData(&data, &isValid);
    
    // Update MDR calibration fields
    data.mdr_adc_zero = adc_zero;
    data.mdr_k_t = k_t;
    
    // Save updated data
    return EEPROM_SaveAllCalibrationData(&data);
}

