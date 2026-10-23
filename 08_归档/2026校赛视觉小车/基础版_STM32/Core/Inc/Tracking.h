#ifndef __TRACKING_H
#define __TRACKING_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "main.h"
#include "motor.h"
#include "oled.h"
#include "gpio.h"
#include "stm32f1xx_hal.h"
/* Function Prototypes -------------------------------------------------------*/
extern uint8_t TrackState[8];

void TrackSensor_Read(void);
float CalculateTrackError(void);
uint8_t DetectTurn(void);

#ifdef __cplusplus
}
#endif

#endif /* __TRACKING_H */