#include "PID.h"
// 定义全局变量
PID_t Angle_pid;
extern float gyroZ_angle;  // 通过陀螺仪积分计算的角度
/**
 * @brief  初始化PID控制器参数，为PID运算做准备
 * @param  pid: 指向PID结构体的指针，用于存储PID控制器的所有参数（输入输出、系数、误差等）
 * @param  kp: 比例系数（Proportional gain），决定对当前误差的响应强度
 * @param  ki: 积分系数（Integral gain），决定对累积误差的响应强度，用于消除稳态误差
 * @param  kd: 微分系数（Derivative gain），决定对误差变化率的响应强度，用于抑制超调
 * @param  out_max: PID输出的最大值（用于限幅，防止输出过大导致系统不稳定）
 */
void PID_Init(PID_t* pid, float kp, float ki, float kd, float out_max) 
{
    pid->Kp = kp;          // 初始化比例系数：将外部传入的kp赋值给结构体的Kp成员
    pid->Ki = ki;          // 初始化积分系数：将外部传入的ki赋值给结构体的Ki成员
    pid->Kd = kd;          // 初始化微分系数：将外部传入的kd赋值给结构体的Kd成员
    pid->Target = 0;       // 初始化PID的目标值（如循迹中小车期望的偏差为0），初始设为0
    pid->OutMax = out_max; // 初始化输出最大值：限制PID输出的上限（如电机PWM的最大占空比）
    pid->OutMin = -out_max;// 初始化输出最小值：限制PID输出的下限（与最大值对称，确保正负方向限幅一致）
    pid->ErrorInt = 0;     // 初始化积分误差累积值：积分项是误差的累积，初始化为0（无累积）
    pid->Error0 = 0;       // 初始化当前误差（第0次误差）：用于存储当前计算周期的误差，初始化为0
    pid->Error1 = 0;       // 初始化上一次误差（第1次误差）：用于计算微分项（当前误差-上一次误差），初始化为0
}

// PID控制器更新函数
void PID_Update(PID_t *p)
{
    // 更新误差历史
    p->Error1 = p->Error0;              // 保存上次误差
    p->Error0 = p->Target - p->Actual;  // 计算当前误差
    // 积分项处理
    if (p->Ki != 0)
    {
        p->ErrorInt += p->Error0;       // 累积误差积分
    }
    else
    {
        p->ErrorInt = 0;                // 无积分作用时清零
    }
    if(p->ErrorInt>400){p->ErrorInt=400;}
    if(p->ErrorInt<-400){p->ErrorInt=-400;}

    // PID输出计算
    p->Out = p->Kp * p->Error0          // 比例项
           + p->Ki * p->ErrorInt        // 积分项  
           + p->Kd * (p->Error0 - p->Error1); // 微分项
    
    // 输出限幅
    if (p->Out > p->OutMax) {p->Out = p->OutMax;}  // 上限
    if (p->Out < p->OutMin) {p->Out = p->OutMin;}  // 下限
}

// // 新增函数：更新角度（通过陀螺仪积分）
// void UpdateAngle(void)
// {
//     static uint32_t lastTime = 0;
//     uint32_t currentTime;
//     float dt;
//     int16_t AccX, AccY, AccZ, GyroX, GyroY, GyroZ;
    
//     // 获取MPU6050数据
//     MPU6050_GetData(&AccX, &AccY, &AccZ, &GyroX, &GyroY, &GyroZ);
    
//     // 获取当前时间（使用TIM2的计数器）
//     currentTime = __HAL_TIM_GET_COUNTER(&htim2);
    
//     // 计算时间差（秒）
//     if (lastTime == 0) {
//         lastTime = currentTime;
//         return;
//     }
    
//     dt = (currentTime - lastTime) * 0.001f; // 假设1ms为单位
//     lastTime = currentTime;
    
//     // 陀螺仪Z轴积分（角速度积分得到角度）
//     // MPU6050量程为±2000°/s，转换系数：2000/32768 ≈ 0.061
//     float gyroZ_dps = GyroZ * 0.061f;  // 度/秒
    
//     // 积分得到角度变化（度）
//     gyroZ_angle += gyroZ_dps * dt;
    
//     // 可选：使用互补滤波融合加速度计和陀螺仪
//     // 这里简化只使用陀螺仪积分
// }
// // 新增函数：执行转弯
// void ExecuteTurn(uint8_t turnType)
// {
//     // 设置目标角度
//     if (turnType == 1) 
//       Angle_pid.Target = gyroZ_angle + 90.0f;  // 左转90度

//     else if (turnType == 2) 
//       Angle_pid.Target = gyroZ_angle - 90.0f;  // 右转90度 

//     else 
//       Angle_pid.Target = gyroZ_angle + 90.0f;  // 默认左转
    
//     // 重置角度环积分（可选，根据实际情况决定）
//     // Angle_pid.ErrorInt = 0;
//     // Angle_pid.Error0 = 0;
//     // Angle_pid.Error1 = 0;
//}
/*     速度环     
PID_t R_pid={
    .Kp=1.0 ,
    .Ki=0,
    .Kd=1,

    .OutMax=400,
    .OutMin=-400,   
  }; */
/*//中断内容
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{  
  
  static uint16_t Count1;
    if (htim->Instance == TIM2)          // 判断是否为 TIM2
    {
        int16_t num = 200;               // 注意：num 原本在中断函数内定义，这里按需保留
        // 电机控制周期（40ms）
        Count1++;
        if (Count1 >= 40)                // 40ms 控制周期
        {
            Count1 = 0;

            if(turn==3)
            {//停车
              L_Speet=0;
              R_Speet=0;

              R_pid.ErrorInt = 0;
              R_pid.Error0  = 0;
              R_pid.Error1  = 0;
              
              Motor_Left_SetSpeed(L_Speet);
              Motor_Right_SetSpeed(R_Speet);
            }
            else if(turn_active==1)
            {//进圈
              L_Speet = 200;      // 左轮速度（正转）
              R_Speet = -190;     // 右轮速度（反转）——原地旋

              if (L_Speet > 400) L_Speet = 400;
              if (L_Speet < -400) L_Speet = -400;
              if (R_Speet > 400) R_Speet = 400;
              if (R_Speet < -400) R_Speet = -400;

              Motor_Left_SetSpeed(L_Speet);
              Motor_Right_SetSpeed(R_Speet);
              
              turn_timer--;          // 倒计时递减
              if (turn_timer == 0) {
                turn_active = 0;   // 转弯结束，恢复循迹模式
              }
            }
            else
            {//PID内容
              R_pid.Target = 0;            // 期望小车在赛道中心
              R_pid.Actual = CalculateTrackError();
              PID_Update(&R_pid);

              L_Speet = num + R_pid.Out * 30;   // 电机实际速度
              R_Speet = num - R_pid.Out * 30;   

              if (L_Speet > 400) L_Speet = 400;
              if (L_Speet < -400) L_Speet = -400;
              if (R_Speet > 400) R_Speet = 400;
              if (R_Speet < -400) R_Speet = -400;

              // 限制速度范围
              Motor_Left_SetSpeed(L_Speet);
              Motor_Right_SetSpeed(R_Speet);
            }
        }
    }
}

*/