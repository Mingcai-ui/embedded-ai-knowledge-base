#ifndef __PID_H
#define __PID_H


#include "main.h"
#include "motor.h"
//#include "MPU6050.h"

// PID控制器结构体
typedef struct {
    float Target;    // 目标值
    float Actual;    // 实际值
    float Out;       // 输出值
    
    float Kp;        // 比例系数
    float Ki;        // 积分系数  
    float Kd;        // 微分系数
    
    float Error0;    // 当前误差
    float Error1;    // 上次误差
    float ErrorInt;  // 误差积分
    
    float OutMax;    // 输出上限
    float OutMin;    // 输出下限
} PID_t;

void PID_Update(PID_t *p);
void PID_Init(PID_t* pid, float kp, float ki, float kd, float out_max);
void UpdateAngle(void);
void ExecuteTurn(uint8_t turnType);

#endif
