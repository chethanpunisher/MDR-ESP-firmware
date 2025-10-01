#include "load_cell_svc.h"
#include "balaji_infotech_machine_controller_v1.h"
#include "config.h"
#include "RTD_temp_svc.h"
#include "hx711Config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Private variables */
static LoadCell_Handle_t loadCell;
static TaskHandle_t loadCellTaskHandle;

/* Private function prototypes */
static void LoadCellTask_Function(void *argument);

/**
  * @brief  Initialize the load cell
  * @retval None
  */
void LoadCell_Init(void)
{
    /* Wait for system to stabilize */
    // HAL_Delay(100);
    
    // Initialize HX711 with GPIO pins (ESP32)
    hx711_init(&loadCell.hx711, (gpio_num_t)CONFIG_HX711_CLK_GPIO, (gpio_num_t)CONFIG_HX711_DAT_GPIO);
    
    /* Wait for HX711 to stabilize */
    // HAL_Delay(100);
    
    /* Set default calibration factor */
    loadCell.calibration_factor = 200.0f;
    
    /* Power cycle HX711 before setting coefficient */
    // hx711_power_down(&loadCell.hx711);
    // HAL_Delay(10);
    // hx711_power_up(&loadCell.hx711);
    // HAL_Delay(10);
    
    /* Set coefficient after power cycle */
    hx711_coef_set(&loadCell.hx711, loadCell.calibration_factor);
    LoadCell_Tare();
    
    /* Create load cell task */
    if (xTaskCreate(LoadCellTask_Function, "LoadCellTask", 5120, NULL, tskIDLE_PRIORITY+1, &loadCellTaskHandle) != pdPASS) {
        UART_Printf("Load Cell Task Creation Failed\r\n");
        // fallthrough
    }
    
    // UART_Printf("Load Cell Initialized\r\n");
}

/**
  * @brief  Get current weight reading
  * @retval float Current weight in grams
  */
float LoadCell_GetWeight(void)
{
    return loadCell.current_weight;
}

/**
  * @brief  Tare the load cell
  * @retval None
  */
void LoadCell_Tare(void)
{
    hx711_tare(&loadCell.hx711, 10);
    loadCell.tare_weight = hx711_weight(&loadCell.hx711, 10);
    UART_Printf("Load Cell Tared\r\n");
}

/**
  * @brief  Calibrate the load cell with known weight
  * @param  known_weight: Known weight in grams
  * @retval None
  */
void LoadCell_Calibrate(float known_weight)
{    
    int32_t raw_value = hx711_value_ave(&loadCell.hx711, 10);
    loadCell.calibration_factor = (float)raw_value / known_weight;
    hx711_coef_set(&loadCell.hx711, loadCell.calibration_factor);
    UART_Printf("Load Cell Calibrated\r\n");
}

/**
  * @brief  Load cell task function
  * @param  argument: Not used
  * @retval None
  */
static void LoadCellTask_Function(void *argument)
{
    /* Initial delay to ensure system is stable */
    vTaskDelay(pdMS_TO_TICKS(100));
    
    for(;;)
    {
        /* Read weight every 1000ms */
        loadCell.current_weight = hx711_weight(&loadCell.hx711, 1);
        UART_Printf("mode%d : %.2f, temp1: %.2f, temp2: %.2f\r\n", mode, loadCell.current_weight, RTD_Temp_GetTemperature(1), RTD_Temp_GetTemperature(2));
        // UART_Printf("loadcell started\r\n");
        vTaskDelay(pdMS_TO_TICKS(16));
    }
}
