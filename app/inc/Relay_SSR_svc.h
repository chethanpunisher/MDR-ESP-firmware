#ifndef RELAY_SSR_SVC_H
#define RELAY_SSR_SVC_H

#include <stdint.h>
#include "driver/gpio.h"

#define ON 1
#define OFF 0

#define SSR_ON 1
#define SSR_OFF 0

/* Exported types */
typedef struct {
    /* Relay GPIO configurations */
    gpio_num_t relay1_gpio;
    gpio_num_t relay2_gpio;
    gpio_num_t relay3_gpio;
    gpio_num_t relay4_gpio;
    
    /* SSR GPIO configurations */
    gpio_num_t ssr1_gpio;
    gpio_num_t ssr2_gpio;
    
    /* States */
    uint8_t relay1_state;
    uint8_t relay2_state;
    uint8_t relay3_state;
    uint8_t relay4_state;
    uint8_t ssr1_state;
    uint8_t ssr2_state;
} Relay_SSR_Handle_t;

/* Exported functions */
void Relay_SSR_Init(void);
void Relay_SSR_SetRelay(uint8_t relay_num, uint8_t state);
void Relay_SSR_SetSSR(uint8_t ssr_num, uint8_t state);
uint8_t Relay_SSR_GetRelayState(uint8_t relay_num);
uint8_t Relay_SSR_GetSSRState(uint8_t ssr_num);

#endif /* RELAY_SSR_SVC_H */
