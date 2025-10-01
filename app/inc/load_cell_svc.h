#ifndef LOAD_CELL_SVC_H
#define LOAD_CELL_SVC_H

#include "hx711.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "config.h"
#include "hx711Config.h"

/* Exported types */
typedef struct {
    hx711_t hx711;
    float current_weight;
    float tare_weight;
    float calibration_factor;
} LoadCell_Handle_t;

/* Exported functions */
void LoadCell_Init(void);
float LoadCell_GetWeight(void);
void LoadCell_Tare(void);
void LoadCell_Calibrate(float known_weight);

#endif /* LOAD_CELL_SVC_H */
