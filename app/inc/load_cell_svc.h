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
    int32_t last_raw;
    int32_t last_raw_filtered;
    
    // Moving average filter for raw values
    int32_t filter_buffer[10];
    int filter_index;
    int filter_count;
} LoadCell_Handle_t;

/* Exported functions */
void LoadCell_Init(void);
float LoadCell_GetWeight(void);
void LoadCell_Tare(void);
void LoadCell_Calibrate(float known_weight);
int32_t LoadCell_GetRaw(void);
int32_t LoadCell_GetRawFiltered(void);

#endif /* LOAD_CELL_SVC_H */
