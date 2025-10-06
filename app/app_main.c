 #include "freertos/FreeRTOS.h"
 #include "freertos/task.h"
 #include "esp_log.h"
 #include "balaji_infotech_machine_controller_v1.h"
 #include "RTD_temp_svc.h"
 #include "Relay_SSR_svc.h"
 #include "eeprom.h"
 #include "comm_exec.h"
 #include "load_cell_svc.h"
 static const char *TAG = "app";

 void app_main(void)
 {
    BALAJI_Board_v1_Init();

    // Init I2C EEPROM (bus provided by BSP) BEFORE modules that load from it
    if (EEPROM_Init(g_i2c_bus) == ESP_OK) {
         ESP_LOGI(TAG, "EEPROM initialized");
     } else {
         ESP_LOGW(TAG, "EEPROM init failed");
     }
    // Initialize services
    RTD_Temp_Init();
    Relay_SSR_Init();
    LoadCell_Init();
     CommTask_Init();
     while (1) {
         vTaskDelay(pdMS_TO_TICKS(1000));
     }
 }