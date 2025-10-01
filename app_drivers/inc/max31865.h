#ifndef MAX31865_H
#define MAX31865_H

#include <stdint.h>
#include "esp_err.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

/* Register definitions */
#define MAX31865_CONFIG_REG          0x00
#define MAX31865_RTD_MSB_REG         0x01
#define MAX31865_RTD_LSB_REG         0x02
#define MAX31865_HIGH_FAULT_MSB_REG  0x03
#define MAX31865_HIGH_FAULT_LSB_REG  0x04
#define MAX31865_LOW_FAULT_MSB_REG   0x05
#define MAX31865_LOW_FAULT_LSB_REG   0x06
#define MAX31865_FAULT_STATUS_REG    0x07

/* Configuration register bits */
#define MAX31865_CONFIG_BIAS         0x80
#define MAX31865_CONFIG_AUTO_CONV    0x40
#define MAX31865_CONFIG_1SHOT        0x20
#define MAX31865_CONFIG_3WIRE        0x10
#define MAX31865_CONFIG_FAULT_CLEAR  0x02
#define MAX31865_CONFIG_50HZ         0x01

/* RTD constants */
#define MAX31865_RTD_A 3.9083e-3
#define MAX31865_RTD_B -5.775e-7
#define MAX31865_REF_RESISTOR 430.0f

/* Status codes */
typedef enum {
    MAX31865_OK = 0,
    MAX31865_ERROR_INVALID_PARAM = -1,
    MAX31865_ERROR_SPI = -2
} MAX31865_Status_t;

/* RTD types */
typedef enum {
    MAX31865_PT100 = 100,
    MAX31865_PT1000 = 1000
} MAX31865_RTDType_t;

/* Wire configuration */
typedef enum {
    MAX31865_2WIRE = 0,
    MAX31865_3WIRE = 1,
    MAX31865_4WIRE = 0
} MAX31865_Wire_t;

/* Filter frequency */
typedef enum {
    MAX31865_60HZ = 0,
    MAX31865_50HZ = 1
} MAX31865_Filter_t;

/* Handle structure (ESP-IDF) */
typedef struct {
    spi_device_handle_t spi;
    gpio_num_t cs_gpio;
    MAX31865_RTDType_t rtd_type;
    MAX31865_Wire_t wires;
    MAX31865_Filter_t filter;
} MAX31865_Handle_t;

/* Function prototypes */
MAX31865_Status_t MAX31865_Init(MAX31865_Handle_t *hmax, spi_device_handle_t spi, gpio_num_t cs_gpio, MAX31865_RTDType_t rtd_type, MAX31865_Wire_t wires, MAX31865_Filter_t filter);
MAX31865_Status_t MAX31865_WriteRegister(MAX31865_Handle_t *hmax, uint8_t reg, uint8_t data);
MAX31865_Status_t MAX31865_ReadRegister(MAX31865_Handle_t *hmax, uint8_t reg, uint8_t *data);
MAX31865_Status_t MAX31865_SetConfig(MAX31865_Handle_t *hmax, uint8_t config);
MAX31865_Status_t MAX31865_GetConfig(MAX31865_Handle_t *hmax, uint8_t *config);
MAX31865_Status_t MAX31865_EnableBias(MAX31865_Handle_t *hmax, uint8_t enable);
MAX31865_Status_t MAX31865_AutoConvert(MAX31865_Handle_t *hmax, uint8_t enable);
MAX31865_Status_t MAX31865_SetWires(MAX31865_Handle_t *hmax, MAX31865_Wire_t wires);
MAX31865_Status_t MAX31865_SetFilter(MAX31865_Handle_t *hmax, MAX31865_Filter_t filter);
MAX31865_Status_t MAX31865_OneShot(MAX31865_Handle_t *hmax);
MAX31865_Status_t MAX31865_ReadTemperature(MAX31865_Handle_t *hmax, float *temperature);
MAX31865_Status_t MAX31865_ReadFault(MAX31865_Handle_t *hmax, uint8_t *fault);
MAX31865_Status_t MAX31865_ClearFault(MAX31865_Handle_t *hmax);

#endif /* MAX31865_H */