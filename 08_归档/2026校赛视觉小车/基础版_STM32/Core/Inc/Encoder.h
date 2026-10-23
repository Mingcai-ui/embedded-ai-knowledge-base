#ifndef __ENCODER_H
#define __ENCODER_H

#include "stm32f1xx_hal.h"

void Encoder_Init(void);
int16_t Encoder_GetDelta(void);    // 返回自上次调用以来的脉冲变化量（带方向）
void Encoder_Reset(void);          // 复位内部计数器

#endif