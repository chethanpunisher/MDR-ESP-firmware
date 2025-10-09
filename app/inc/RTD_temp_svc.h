#ifndef RTD_TEMP_SVC_H
#define RTD_TEMP_SVC_H

#include "config.h"
#include "max31865.h"
#include "Relay_SSR_svc.h"

/* Exported types */
typedef struct {
    MAX31865_Handle_t max31865_dev1;
    MAX31865_Handle_t max31865_dev2;
    float current_temperature_dev1;
    float current_temperature_dev2;
    float known_temperature_dev1;
    float known_temperature_dev2;
    float tempSetPoint;           // Global setpoint (legacy)
    float tempSetPoint_dev1;      // Individual setpoint for device 1
    float tempSetPoint_dev2;      // Individual setpoint for device 2
} RTD_Temp_Handle_t;

/* Exported functions */
void RTD_Temp_Init(void);
float RTD_Temp_GetTemperature(uint8_t dev_num);
void RTD_Temp_Calibrate(uint8_t dev_num, float known_temp);
void RTD_Temp_SetTempSetPoint(float tempSetPoint);
void RTD_Temp_SetTempSetPointIndividual(uint8_t dev_num, float tempSetPoint);
void RTD_Temp_SetFactor(uint8_t dev_num, float factor);

/* EEPROM-backed calibration helpers */
void RTD_Temp_LoadCalibration(void);
void RTD_Temp_CalibrateAndSave(uint8_t dev_num, float known_temp);

#endif /* RTD_TEMP_SVC_H */
