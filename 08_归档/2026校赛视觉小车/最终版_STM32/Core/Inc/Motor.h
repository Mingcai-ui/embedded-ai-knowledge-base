#ifndef __MOTOR_H
#define __MOTOR_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "PWM.h"

/* Motor Pin Definitions -----------------------------------------------------*/
/* 右侧电机控制引脚 */
#define MOTOR_RIGHT_IN1_PIN    GPIO_PIN_4
#define MOTOR_RIGHT_IN1_PORT   GPIOA
#define MOTOR_RIGHT_IN2_PIN    GPIO_PIN_5
#define MOTOR_RIGHT_IN2_PORT   GPIOA

/* 左侧电机控制引脚 */
#define MOTOR_LEFT_IN1_PIN     GPIO_PIN_6
#define MOTOR_LEFT_IN1_PORT    GPIOA
#define MOTOR_LEFT_IN2_PIN     GPIO_PIN_7
#define MOTOR_LEFT_IN2_PORT    GPIOA

/* Motor Speed Limits --------------------------------------------------------*/
#define MOTOR_MAX_SPEED        999   /* 最大PWM占空比 */
#define MOTOR_MIN_SPEED        0     /* 最小PWM占空比 */

/* Motor States --------------------------------------------------------------*/
typedef enum
{
    MOTOR_STATE_STOP = 0,
    MOTOR_STATE_FORWARD,
    MOTOR_STATE_BACKWARD,
    MOTOR_STATE_BRAKE
} Motor_State_t;

/* Function Prototypes -------------------------------------------------------*/
void Motor_Init(void);
void Motor_Right_SetSpeed(int16_t Speed);
void Motor_Left_SetSpeed(int16_t Speed);
void Motor_SetSpeeds(int16_t RightSpeed, int16_t LeftSpeed);
void Motor_Brake(void);
void Motor_Stop(void);
Motor_State_t Motor_Right_GetState(void);
Motor_State_t Motor_Left_GetState(void);

#ifdef __cplusplus
}
#endif

#endif /* __MOTOR_H */