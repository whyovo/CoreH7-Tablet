/**
 * @file mpu6050.h
 * @author 菜菜why（B站：菜菜whyy）
 * @brief MPU6050 驱动 (支持 硬件/软件 I2C，支持 DMP/卡尔曼 切换)
 * @attention
 * - 请在 config.h 中定义 MPU6050_I2C_ENABLE 以启用此模块
 * - 在本文件中选择 I2C 模式 (MPU_USE_HARD_I2C / MPU_USE_SOFT_I2C)
 * - 在本文件中选择 解算方式 (MPU_USE_DMP / MPU_USE_KALMAN)
 */

#ifndef __MPU6050_H__
#define __MPU6050_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "config.h"

#ifdef MPU6050_I2C_ENABLE

/* ================= 用户配置区域 ================= */

/* 1. 驱动模式选择 (二选一) */
#define MPU_USE_HARD_I2C /*!< 使用硬件 I2C */
// #define MPU_USE_SOFT_I2C  /*!< 使用软件 I2C */

/* 2. 解算算法选择 (二选一) */
// #define MPU_USE_DMP /*!< 使用官方 DMP 库*/
#define MPU_USE_KALMAN    /*!< 使用卡尔曼滤波 (推荐！！！) */

/* 3. 硬件 I2C 配置 */
#ifdef MPU_USE_HARD_I2C
    extern I2C_HandleTypeDef hi2c1;
#define MPU_HARD_HANDLE hi2c1
#endif

/* 4. 软件 I2C 配置 */
#ifdef MPU_USE_SOFT_I2C
#define MPU_SOFT_SCL_PORT GPIOB
#define MPU_SOFT_SCL_PIN GPIO_PIN_9
#define MPU_SOFT_SDA_PORT GPIOB
#define MPU_SOFT_SDA_PIN GPIO_PIN_8
#define MPU_SOFT_DELAY_US 5 
#endif

    /* ================= 接口定义 ================= */

    /**
     * @brief MPU6050 初始化
     * @return 0:成功, 其他:失败
     */
    uint8_t MPU6050_Init(void);

    /**
     * @brief 获取欧拉角 (统一接口)
     * @param pitch: 俯仰角 (指针)
     * @param roll:  横滚角 (指针)
     * @param yaw:   航向角 (指针)
     * @return 0:成功, 其他:失败
     */
    uint8_t MPU6050_Get_Angle(float *pitch, float *roll, float *yaw);

/* ================= 导出变量 (仅卡尔曼模式下有效) ================= */
#ifdef MPU_USE_KALMAN
    extern short mpu_aacx, mpu_aacy, mpu_aacz;    // 加速度原始数据
    extern short mpu_gyrox, mpu_gyroy, mpu_gyroz; // 陀螺仪原始数据
#endif

#endif /* MPU6050_I2C_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* __MPU6050_H__ */
