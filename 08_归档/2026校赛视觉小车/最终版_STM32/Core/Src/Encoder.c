#include "Encoder.h"

TIM_HandleTypeDef htim3;

/**
 * @brief 初始化编码器相关硬件（TIM3定时中断 + PB12/PB13输入）
 */
void Encoder_Init(void)
{
    // 1. 配置 PB12、PB13 为上拉输入（无中断）
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // 2. 配置 TIM3 为基础定时器（100μs 中断，10kHz 采样）
    __HAL_RCC_TIM3_CLK_ENABLE();
    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 72 - 1;          // 1 MHz
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 100 - 1;            // 100μs 溢出，10倍采样率
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_Base_Init(&htim3);

    HAL_NVIC_SetPriority(TIM3_IRQn, 2, 0);  // 优先级低于 TIM2
    HAL_NVIC_EnableIRQ(TIM3_IRQn);

    HAL_TIM_Base_Start_IT(&htim3);
}