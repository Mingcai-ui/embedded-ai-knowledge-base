/**
  * @file    Motor.c
  * @brief   直流电机控制模块 - HAL库版本
  * @author  Embedded Systems
  * @date    2025-01-01
  * @note    使用H桥驱动直流电机，支持正转、反转、刹车和停止
  */

/* Includes ------------------------------------------------------------------*/
#include "Motor.h"

/* Private variables ---------------------------------------------------------*/
static Motor_State_t RightMotor_State = MOTOR_STATE_STOP;
static Motor_State_t LeftMotor_State = MOTOR_STATE_STOP;

/* Private function prototypes -----------------------------------------------*/

static uint16_t Motor_LimitSpeed(int16_t Speed);

/* Functions -----------------------------------------------------------------*/

/**
  * @brief  电机初始化
  * @note   初始化GPIO和PWM，设置电机初始状态为停止
  */
void Motor_Init(void)
{
    /* GPIO已经在MX_GPIO_Init中初始化，这里只需确保状态正确 */
    
    /* 停止电机 */
    Motor_Stop();
    
    /* 初始化PWM */
    PWM_Init();
    
    /* 设置初始状态 */
    RightMotor_State = MOTOR_STATE_STOP;
    LeftMotor_State = MOTOR_STATE_STOP;
}

/**
  * @brief  设置右侧电机速度
  * @param  Speed: 速度值，正数正转，负数反转，0停止 (-999 到 999)
  */
