#ifndef __KEY_H
#define __KEY_H


#include "main.h"

#define     KEY_1_GPIO_PIN      GPIO_PIN_0
#define     KEY_2_GPIO_PIN      GPIO_PIN_1
#define     KEY_1_GPIOX         GPIOB
#define     KEY_2_GPIOX         GPIOB

uint8_t Key_GetNum(void);

#endif
