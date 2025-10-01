#include "hx711.h"
#include "hx711Config.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define hx711_delay(x)    vTaskDelay(pdMS_TO_TICKS(x))

//#############################################################################################
static inline void hx711_delay_us_inline(uint32_t us)
{
  uint64_t start = esp_timer_get_time();
  while ((esp_timer_get_time() - start) < us) { }
}
//#############################################################################################
void hx711_lock(hx711_t *hx711)
{
  while (hx711->lock)
    hx711_delay(1);
  hx711->lock = 1;      
}
//#############################################################################################
void hx711_unlock(hx711_t *hx711)
{
  hx711->lock = 0;
}
//#############################################################################################
void hx711_init(hx711_t *hx711, gpio_num_t clk_gpio, gpio_num_t dat_gpio)
{
  hx711_lock(hx711);
  hx711->clk_gpio = clk_gpio;
  hx711->dat_gpio = dat_gpio;
  
  gpio_config_t clk_conf = {
    .pin_bit_mask = (1ULL << clk_gpio),
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE
  };
  gpio_config(&clk_conf);
  gpio_config_t dat_conf = {
    .pin_bit_mask = (1ULL << dat_gpio),
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_ENABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE
  };
  gpio_config(&dat_conf);
  gpio_set_level(hx711->clk_gpio, 1);
  hx711_delay(10);
  gpio_set_level(hx711->clk_gpio, 0);
  hx711_delay(10);  
  hx711_value(hx711);
  hx711_value(hx711);
  hx711_unlock(hx711); 
}
//#############################################################################################
int32_t hx711_value(hx711_t *hx711)
{
  uint32_t data = 0;
  uint64_t  startTime = esp_timer_get_time();
  while(gpio_get_level(hx711->dat_gpio) == 1)
  {
    hx711_delay(1);
    if((esp_timer_get_time() - startTime) > 150000)
      return 0;
  }
  for(int8_t i=0; i<24 ; i++)
  {
    gpio_set_level(hx711->clk_gpio, 1);   
    hx711_delay_us_inline(_HX711_DELAY_US_LOOP);
    gpio_set_level(hx711->clk_gpio, 0);
    hx711_delay_us_inline(_HX711_DELAY_US_LOOP);
    data = data << 1;    
    if(gpio_get_level(hx711->dat_gpio) == 1)
      data ++;
  }
  data = data ^ 0x800000; 
  gpio_set_level(hx711->clk_gpio, 1);   
  hx711_delay_us_inline(_HX711_DELAY_US_LOOP);
  gpio_set_level(hx711->clk_gpio, 0);
  hx711_delay_us_inline(_HX711_DELAY_US_LOOP);
  return data;    
}
//#############################################################################################
int32_t hx711_value_ave(hx711_t *hx711, uint16_t sample)
{
  hx711_lock(hx711);
  int64_t  ave = 0;
  for(uint16_t i=0 ; i<sample ; i++)
  {
    ave += hx711_value(hx711);
    hx711_delay(5);
  }
  int32_t answer = (int32_t)(ave / sample);
  hx711_unlock(hx711);
  return answer;
}
//#############################################################################################
void hx711_tare(hx711_t *hx711, uint16_t sample)
{
  hx711_lock(hx711);
  int64_t  ave = 0;
  for(uint16_t i=0 ; i<sample ; i++)
  {
    ave += hx711_value(hx711);
    hx711_delay(5);
  }
  hx711->offset = (int32_t)(ave / sample);
  hx711_unlock(hx711);
}
//#############################################################################################
void hx711_calibration(hx711_t *hx711, int32_t noload_raw, int32_t load_raw, float scale)
{
  hx711_lock(hx711);
  hx711->offset = noload_raw;
  hx711->coef = (load_raw - noload_raw) / scale;  
  hx711_unlock(hx711);
}
//#############################################################################################
float hx711_weight(hx711_t *hx711, uint16_t sample)
{
  hx711_lock(hx711);
  int64_t  ave = 0;
  for(uint16_t i=0 ; i<sample ; i++)
  {
    ave += hx711_value(hx711);
    hx711_delay(5);
  }
  int32_t data = (int32_t)(ave / sample);
  float answer =  (data - hx711->offset) / hx711->coef;
  hx711_unlock(hx711);
  return answer;
}
//#############################################################################################
void hx711_coef_set(hx711_t *hx711, float coef)
{
  hx711->coef = coef;  
}
//#############################################################################################
float hx711_coef_get(hx711_t *hx711)
{
  return hx711->coef;  
}
//#############################################################################################
void hx711_power_down(hx711_t *hx711)
{
  gpio_set_level(hx711->clk_gpio, 0);
  gpio_set_level(hx711->clk_gpio, 1);
  hx711_delay(1);  
}
//#############################################################################################
void hx711_power_up(hx711_t *hx711)
{
  gpio_set_level(hx711->clk_gpio, 0);
}
//#############################################################################################
