#ifndef _HX711CONFIG_H_
#define _HX711CONFIG_H_

#define		_HX711_USE_FREERTOS		1
 #define   _HX711_DELAY_US_LOOP  2

// ESP32 GPIO pins
#ifndef CONFIG_HX711_CLK_GPIO
#define CONFIG_HX711_CLK_GPIO  2   // CLK -> IO2
#endif
#ifndef CONFIG_HX711_DAT_GPIO
#define CONFIG_HX711_DAT_GPIO  15  // DOUT -> IO15
#endif

#endif 
