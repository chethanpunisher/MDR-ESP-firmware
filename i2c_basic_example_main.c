/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* i2c - Simple Example

   Simple I2C example that shows how to initialize I2C
   as well as reading and writing from and to registers for a sensor connected over I2C.

   The sensor used in this example is a MPU9250 inertial measurement unit.
*/
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "eeprom.h"

static const char *TAG = "example";

#define I2C_MASTER_SCL_IO           CONFIG_I2C_MASTER_SCL       /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           CONFIG_I2C_MASTER_SDA       /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              I2C_NUM_0                   /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ          CONFIG_I2C_MASTER_FREQUENCY /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                           /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                           /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000

// EEPROM 24xx device address (7-bit)
// We use the driver abstraction, address is set there.

uint8_t addr[2] = { 0x00, 0x00 };
uint8_t addr2[2] = { 0x3F, 0x00 };
uint8_t tx64[64] = { 0 };
uint8_t rx64[64] = { 0 };
/**
 * @brief Read a sequence of bytes from a MPU9250 sensor registers
 */
// No direct register helpers needed for EEPROM example

/**
 * @brief i2c master bus initialization (no device added here)
 */
static void i2c_master_init(i2c_master_bus_handle_t *bus_handle)
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_MASTER_NUM,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, bus_handle));
}

void app_main(void)
{
    i2c_master_bus_handle_t bus_handle;
    i2c_master_init(&bus_handle);
    ESP_LOGI(TAG, "I2C initialized successfully");

    // Initialize EEPROM device on the created bus
    ESP_ERROR_CHECK(EEPROM_Init(bus_handle));

    // Write then read back a small pattern at address 0x0000

    for (int i = 0; i < 32; i++) {
        tx64[i] = i;
    }
    for (int i = 0; i < 32; i++) {
        tx64[32 + i] = (uint8_t)(i + 10);
    }

    ESP_ERROR_CHECK(EEPROM_Write64Split(addr, addr2, tx64));
    vTaskDelay(pdMS_TO_TICKS(10));

    ESP_ERROR_CHECK(EEPROM_Read64Split(addr, addr2, rx64));

    for (int i = 0; i < 64; i++) {
        ESP_LOGI(TAG, "EEPROM R/W %d: %02X", i, rx64[i]);
    }

    // ESP_ERROR_CHECK(EEPROM_ReadData(addr2, rx, 2, sizeof(rx)));
    // // ESP_LOGI(TAG, "EEPROM R/W: %02X %02X %02X %02X %02X %02X", rx[0], rx[1], rx[2], rx[3], rx[4], rx[5]);
    // for (int i = 0; i < 8; i++) {
    //     ESP_LOGI(TAG, "EEPROM R/W: %02X", rx[i]);
    // }
    // Clean up
    ESP_ERROR_CHECK(EEPROM_Deinit());
    ESP_ERROR_CHECK(i2c_del_master_bus(bus_handle));
    ESP_LOGI(TAG, "I2C de-initialized successfully");
}
