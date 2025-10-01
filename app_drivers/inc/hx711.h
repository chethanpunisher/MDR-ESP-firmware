#ifndef HX711_H_
#define HX711_H_


/*
  Author:     Nima Askari
  WebSite:    http://www.github.com/NimaLTD
  Instagram:  http://instagram.com/github.NimaLTD
  Youtube:    https://www.youtube.com/channel/UCUhY7qY1klJm1d2kulr9ckw
  
  Version:    1.1.1
  
  
  Reversion History:
  
  (1.1.1):
    Add power down/up.
  (1.1.0):
    Add structure, Add calibration, Add weight, change names, ...
  (1.0.0):
    First Release.
*/

#ifdef __cplusplus
extern "C" {
#endif
  
#include <stdint.h>
#ifdef ESP_PLATFORM
#include "driver/gpio.h"
#else
typedef int gpio_num_t;
#endif
#include <stdint.h>

//####################################################################################################################

typedef struct
{
  gpio_num_t    clk_gpio;
  gpio_num_t    dat_gpio;
  int32_t       offset;
  float         coef;
  uint8_t       lock;    
  
}hx711_t;

//####################################################################################################################

void        hx711_init(hx711_t *hx711, gpio_num_t clk_gpio, gpio_num_t dat_gpio);
void        hx711_lock(hx711_t *hx711);
void        hx711_unlock(hx711_t *hx711);
int32_t     hx711_value(hx711_t *hx711);
int32_t     hx711_value_ave(hx711_t *hx711, uint16_t sample);

void        hx711_coef_set(hx711_t *hx711, float coef);
float       hx711_coef_get(hx711_t *hx711);
void        hx711_calibration(hx711_t *hx711, int32_t value_noload, int32_t value_load, float scale);
void        hx711_tare(hx711_t *hx711, uint16_t sample);
float       hx711_weight(hx711_t *hx711, uint16_t sample);
void        hx711_power_down(hx711_t *hx711);
void        hx711_power_up(hx711_t *hx711);

//####################################################################################################################

#ifdef __cplusplus
}
#endif

#endif 
