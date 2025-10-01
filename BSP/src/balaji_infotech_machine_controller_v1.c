#include "balaji_infotech_machine_controller_v1.h"
#include "esp_log.h"

spi_device_handle_t g_rtd_spi = NULL;
i2c_master_bus_handle_t g_i2c_bus = NULL;

static const char *TAG = "BSP";

static void init_spi(void)
{
    spi_bus_config_t buscfg = {
        .miso_io_num = CONFIG_RTD_SPI_MISO,
        .mosi_io_num = CONFIG_RTD_SPI_MOSI,
        .sclk_io_num = CONFIG_RTD_SPI_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32,
        .flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_MISO | SPICOMMON_BUSFLAG_MOSI
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_DISABLED));

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1000000,
        .mode = 1,
        .spics_io_num = -1, // manual CS
        .queue_size = 1,
        .flags = SPI_DEVICE_HALFDUPLEX,
        .command_bits = 0,
        .address_bits = 8,
        .dummy_bits = 0
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &g_rtd_spi));
}

static void init_i2c(void)
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = CONFIG_I2C_MASTER_SDA,
        .scl_io_num = CONFIG_I2C_MASTER_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &g_i2c_bus));
}

static void init_gpio_outputs(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << RELAY_1_GPIO) | (1ULL << RELAY_2_GPIO) |
                        (1ULL << RELAY_3_GPIO) | (1ULL << RELAY_4_GPIO) |
                        (1ULL << SSR_1_GPIO)   | (1ULL << SSR_2_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(RELAY_1_GPIO, 0);
    gpio_set_level(RELAY_2_GPIO, 0);
    gpio_set_level(RELAY_3_GPIO, 0);
    gpio_set_level(RELAY_4_GPIO, 0);
    gpio_set_level(SSR_1_GPIO, 0);
    gpio_set_level(SSR_2_GPIO, 0);
}

void BALAJI_Board_v1_Init(void)
{
    init_spi();
    init_i2c();
    init_gpio_outputs();
    ESP_LOGI(TAG, "BSP init complete");
}