void Motor_Right_SetSpeed(int16_t Speed)
{
    uint16_t pwm_value;
    
    /* 限制速度范围 */
    Speed = Motor_LimitSpeed(Speed);
    
    if (Speed > 0)  /* 正转 */
    {
        /* 设置方向：IN1=1, IN2=0 */
        HAL_GPIO_WritePin(MOTOR_RIGHT_IN1_PORT, MOTOR_RIGHT_IN1_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(MOTOR_RIGHT_IN2_PORT, MOTOR_RIGHT_IN2_PIN, GPIO_PIN_RESET);
        
        /* 设置PWM占空比 */
        PWM_SetCompare1((uint16_t)Speed);
        
        RightMotor_State = MOTOR_STATE_FORWARD;
    }
    else if (Speed < 0)  /* 反转 */
    {
        /* 设置方向：IN1=0, IN2=1 */
        HAL_GPIO_WritePin(MOTOR_RIGHT_IN1_PORT, MOTOR_RIGHT_IN1_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MOTOR_RIGHT_IN2_PORT, MOTOR_RIGHT_IN2_PIN, GPIO_PIN_SET);
        
        /* 设置PWM占空比 */
        pwm_value = (uint16_t)(-Speed);
        PWM_SetCompare1(pwm_value);
        
        RightMotor_State = MOTOR_STATE_BACKWARD;
    }
    else  /* 停止 */
    {
        /* 设置方向：IN1=0, IN2=0 */
        HAL_GPIO_WritePin(MOTOR_RIGHT_IN1_PORT, MOTOR_RIGHT_IN1_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MOTOR_RIGHT_IN2_PORT, MOTOR_RIGHT_IN2_PIN, GPIO_PIN_RESET);
        
        /* 设置PWM占空比为0 */
        PWM_SetCompare1(0);
        
        RightMotor_State = MOTOR_STATE_STOP;
    }
}

/**
  * @brief  设置左侧电机速度
  * @param  Speed: 速度值，正数正转，负数反转，0停止 (-999 到 999)
  */
void Motor_Left_SetSpeed(int16_t Speed)
{
    uint16_t pwm_value;
    
    /* 限制速度范围 */
    Speed = Motor_LimitSpeed(Speed);
    
    if (Speed > 0)  /* 正转 */
    {
        /* 设置方向：IN1=0, IN2=1 */
        HAL_GPIO_WritePin(MOTOR_LEFT_IN1_PORT, MOTOR_LEFT_IN1_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MOTOR_LEFT_IN2_PORT, MOTOR_LEFT_IN2_PIN, GPIO_PIN_SET);
        
        /* 设置PWM占空比 */
        PWM_SetCompare2((uint16_t)Speed);
        
        LeftMotor_State = MOTOR_STATE_FORWARD;
    }
    else if (Speed < 0)  /* 反转 */
    {
        /* 设置方向：IN1=1, IN2=0 */
        HAL_GPIO_WritePin(MOTOR_LEFT_IN1_PORT, MOTOR_LEFT_IN1_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(MOTOR_LEFT_IN2_PORT, MOTOR_LEFT_IN2_PIN, GPIO_PIN_RESET);
        
        /* 设置PWM占空比 */
        pwm_value = (uint16_t)(-Speed);
        PWM_SetCompare2(pwm_value);
        
        LeftMotor_State = MOTOR_STATE_BACKWARD;
    }
    else  /* 停止 */
    {
        /* 设置方向：IN1=0, IN2=0 */
        HAL_GPIO_WritePin(MOTOR_LEFT_IN1_PORT, MOTOR_LEFT_IN1_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MOTOR_LEFT_IN2_PORT, MOTOR_LEFT_IN2_PIN, GPIO_PIN_RESET);
        
        /* 设置PWM占空比为0 */
        PWM_SetCompare2(0);
        
        LeftMotor_State = MOTOR_STATE_STOP;
    }
}

/**
  * @brief  同时设置两个电机的速度
  * @param  RightSpeed: 右侧电机速度 (-999 到 999)
  * @param  LeftSpeed: 左侧电机速度 (-999 到 999)
  */
void Motor_SetSpeeds(int16_t RightSpeed, int16_t LeftSpeed)
{
    Motor_Right_SetSpeed(RightSpeed);
    Motor_Left_SetSpeed(LeftSpeed);
}

/**
  * @brief  刹车功能
  * @note   将电机两端短接，实现快速制动
  */
void Motor_Brake(void)
{
    /* 右侧电机刹车：IN1=1, IN2=1 */
    HAL_GPIO_WritePin(MOTOR_RIGHT_IN1_PORT, MOTOR_RIGHT_IN1_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(MOTOR_RIGHT_IN2_PORT, MOTOR_RIGHT_IN2_PIN, GPIO_PIN_SET);
    
    /* 左侧电机刹车：IN1=1, IN2=1 */
    HAL_GPIO_WritePin(MOTOR_LEFT_IN1_PORT, MOTOR_LEFT_IN1_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(MOTOR_LEFT_IN2_PORT, MOTOR_LEFT_IN2_PIN, GPIO_PIN_SET);
    
    /* 设置PWM占空比为0 */
    PWM_SetCompare1(0);
    PWM_SetCompare2(0);
    
    RightMotor_State = MOTOR_STATE_BRAKE;
    LeftMotor_State = MOTOR_STATE_BRAKE;
}

/**
  * @brief  停止电机
  * @note   释放电机，使电机自由旋转
  */
void Motor_Stop(void)
{
    /* 右侧电机停止：IN1=0, IN2=0 */
    HAL_GPIO_WritePin(MOTOR_RIGHT_IN1_PORT, MOTOR_RIGHT_IN1_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MOTOR_RIGHT_IN2_PORT, MOTOR_RIGHT_IN2_PIN, GPIO_PIN_RESET);
    
    /* 左侧电机停止：IN1=0, IN2=0 */
    HAL_GPIO_WritePin(MOTOR_LEFT_IN1_PORT, MOTOR_LEFT_IN1_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MOTOR_LEFT_IN2_PORT, MOTOR_LEFT_IN2_PIN, GPIO_PIN_RESET);
    
    /* 设置PWM占空比为0 */
    PWM_SetCompare1(0);
    PWM_SetCompare2(0);
    
    RightMotor_State = MOTOR_STATE_STOP;
    LeftMotor_State = MOTOR_STATE_STOP;
}

/**
  * @brief  获取右侧电机状态
  * @retval Motor_State_t: 电机状态
  */
Motor_State_t Motor_Right_GetState(void)
{
    return RightMotor_State;
}

/**
  * @brief  获取左侧电机状态
  * @retval Motor_State_t: 电机状态
  */
Motor_State_t Motor_Left_GetState(void)
{
    return LeftMotor_State;
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  限制速度值范围
  * @param  Speed: 输入速度值
  * @retval int16_t: 限制后的速度值
  */
static uint16_t Motor_LimitSpeed(int16_t Speed)
{
    if (Speed > MOTOR_MAX_SPEED)
    {
        return MOTOR_MAX_SPEED;
    }
    else if (Speed < -MOTOR_MAX_SPEED)
    {
        return -MOTOR_MAX_SPEED;
    }
    else
    {
        return Speed;
    }
}


