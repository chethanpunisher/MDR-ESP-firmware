#include "max31865.h"
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

static inline void cs_low(gpio_num_t cs) { gpio_set_level(cs, 0); }
static inline void cs_high(gpio_num_t cs) { gpio_set_level(cs, 1); }

// Half-duplex helpers using 8 address bits like the reference example
static MAX31865_Status_t max31865_write_byte(MAX31865_Handle_t *hmax, uint8_t reg, uint8_t data)
{
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.addr = (uint32_t)(reg | 0x80); // write bit set
    t.length = 8;
    t.flags = SPI_TRANS_USE_TXDATA;
    t.tx_data[0] = data;
    cs_low(hmax->cs_gpio);
    esp_err_t ret = spi_device_polling_transmit(hmax->spi, &t);
    cs_high(hmax->cs_gpio);
    return (ret == ESP_OK) ? MAX31865_OK : MAX31865_ERROR_SPI;
}

static MAX31865_Status_t max31865_read_byte(MAX31865_Handle_t *hmax, uint8_t reg, uint8_t *data)
{
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.addr = (uint32_t)(reg & 0x7F); // read
    t.rxlength = 8;
    t.flags = SPI_TRANS_USE_RXDATA;
    cs_low(hmax->cs_gpio);
    esp_err_t ret = spi_device_polling_transmit(hmax->spi, &t);
    cs_high(hmax->cs_gpio);
    if (ret != ESP_OK) return MAX31865_ERROR_SPI;
    *data = t.rx_data[0];
    return MAX31865_OK;
}

static MAX31865_Status_t readRTD(MAX31865_Handle_t *hmax, uint16_t *rtd)
{
    (void)MAX31865_ClearFault(hmax);
    (void)MAX31865_EnableBias(hmax, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    
    uint8_t config;
    (void)MAX31865_GetConfig(hmax, &config);
    config |= MAX31865_CONFIG_1SHOT;
    (void)MAX31865_SetConfig(hmax, config);
    vTaskDelay(pdMS_TO_TICKS(65));
    
    uint8_t msb, lsb;
    if (max31865_read_byte(hmax, MAX31865_RTD_MSB_REG, &msb) != MAX31865_OK) return MAX31865_ERROR_SPI;
    if (max31865_read_byte(hmax, MAX31865_RTD_LSB_REG, &lsb) != MAX31865_OK) return MAX31865_ERROR_SPI;

    *rtd = (uint16_t)((msb << 8) | lsb);
    *rtd >>= 1;
    (void)MAX31865_EnableBias(hmax, 0);
    return MAX31865_OK;
}

static float calculateTemperature(uint16_t RTDraw, float RTDnominal, float refResistor)
{
    float Z1, Z2, Z3, Z4, Rt, temp;

    Rt = RTDraw;
    Rt /= 32768.0f;
    Rt *= refResistor;

    Z1 = -MAX31865_RTD_A;
    Z2 = MAX31865_RTD_A * MAX31865_RTD_A - (4.0f * MAX31865_RTD_B);
    Z3 = (4.0f * MAX31865_RTD_B) / RTDnominal;
    Z4 = 2.0f * MAX31865_RTD_B;

    temp = Z2 + (Z3 * Rt);
    temp = (sqrtf(temp) + Z1) / Z4;

    if (temp >= 0.0f) {
        return temp;
    }

    Rt /= RTDnominal;
    Rt *= 100.0f;
    float rpoly = Rt;

    temp = -242.02f;
    temp += 2.2228f * rpoly;
    rpoly *= Rt;
    temp += 2.5859e-3f * rpoly;
    rpoly *= Rt;
    temp -= 4.8260e-6f * rpoly;
    rpoly *= Rt;
    temp -= 2.8183e-8f * rpoly;
    rpoly *= Rt;
    temp += 1.5243e-10f * rpoly;
    return temp;
}

MAX31865_Status_t MAX31865_Init(MAX31865_Handle_t *hmax, spi_device_handle_t spi, gpio_num_t cs_gpio, MAX31865_RTDType_t rtd_type, MAX31865_Wire_t wires, MAX31865_Filter_t filter)
{
    if (hmax == NULL) {
        return MAX31865_ERROR_INVALID_PARAM;
    }
    hmax->spi = spi;
    hmax->cs_gpio = cs_gpio;
    hmax->rtd_type = rtd_type;
    hmax->wires = wires;
    hmax->filter = filter;

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << cs_gpio),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    cs_high(cs_gpio);

    (void)MAX31865_SetWires(hmax, wires);
    (void)MAX31865_EnableBias(hmax, 0);
    (void)MAX31865_AutoConvert(hmax, 0);
    (void)MAX31865_WriteRegister(hmax, MAX31865_HIGH_FAULT_MSB_REG, 0xFF);
    (void)MAX31865_WriteRegister(hmax, MAX31865_HIGH_FAULT_LSB_REG, 0xFF);
    (void)MAX31865_WriteRegister(hmax, MAX31865_LOW_FAULT_MSB_REG, 0x00);
    (void)MAX31865_WriteRegister(hmax, MAX31865_LOW_FAULT_LSB_REG, 0x00);
    (void)MAX31865_ClearFault(hmax);
    (void)MAX31865_SetFilter(hmax, filter);
    return MAX31865_OK;
}

