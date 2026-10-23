#include "Tracking.h"

// 传感器状态数组（0=检测到黑线，1=检测到白线，需根据硬件调整）
uint8_t TrackState[8] = {1,1,1,1,1,1,1,1};

/**
 * @brief  读取8路循迹传感器状态
 * @param  无
 * @retval 无（结果存入TrackState数组）
 */
void TrackSensor_Read(void)
{
  // 读取PA15状态（0=黑线，1=白线）
    TrackState[0] = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15);
    // 读取PA12状态（原注释为PA12，但代码是PA11）
    TrackState[1] = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_11);
    // 读取PA10状态
    TrackState[2] = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_10);
    // 读取PA8状态
    TrackState[3] = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8);
    // 读取PB7状态
    TrackState[4] = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7);
    // 读取PB6状态
    TrackState[5] = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6);
    // 读取PB4状态
    TrackState[6] = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4);
    // 读取PA9状态（原注释为PB3，但代码是PA9）
    TrackState[7] = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_9);
}

/**
 * @brief  根据8路传感器状态计算循迹偏差
 * @retval 偏差值（-3.5到+3.5，0表示居中）
 */
float CalculateTrackError(void)
{
    static float last_error=0;
    TrackSensor_Read();// 读取传感器状态
    float weights[8] = {0, -4.5, -4, -2, 2, 4, 4.5, 0}; // 为每个传感器分配权重
    
    float sum =0;
    float position_error = 0;//误差值
    uint8_t sensor_count  = 0;//检测到黑线的数量
       
    
    for(int i = 0; i < 8; i++) {
        if(TrackState[i] == 0) { // 0表示检测到黑线
            sum += weights[i];
            sensor_count++;
        }
    }
    
    if(sensor_count>0)
    {
        position_error = sum / sensor_count;
         position_error = 0.7 * position_error + 0.3 * last_error;
        last_error = position_error;       
    }
    // 如果没有传感器检测到黑线，保持上一次偏差或返回特殊值
    else
    {
        // 可以根据上一次误差或者返回一个极大值让小车旋转寻找黑线
        return 0; // 返回极大值促使小车旋转
    }
    
    return position_error;
}

uint8_t DetectTurn(void)
{
    TrackSensor_Read();
    
    // 左转检测：左侧传感器检测到黑线，右侧传感器为白线
    if (TrackState[0] == 0 && TrackState[1] == 0 && 
        TrackState[2] == 0 && TrackState[6] == 1 && 
        TrackState[7] == 1)
    {
        return 1; // 左转
    }
    // 右转检测：右侧传感器检测到黑线，左侧传感器为白线
    else if ((TrackState[0]==0 && TrackState[1]==0 && TrackState[2]==1 && TrackState[6]==1 && TrackState[7]==1)||
    (TrackState[0]==0 && TrackState[2]==1 && TrackState[6]==1 && TrackState[7]==1)||
    (TrackState[1]==0 && TrackState[2]==1 && TrackState[6]==1 && TrackState[7]==1))
    {
        return 2; // 右转
    }
    if(TrackState[0]==0 && TrackState[1]==0 && TrackState[2]==0 && TrackState[3]==0 && 
       TrackState[4]==0 && TrackState[5]==0 && TrackState[6]==0 && TrackState[7]==0)
    {
        return 3;
    }
  
    return 0; // 不需要转弯
}
