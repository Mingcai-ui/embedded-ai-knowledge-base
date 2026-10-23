#include "MPU6050.h"

#define MPU6050_ADDRESS       0xD0

/**
 * @brief 写MPU6050寄存器
 * @param RegAddress 寄存器地址
 * @param Data       要写入的数据
 */
void MPU6050_WriteReg(uint8_t RegAddress, uint8_t Data)
{
    HAL_I2C_Mem_Write(&hi2c2, MPU6050_ADDRESS, RegAddress, I2C_MEMADD_SIZE_8BIT, &Data, 1, HAL_MAX_DELAY);
}

/**
 * @brief 读MPU6050寄存器
 * @param RegAddress 寄存器地址
 * @return 读取到的数据
 */
uint8_t MPU6050_ReadReg(uint8_t RegAddress)
{
    uint8_t Data;
    HAL_I2C_Mem_Read(&hi2c2, MPU6050_ADDRESS, RegAddress, I2C_MEMADD_SIZE_8BIT, &Data, 1, HAL_MAX_DELAY);
    return Data;
}

/**
 * @brief MPU6050初始化
 * @note  假设I2C2已由HAL初始化（如MX_I2C2_Init），本函数仅配置传感器寄存器
 */
void MPU6050_Init(void)
{
    MPU6050_WriteReg(MPU6050_PWR_MGMT_1, 0x01);   // 唤醒并选择时钟源
    MPU6050_WriteReg(MPU6050_PWR_MGMT_2, 0x00);   // 所有轴使能
    MPU6050_WriteReg(MPU6050_SMPLRT_DIV, 0x09);   // 采样率 = 1kHz/(1+9)=100Hz
    MPU6050_WriteReg(MPU6050_CONFIG, 0x06);       // DLPF带宽 = 21Hz
    MPU6050_WriteReg(MPU6050_GYRO_CONFIG, 0x18);  // 陀螺仪量程 ±2000°/s
    MPU6050_WriteReg(MPU6050_ACCEL_CONFIG, 0x18); // 加速度计量程 ±16g
}

/**
 * @brief 获取设备ID
 * @return WHO_AM_I寄存器的值（通常为0x68）
 */
uint8_t MPU6050_GetID(void)
{
    return MPU6050_ReadReg(MPU6050_WHO_AM_I);
}

/**
 * @brief 读取6轴原始数据
 * @param AccX  加速度X轴指针
 * @param AccY  加速度Y轴指针
 * @param AccZ  加速度Z轴指针
 * @param GyroX 陀螺仪X轴指针
 * @param GyroY 陀螺仪Y轴指针
 * @param GyroZ 陀螺仪Z轴指针
 */
void MPU6050_GetData(int16_t *AccX, int16_t *AccY, int16_t *AccZ,
                     int16_t *GyroX, int16_t *GyroY, int16_t *GyroZ)
{
    uint8_t DataH, DataL;

    DataH = MPU6050_ReadReg(MPU6050_ACCEL_XOUT_H);
    DataL = MPU6050_ReadReg(MPU6050_ACCEL_XOUT_L);
    *AccX = (DataH << 8) | DataL;

    DataH = MPU6050_ReadReg(MPU6050_ACCEL_YOUT_H);
    DataL = MPU6050_ReadReg(MPU6050_ACCEL_YOUT_L);
    *AccY = (DataH << 8) | DataL;

    DataH = MPU6050_ReadReg(MPU6050_ACCEL_ZOUT_H);
    DataL = MPU6050_ReadReg(MPU6050_ACCEL_ZOUT_L);
    *AccZ = (DataH << 8) | DataL;

    DataH = MPU6050_ReadReg(MPU6050_GYRO_XOUT_H);
    DataL = MPU6050_ReadReg(MPU6050_GYRO_XOUT_L);
    *GyroX = (DataH << 8) | DataL;

    DataH = MPU6050_ReadReg(MPU6050_GYRO_YOUT_H);
    DataL = MPU6050_ReadReg(MPU6050_GYRO_YOUT_L);
    *GyroY = (DataH << 8) | DataL;

    DataH = MPU6050_ReadReg(MPU6050_GYRO_ZOUT_H);
    DataL = MPU6050_ReadReg(MPU6050_GYRO_ZOUT_L);
    *GyroZ = (DataH << 8) | DataL;
}
