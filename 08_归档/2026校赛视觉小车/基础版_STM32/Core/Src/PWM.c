/**
  * @file    PWM.c
  * @brief   PWM控制模块 - HAL库版本
  * @author  Embedded Systems
  * @date    2025-01-01
  * @note    使用HAL库控制TIM2产生PWM信号
  */

/* Includes ------------------------------------------------------------------*/
#include "PWM.h"

/* Private function prototypes -----------------------------------------------*/
static void PWM_Error_Handler(void);

/* Functions -----------------------------------------------------------------*/

/**
  * @brief  初始化PWM功能
  * @note   基于TIM2的通道1和通道2生成PWM信号
  *         频率: 72MHz / (71+1) / (999+1) = 1kHz
  */
void PWM_Init(void)
{
    /* 使用HAL库的定时器初始化 */
    MX_TIM2_Init();
    
    /* 启动PWM输出 */
    if (HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1) != HAL_OK)
    {
        PWM_Error_Handler();
    }
    
    if (HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2) != HAL_OK)
    {
        PWM_Error_Handler();
    }
    
    /* 初始化为0占空比 */
    PWM_SetCompare1(0);
    PWM_SetCompare2(0);
}

/**
  * @brief  设置通道1的PWM占空比
  * @param  Compare: 比较值 (0-999)
  */
void PWM_SetCompare1(uint16_t Compare)
{
    /* 限制比较值范围 */
    if (Compare > htim2.Init.Period)
    {
        Compare = htim2.Init.Period;
    }
    
    /* 设置TIM2通道1的比较值 */
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, Compare);
    
    /* 或者使用HAL库函数 */
    // TIM_OC_InitTypeDef sConfigOC = {0};
    // sConfigOC.OCMode = TIM_OCMODE_PWM1;
    // sConfigOC.Pulse = Compare;
    // sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    // sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    // if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
    // {
    //     PWM_Error_Handler();
    // }
}

/**
  * @brief  设置通道2的PWM占空比
  * @param  Compare: 比较值 (0-999)
  */
void PWM_SetCompare2(uint16_t Compare)
{
    /* 限制比较值范围 */
    if (Compare > htim2.Init.Period)
    {
        Compare = htim2.Init.Period;
    }
    
    /* 设置TIM2通道2的比较值 */
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, Compare);
}

/**
  * @brief  PWM错误处理函数
  */
static void PWM_Error_Handler(void)
{
    /* 可以在这里添加错误处理代码 */
    /* 例如: LED闪烁, 串口打印错误信息等 */
    while (1)
    {
        HAL_Delay(100);
    }
}