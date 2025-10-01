 #include "RTD_temp_svc.h"
 #include "freertos/FreeRTOS.h"
 #include "freertos/task.h"
 #include "config.h"
 #include "max31865.h"
 #include "balaji_infotech_machine_controller_v1.h"


/* Private variables */
static RTD_Temp_Handle_t rtd_handle;
static TaskHandle_t RTD_TaskHandle;

/* Private function prototypes */
static void RTD_Task(void *argument);

/* Function implementations */
void RTD_Temp_Init(void)
{
    /* Initialize MAX31865 */

    /* Create RTD task */
    xTaskCreate(RTD_Task, "RTD_Task", 5000, NULL, tskIDLE_PRIORITY+1, &RTD_TaskHandle);
}

float RTD_Temp_GetTemperature(uint8_t dev_num)
{
    if(dev_num == 1)
    {
        return (rtd_handle.current_temperature_dev1 - rtd_handle.known_temperature_dev1);
    }
    else if(dev_num == 2)
    {
        return (rtd_handle.current_temperature_dev2 - rtd_handle.known_temperature_dev2);
    }
    else
    {
        return 0;
    }
}

void RTD_Temp_Calibrate(uint8_t dev_num, float known_temp)
{
    if(dev_num == 1)
    {
        rtd_handle.known_temperature_dev1 = rtd_handle.current_temperature_dev1 - known_temp;
        UART_Printf("temp1Factor: %f\r\n", rtd_handle.known_temperature_dev1);
    }
    else if(dev_num == 2)
    {
        rtd_handle.known_temperature_dev2 = rtd_handle.current_temperature_dev2 - known_temp;
        UART_Printf("temp2Factor: %f\r\n", rtd_handle.known_temperature_dev2);
    }
    else
    {
        return;
    }
}

void RTD_Temp_SetTempSetPoint(float tempSetPoint)
{
    rtd_handle.tempSetPoint = tempSetPoint;
}

void RTD_Temp_SetFactor(uint8_t dev_num, float factor)
{
    if(dev_num == 1)
    {
        rtd_handle.known_temperature_dev1 = factor;
    }
    else if(dev_num == 2)
    {
        rtd_handle.known_temperature_dev2 = factor;
    }
    else
    {
        return;
    }
}

static void RTD_Task(void *argument)
{
    MAX31865_Init(&rtd_handle.max31865_dev1, g_rtd_spi, RTD_CS1_GPIO, MAX31865_PT100, MAX31865_3WIRE, MAX31865_50HZ);
    MAX31865_Init(&rtd_handle.max31865_dev2, g_rtd_spi, RTD_CS2_GPIO, MAX31865_PT100, MAX31865_3WIRE, MAX31865_50HZ);
    
    // Initialize with default values
    rtd_handle.known_temperature_dev1 = 0;
    rtd_handle.known_temperature_dev2 = 0;
    rtd_handle.tempSetPoint = 180;
    
    // // Try to load saved values from flash
    // Temp_Data_t tempData;
    // if (Temp_Data_Load(&tempData))
    // {
    //     // If valid data exists in flash, use those values
    //     rtd_handle.tempSetPoint = tempData.tempSetPoint;
    //     rtd_handle.known_temperature_dev1 = tempData.temp1Calib;
    //     rtd_handle.known_temperature_dev2 = tempData.temp2Calib;
        
    //     UART_Printf("Loaded from flash - SetPoint: %.2f, Calib1: %.2f, Calib2: %.2f\r\n",
    //                tempData.tempSetPoint, tempData.temp1Calib, tempData.temp2Calib);
    // }
    // else
    // {
    //     UART_Printf("No valid temperature data in flash, using defaults\r\n");
    //     rtd_handle.tempSetPoint = 180;
    //     rtd_handle.known_temperature_dev1 = 0;
    //     rtd_handle.known_temperature_dev2 = 0;
    // }
    rtd_handle.tempSetPoint = 180;
    rtd_handle.known_temperature_dev1 = 0;
    rtd_handle.known_temperature_dev2 = 0;
    
    /* Infinite loop */
    for(;;)
    {
        /* Read temperature from MAX31865 */
        MAX31865_ReadTemperature(&rtd_handle.max31865_dev1, &rtd_handle.current_temperature_dev1);
        MAX31865_ReadTemperature(&rtd_handle.max31865_dev2, &rtd_handle.current_temperature_dev2);

        if((RTD_Temp_GetTemperature(1) >= rtd_handle.tempSetPoint) || (RTD_Temp_GetTemperature(1) <= 0))
        {
            Relay_SSR_SetSSR(1, SSR_OFF);
        }
        else
        {
            Relay_SSR_SetSSR(1, SSR_ON);
        }

        if((RTD_Temp_GetTemperature(2) >= rtd_handle.tempSetPoint) || (RTD_Temp_GetTemperature(2) <= 0))
        {
            Relay_SSR_SetSSR(2, SSR_OFF);
        }
        else
        {
            Relay_SSR_SetSSR(2, SSR_ON);
        }
        
        /* Wait for 1000ms */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
