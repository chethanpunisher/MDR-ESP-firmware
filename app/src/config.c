#include "config.h"
#include "esp_log.h"
#include <stdio.h>
#include <stdarg.h>

uint8_t mode = 0;
uint32_t offTime = 60; // Default to 60 seconds
uint8_t triggerFlg = 0;

void UART_Printf(const char *format, ...)
{
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    ESP_LOGI("UART", "%s", buffer);
}


