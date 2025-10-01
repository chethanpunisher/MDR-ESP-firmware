#include "Relay_SSR_svc.h"
#include "balaji_infotech_machine_controller_v1.h"
#include "config.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


/* Private variables */
static Relay_SSR_Handle_t relay_ssr;
static TaskHandle_t relaySSRTaskHandle;

/* Private function prototypes */
static void RelaySSRTask_Function(void *argument);

/**
  * @brief  Initialize the Relays and SSRs
  * @retval None
  */
void Relay_SSR_Init(void)
{
    /* Initialize GPIO pins for Relays */
    relay_ssr.relay1_gpio = RELAY_1_GPIO;
    relay_ssr.relay2_gpio = RELAY_2_GPIO;
    relay_ssr.relay3_gpio = RELAY_3_GPIO;
    relay_ssr.relay4_gpio = RELAY_4_GPIO;
    
    /* Initialize GPIO pins for SSRs */
    relay_ssr.ssr1_gpio = SSR_1_GPIO;
    relay_ssr.ssr2_gpio = SSR_2_GPIO;
    
    /* Initialize states to OFF */
    relay_ssr.relay1_state = 0;
    relay_ssr.relay2_state = 0;
    relay_ssr.relay3_state = 0;
    relay_ssr.relay4_state = 0;
    relay_ssr.ssr1_state = 0;
    relay_ssr.ssr2_state = 0;
    
    /* Set initial states */
    gpio_set_level(relay_ssr.relay1_gpio, OFF);
    gpio_set_level(relay_ssr.relay2_gpio, OFF);
    gpio_set_level(relay_ssr.relay3_gpio, OFF);
    gpio_set_level(relay_ssr.relay4_gpio, OFF);
    gpio_set_level(relay_ssr.ssr1_gpio, OFF);
    gpio_set_level(relay_ssr.ssr2_gpio, OFF);
    
    /* Create Relay SSR task */
    if (xTaskCreate(RelaySSRTask_Function, "RelaySSRTask", 2048, NULL, tskIDLE_PRIORITY+1, &relaySSRTaskHandle) != pdPASS) {
        UART_Printf("Relay SSR Task Creation Failed\r\n");
    }
}

/**
  * @brief  Set Relay state
  * @param  relay_num: Relay number (1-4)
  * @param  state: 1 for ON, 0 for OFF
  * @retval None
  */
void Relay_SSR_SetRelay(uint8_t relay_num, uint8_t state)
{
    switch(relay_num) {
        case 1:
            relay_ssr.relay1_state = state;
            gpio_set_level(relay_ssr.relay1_gpio, state ? ON : OFF);
            break;
        case 2:
            relay_ssr.relay2_state = state;
            gpio_set_level(relay_ssr.relay2_gpio, state ? ON : OFF);
            break;
        case 3:
            relay_ssr.relay3_state = state;
            gpio_set_level(relay_ssr.relay3_gpio, state ? ON : OFF);
            break;
        case 4:
            relay_ssr.relay4_state = state;
            gpio_set_level(relay_ssr.relay4_gpio, state ? ON : OFF);
            break;
    }
}

/**
  * @brief  Set SSR state
  * @param  ssr_num: SSR number (1-2)
  * @param  state: 1 for ON, 0 for OFF
  * @retval None
  */
void Relay_SSR_SetSSR(uint8_t ssr_num, uint8_t state)
{
    switch(ssr_num) {
        case 1:
            relay_ssr.ssr1_state = state;
            gpio_set_level(relay_ssr.ssr1_gpio, state ? ON : OFF);
            break;
        case 2:
            relay_ssr.ssr2_state = state;
            gpio_set_level(relay_ssr.ssr2_gpio, state ? ON : OFF);
            break;
    }
}

/**
  * @brief  Get current Relay state
  * @param  relay_num: Relay number (1-4)
  * @retval uint8_t Current Relay state
  */
uint8_t Relay_SSR_GetRelayState(uint8_t relay_num)
{
    switch(relay_num) {
        case 1: return relay_ssr.relay1_state;
        case 2: return relay_ssr.relay2_state;
        case 3: return relay_ssr.relay3_state;
        case 4: return relay_ssr.relay4_state;
        default: return 0;
    }
}

/**
  * @brief  Get current SSR state
  * @param  ssr_num: SSR number (1-2)
  * @retval uint8_t Current SSR state
  */
uint8_t Relay_SSR_GetSSRState(uint8_t ssr_num)
{
    switch(ssr_num) {
        case 1: return relay_ssr.ssr1_state;
        case 2: return relay_ssr.ssr2_state;
        default: return 0;
    }
}

/**
  * @brief  Relay SSR task function
  * @param  argument: Not used
  * @retval None
  */
static void RelaySSRTask_Function(void *argument)
{
    /* Initial delay to ensure system is stable */
    vTaskDelay(pdMS_TO_TICKS(100));
    
    for(;;)
    {
        /* Task runs every 1000ms */
        // UART_Printf("Relay States: %d %d %d %d, SSR States: %d %d\r\n", 
        //            relay_ssr.relay1_state,
        //            relay_ssr.relay2_state,
        //            relay_ssr.relay3_state,
        //            relay_ssr.relay4_state,
        //            relay_ssr.ssr1_state,
        //            relay_ssr.ssr2_state);

        if ((triggerFlg == 1) && (offTime > 0)) {
            gpio_set_level(RELAY_1_GPIO, ON);
            vTaskDelay(pdMS_TO_TICKS(1000));
            gpio_set_level(RELAY_2_GPIO, ON);
            vTaskDelay(pdMS_TO_TICKS(1000));
            gpio_set_level(RELAY_3_GPIO, ON);
            vTaskDelay(pdMS_TO_TICKS(1000));
            gpio_set_level(RELAY_4_GPIO, ON);
            vTaskDelay(pdMS_TO_TICKS(3000));
            mode = 1;
            triggerFlg = 0;
            // osDelay(1000);
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* Note: STM32 timer callback removed; ESP-IDF tasks/timers should be used */