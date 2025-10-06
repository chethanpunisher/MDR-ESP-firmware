#ifndef COMM_EXEC_H
#define COMM_EXEC_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "config.h"
#include "load_cell_svc.h"
#include "Relay_SSR_svc.h"
#include "RTD_temp_svc.h"
/* flash_svc removed on ESP32 */

/* Exported types */
extern TaskHandle_t CommTaskHandle;

/* Exported functions */
void CommTask_Init(void);
void CommTask_Start(void);

/* JSON command format (examples):
   {"cmd":"rtd_calib","dev":1,"known":100.0}
   {"cmd":"set_temp","value":180}
   {"cmd":"set_mode","value":"run|idle|stop|calib"}
   {"cmd":"set_run_time","seconds":120}
   {"cmd":"calibrate_mdr","weight":2.0,"lever":0.12}
   {"cmd":"offset_mdr","ms":5000}
*/

#endif /* COMM_EXEC_H */