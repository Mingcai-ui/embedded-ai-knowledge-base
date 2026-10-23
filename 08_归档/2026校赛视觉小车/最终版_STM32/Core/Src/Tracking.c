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
    TrackState[0] = HAL_GPIO_ReadPin(GPIOA, IN0);
    TrackState[1] = HAL_GPIO_ReadPin(GPIOA, IN1);
    TrackState[2] = HAL_GPIO_ReadPin(GPIOA, IN2);
    TrackState[3] = HAL_GPIO_ReadPin(GPIOA, IN3);
    TrackState[4] = HAL_GPIO_ReadPin(GPIOB, IN4);
    TrackState[5] = HAL_GPIO_ReadPin(GPIOB, IN5);
    TrackState[6] = HAL_GPIO_ReadPin(GPIOB, IN6);
    TrackState[7] = HAL_GPIO_ReadPin(GPIOB, IN7);
}

/**
 * @brief  根据8路传感器状态计算循迹偏差
 * @retval 偏差值（负数左偏，正数右偏，0居中）
 */
float CalculateTrackError(void)
{
    static float last_error = 0;
    TrackSensor_Read();

    // 权重：左负右正，对称分布
    float weights[8] = {-5.5, -4, -3, -2, 2, 3, 4, 5.5};

    float sum = 0;
    uint8_t sensor_count = 0;

    for (int i = 0; i < 8; i++) {
        if (TrackState[i] == 0) {   // 0 = 检测到黑线
            sum += weights[i];
            sensor_count++;
        }
    }

    if (sensor_count > 0) {
        float position_error = sum / sensor_count;
        position_error = 0.7f * position_error + 0.3f * last_error;
        last_error = position_error;
        return position_error;
    } else {
        // 丢线：返回 LOST_SENSOR_INDEX 对应传感器的误差
        return -1;
    }
}

// uint8_t DetectTurn(void)
// {
//     TrackSensor_Read();

//     // 左转检测：左侧传感器检测到黑线，右侧传感器为白线
//     if (TrackState[0] == 0 && TrackState[1] == 0 &&
//         TrackState[2] == 0 && TrackState[6] == 1 &&
//         TrackState[7] == 1)
//     {
//         return 1; // 左转
//     }
//     // 右转检测：右侧传感器检测到黑线，左侧传感器为白线
//     else if ((TrackState[0]==0 && TrackState[1]==0 && TrackState[2]==1 && TrackState[6]==1 && TrackState[7]==1)||
//     (TrackState[0]==0 && TrackState[2]==1 && TrackState[6]==1 && TrackState[7]==1)||
//     (TrackState[1]==0 && TrackState[2]==1 && TrackState[6]==1 && TrackState[7]==1))
//     {
//         return 2; // 右转
//     }
//     if(TrackState[0]==0 && TrackState[1]==0 && TrackState[2]==0 && TrackState[3]==0 &&
//        TrackState[4]==0 && TrackState[5]==0 && TrackState[6]==0 && TrackState[7]==0)
//     {
//         return 3;
//     }

//     return 0; // 不需要转弯
// }