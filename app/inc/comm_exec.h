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

#endif /* COMM_EXEC_H */