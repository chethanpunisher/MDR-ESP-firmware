 #ifndef BALAJI_INFOTECH_MACHINE_CONTROLLER_V1_H
 #define BALAJI_INFOTECH_MACHINE_CONTROLLER_V1_H

 #include <stdint.h>
 #include "esp_err.h"
 #include "driver/gpio.h"
 #include "driver/spi_master.h"
 #include "driver/i2c_master.h"

 /* SPI pins (match spi_master_example_main.c_reference) */
 #ifndef CONFIG_RTD_SPI_MISO
 #define CONFIG_RTD_SPI_MISO 26
 #endif
 #ifndef CONFIG_RTD_SPI_MOSI
 #define CONFIG_RTD_SPI_MOSI 25
 #endif
 #ifndef CONFIG_RTD_SPI_CLK
 #define CONFIG_RTD_SPI_CLK 27
 #endif
 #ifndef CONFIG_RTD_CS1
 #define CONFIG_RTD_CS1 32
 #endif
 #ifndef CONFIG_RTD_CS2
 #define CONFIG_RTD_CS2 33
 #endif

 /* I2C pins (match i2c_basic_example_main.c_reference) */
 #ifndef CONFIG_I2C_MASTER_SDA
 #define CONFIG_I2C_MASTER_SDA 21
 #endif
 #ifndef CONFIG_I2C_MASTER_SCL
 #define CONFIG_I2C_MASTER_SCL 22
 #endif
 #ifndef CONFIG_I2C_MASTER_FREQUENCY
 #define CONFIG_I2C_MASTER_FREQUENCY 400000
 #endif

 /* Relay/SSR pins (per user): relay1=5, relay2=14, relay3=13, SSR1=19, SSR2=23 */
 #ifndef CONFIG_RELAY1_GPIO
 #define CONFIG_RELAY1_GPIO 5
 #endif
 #ifndef CONFIG_RELAY2_GPIO
 #define CONFIG_RELAY2_GPIO 14
 #endif
 #ifndef CONFIG_RELAY3_GPIO
 #define CONFIG_RELAY3_GPIO 13
 #endif
#ifndef CONFIG_RELAY4_GPIO
#define CONFIG_RELAY4_GPIO 17
#endif
 #ifndef CONFIG_SSR1_GPIO
 #define CONFIG_SSR1_GPIO 19
 #endif
 #ifndef CONFIG_SSR2_GPIO
 #define CONFIG_SSR2_GPIO 23
 #endif

 /* Exported handles */
 extern spi_device_handle_t g_rtd_spi;
 extern i2c_master_bus_handle_t g_i2c_bus;

 /* Exported CS GPIOs */
 #define RTD_CS1_GPIO ((gpio_num_t)CONFIG_RTD_CS1)
 #define RTD_CS2_GPIO ((gpio_num_t)CONFIG_RTD_CS2)

 /* Relay/SSR GPIOs */
 #define RELAY_1_GPIO ((gpio_num_t)CONFIG_RELAY1_GPIO)
 #define RELAY_2_GPIO ((gpio_num_t)CONFIG_RELAY2_GPIO)
 #define RELAY_3_GPIO ((gpio_num_t)CONFIG_RELAY3_GPIO)
 #define RELAY_4_GPIO ((gpio_num_t)CONFIG_RELAY4_GPIO)
 #define SSR_1_GPIO   ((gpio_num_t)CONFIG_SSR1_GPIO)
 #define SSR_2_GPIO   ((gpio_num_t)CONFIG_SSR2_GPIO)

 /* Initialization */
 void BALAJI_Board_v1_Init(void);

 #endif /* BALAJI_INFOTECH_MACHINE_CONTROLLER_V1_H */
