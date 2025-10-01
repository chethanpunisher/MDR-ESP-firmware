 #include "comm_exec.h"
 #include <stdarg.h>
 #include <stdio.h>
 #include <string.h>
 #include <stdint.h>  // For uint16_t
 #include "esp_log.h"
#include "driver/uart.h"

/* Private variables */
TaskHandle_t CommTaskHandle;

// Command parsing disabled on ESP32 build for now

/* Private function prototypes */
static void CommTask_Function(void *argument);
// static void ParseCommand(char *cmd);

/**
  * @brief Initialize the communication task
  * @param None
  * @retval None
  */
void CommTask_Init(void)
{
  /* Configure UART0 for USB-UART bridge echo */
  const uart_config_t uart_config = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_APB,
  };
  uart_driver_install(UART_NUM_0, 2048, 2048, 0, NULL, 0);
  uart_param_config(UART_NUM_0, &uart_config);
  uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

  /* Create the task */
  xTaskCreate(CommTask_Function, "CommTask", 4096, NULL, tskIDLE_PRIORITY+1, &CommTaskHandle);
}

/**
  * @brief Start the communication task
  * @param None
  * @retval None
  */
void CommTask_Start(void)
{
  /* No-op on ESP32 */
}

/* UART_Printf is provided in config.c */

/**
  * @brief Parse incoming command
  * @param cmd: Command string to parse
  * @retval None
  */
/* Command parsing removed in ESP32 build. */

/**
  * @brief Communication task function
  * @param argument: Not used
  * @retval None
  */
static void CommTask_Function(void *argument)
{
  uint8_t rxbuf[256];
  for(;;) {
    int len = uart_read_bytes(UART_NUM_0, rxbuf, sizeof(rxbuf), pdMS_TO_TICKS(20));
    if (len > 0) {
      /* Echo back */
      uart_write_bytes(UART_NUM_0, (const char*)rxbuf, len);
      /* Also log */
      ESP_LOGI("UART", "RX %d bytes", len);
    }
  }
}
