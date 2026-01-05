/**
 ******************************************************************************
 * @file    dcmi_sccb.h
 * @author  菜菜why(B站:菜菜whyy)
 * @brief   SCCB (Serial Camera Control Bus) I2C 驱动头文件
 ******************************************************************************
 * @attention
 *
 * 使用方法:
 * - SCCB_GPIO_Config() 初始化 I2C 引脚
 * - SCCB_WriteReg_16Bit() 写入 16位地址寄存器
 * - SCCB_ReadReg_16Bit() 读取 16位地址寄存器
 * - SCCB_WriteBuffer_16Bit() 批量写入数据(用于固件下载)
 *
 ******************************************************************************
 */

#ifndef __DCMI_SCCB_H
#define __DCMI_SCCB_H

#ifdef __cplusplus
extern "C" {
#endif


#include "config.h"
#ifdef OV5640_ENABLE

/* 摄像头 I2C 地址定义 */
#define OV2640_DEVICE_ADDRESS 0x60 /*!< OV2640 I2C 地址 */
#define OV5640_DEVICE_ADDRESS 0X78 /*!< OV5640 I2C 地址 */

/* ============================================================================ */
/*                              I2C 引脚配置                                   */
/* ============================================================================ */

#define SCCB_SCL_CLK_ENABLE __HAL_RCC_GPIOG_CLK_ENABLE() /*!< SCL 时钟使能 */
#define SCCB_SCL_PORT GPIOG								 /*!< SCL 端口 */
#define SCCB_SCL_PIN GPIO_PIN_2							 /*!< SCL 引脚 */

#define SCCB_SDA_CLK_ENABLE __HAL_RCC_GPIOG_CLK_ENABLE() /*!< SDA 时钟使能 */
#define SCCB_SDA_PORT GPIOG								 /*!< SDA 端口 */
#define SCCB_SDA_PIN GPIO_PIN_3							 /*!< SDA 引脚 */

/* ============================================================================ */
/*                              I2C 通信定义                                   */
/* ============================================================================ */

#define ACK_OK 1  /*!< 应答成功 */
#define ACK_ERR 0 /*!< 应答失败 */

#define SCCB_DelayVaule 8 /*!< I2C 延时值，通信速度约 300KHz */

/* ============================================================================ */
/*                              IO 口操作宏                                    */
/* ============================================================================ */

#define SCCB_SCL(a)                                                   \
	if (a)                                                            \
		HAL_GPIO_WritePin(SCCB_SCL_PORT, SCCB_SCL_PIN, GPIO_PIN_SET); \
	else                                                              \
		HAL_GPIO_WritePin(SCCB_SCL_PORT, SCCB_SCL_PIN, GPIO_PIN_RESET)

#define SCCB_SDA(a)                                                   \
	if (a)                                                            \
		HAL_GPIO_WritePin(SCCB_SDA_PORT, SCCB_SDA_PIN, GPIO_PIN_SET); \
	else                                                              \
		HAL_GPIO_WritePin(SCCB_SDA_PORT, SCCB_SDA_PIN, GPIO_PIN_RESET)

/* ============================================================================ */
/*                              导出函数声明                                   */
/* ============================================================================ */

/**
 * @brief  初始化 I2C 引脚
 * @retval None
 */
void SCCB_GPIO_Config(void);

/**
 * @brief  I2C 延时函数
 * @param  a: 延时参数
 * @retval None
 */
void SCCB_Delay(uint32_t a);

/**
 * @brief  I2C 起始信号
 * @retval None
 */
void SCCB_Start(void);

/**
 * @brief  I2C 停止信号
 * @retval None
 */
void SCCB_Stop(void);

/**
 * @brief  I2C 应答信号
 * @retval None
 */
void SCCB_ACK(void);

/**
 * @brief  I2C 非应答信号
 * @retval None
 */
void SCCB_NoACK(void);

/**
 * @brief  等待 I2C 应答信号
 * @retval ACK_OK: 应答成功，ACK_ERR: 应答失败
 */
uint8_t SCCB_WaitACK(void);

/**
 * @brief  写入一字节数据
 * @param  IIC_Data: 要写入的数据(8位)
 * @retval ACK_OK: 应答成功，ACK_ERR: 应答失败
 */
uint8_t SCCB_WriteByte(uint8_t IIC_Data);

/**
 * @brief  读取一字节数据
 * @param  ACK_Mode: 响应模式 (1=应答, 0=非应答)
 * @retval 读取的数据(8位)
 * @note   应在主机接收最后一字节时发送非应答信号
 */
uint8_t SCCB_ReadByte(uint8_t ACK_Mode);

/**
 * @brief  对 8位地址寄存器写入一字节数据 (OV2640 用)
 * @param  addr: 寄存器地址(8位)
 * @param  value: 要写入的数据
 * @retval 操作结果
 */
uint8_t SCCB_WriteReg(uint8_t addr, uint8_t value);

/**
 * @brief  读取 8位地址寄存器 (OV2640 用)
 * @param  addr: 寄存器地址(8位)
 * @retval 读取的数据
 */
uint8_t SCCB_ReadReg(uint8_t addr);

/**
 * @brief  对 16位地址寄存器写入一字节数据 (OV5640 用)
 * @param  addr: 寄存器地址(16位)
 * @param  value: 要写入的数据
 * @retval 操作结果
 */
uint8_t SCCB_WriteReg_16Bit(uint16_t addr, uint8_t value);

/**
 * @brief  读取 16位地址寄存器 (OV5640 用)
 * @param  addr: 寄存器地址(16位)
 * @retval 读取的数据
 */
uint8_t SCCB_ReadReg_16Bit(uint16_t addr);

/**
 * @brief  批量写入数据到 16位地址寄存器 (OV5640 固件下载用)
 * @param  addr: 寄存器地址(16位)
 * @param  pData: 数据缓冲区指针
 * @param  size: 数据大小(字节数)
 * @retval 操作结果
 */
uint8_t SCCB_WriteBuffer_16Bit(uint16_t addr, uint8_t *pData, uint32_t size);

#endif

#ifdef __cplusplus
}
#endif

#endif // !__DCMI_SCCB_H