MAX31865_Status_t MAX31865_WriteRegister(MAX31865_Handle_t *hmax, uint8_t reg, uint8_t data)
{
    if (hmax == NULL) {
        return MAX31865_ERROR_INVALID_PARAM;
    }
    return max31865_write_byte(hmax, reg, data);
}

MAX31865_Status_t MAX31865_ReadRegister(MAX31865_Handle_t *hmax, uint8_t reg, uint8_t *data)
{
    if (hmax == NULL || data == NULL) {
        return MAX31865_ERROR_INVALID_PARAM;
    }
    return max31865_read_byte(hmax, reg, data);
}

MAX31865_Status_t MAX31865_SetConfig(MAX31865_Handle_t *hmax, uint8_t config)
{
    return MAX31865_WriteRegister(hmax, MAX31865_CONFIG_REG, config);
}

MAX31865_Status_t MAX31865_GetConfig(MAX31865_Handle_t *hmax, uint8_t *config)
{
    return MAX31865_ReadRegister(hmax, MAX31865_CONFIG_REG, config);
}

MAX31865_Status_t MAX31865_EnableBias(MAX31865_Handle_t *hmax, uint8_t enable)
{
    uint8_t config;
    MAX31865_Status_t status = MAX31865_GetConfig(hmax, &config);
    if (status != MAX31865_OK) return status;
    if (enable) config |= MAX31865_CONFIG_BIAS; else config &= (uint8_t)~MAX31865_CONFIG_BIAS;
    return MAX31865_SetConfig(hmax, config);
}

MAX31865_Status_t MAX31865_AutoConvert(MAX31865_Handle_t *hmax, uint8_t enable)
{
    uint8_t config;
    MAX31865_Status_t status = MAX31865_GetConfig(hmax, &config);
    if (status != MAX31865_OK) return status;
    if (enable) config |= MAX31865_CONFIG_AUTO_CONV; else config &= (uint8_t)~MAX31865_CONFIG_AUTO_CONV;
    return MAX31865_SetConfig(hmax, config);
}

MAX31865_Status_t MAX31865_SetWires(MAX31865_Handle_t *hmax, MAX31865_Wire_t wires)
{
    uint8_t config;
    MAX31865_Status_t status = MAX31865_GetConfig(hmax, &config);
    if (status != MAX31865_OK) return status;
    if (wires == MAX31865_3WIRE) config |= MAX31865_CONFIG_3WIRE; else config &= (uint8_t)~MAX31865_CONFIG_3WIRE;
    return MAX31865_SetConfig(hmax, config);
}

MAX31865_Status_t MAX31865_SetFilter(MAX31865_Handle_t *hmax, MAX31865_Filter_t filter)
{
    uint8_t config;
    MAX31865_Status_t status = MAX31865_GetConfig(hmax, &config);
    if (status != MAX31865_OK) return status;
    if (filter == MAX31865_50HZ) config |= MAX31865_CONFIG_50HZ; else config &= (uint8_t)~MAX31865_CONFIG_50HZ;
    return MAX31865_SetConfig(hmax, config);
}

MAX31865_Status_t MAX31865_OneShot(MAX31865_Handle_t *hmax)
{
    uint8_t config;
    MAX31865_Status_t status = MAX31865_GetConfig(hmax, &config);
    if (status != MAX31865_OK) return status;
    config |= MAX31865_CONFIG_1SHOT;
    return MAX31865_SetConfig(hmax, config);
}

MAX31865_Status_t MAX31865_ReadTemperature(MAX31865_Handle_t *hmax, float *temperature)
{
    if (hmax == NULL || temperature == NULL) {
        return MAX31865_ERROR_INVALID_PARAM;
    }
    uint16_t rtd;
    MAX31865_Status_t status = readRTD(hmax, &rtd);
    if (status != MAX31865_OK) return status;
    *temperature = calculateTemperature(rtd, (float)hmax->rtd_type, MAX31865_REF_RESISTOR);
    return MAX31865_OK;
}

MAX31865_Status_t MAX31865_ReadFault(MAX31865_Handle_t *hmax, uint8_t *fault)
{
    if (hmax == NULL || fault == NULL) {
        return MAX31865_ERROR_INVALID_PARAM;
    }
    return MAX31865_ReadRegister(hmax, MAX31865_FAULT_STATUS_REG, fault);
}

MAX31865_Status_t MAX31865_ClearFault(MAX31865_Handle_t *hmax)
{
    if (hmax == NULL) {
        return MAX31865_ERROR_INVALID_PARAM;
    }
    uint8_t config;
    if (MAX31865_ReadRegister(hmax, MAX31865_CONFIG_REG, &config) != MAX31865_OK) {
        return MAX31865_ERROR_SPI;
    }
    config |= MAX31865_CONFIG_FAULT_CLEAR;
    return MAX31865_WriteRegister(hmax, MAX31865_CONFIG_REG, config);
} 