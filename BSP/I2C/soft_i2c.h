/**
 *******************************************************************************
 * @file    soft_i2c.h
 * @author  菜菜why(B站:菜菜whyy)
 * @brief   软件 I2C 中间层头文件
 *******************************************************************************
 * @attention
 *
 * 本文件提供两套 API:
 * 1. 基于句柄（SOFT_I2C_Handle）的 API，适用于频繁操作同一总线
 * 2. 直接引脚（_Pins）的 API，适用于临时/一次性操作
 *
 * 使用方法：
 * - 方式1（推荐）：使用句柄
 *   SOFT_I2C_Handle h;
 *   SOFT_I2C_ConfigHandle(&h, GPIOB, GPIO_PIN_6, GPIOB, GPIO_PIN_7, 5);
 *   SOFT_I2C_Start(&h);
 *   SOFT_I2C_WriteByte(&h, 0xA0);
 *   ...
 *
 * - 方式2：直接传引脚
 *   SOFT_I2C_Start_Pins(GPIOB, GPIO_PIN_6, GPIOB, GPIO_PIN_7, 5);
 *   SOFT_I2C_WriteByte_Pins(GPIOB, GPIO_PIN_6, GPIOB, GPIO_PIN_7, 0xA0, 5);
 *   ...
 *
 *******************************************************************************
 */

#ifndef SOFT_I2C_H
#define SOFT_I2C_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"

#if defined(OLED_I2C_ENABLE) || defined(MPU6050_I2C_ENABLE)
/*******************************************************************************
 *                              默认参数配置
 ******************************************************************************/
/**
 * @brief  默认时序延时（微秒）
 * @note   根据 I2C 速率调整：标准模式100kHz建议5us，快速模式400kHz建议2us
 */
#ifndef SOFT_I2C_DEFAULT_DELAY_US
#define SOFT_I2C_DEFAULT_DELAY_US 5
#endif

/*******************************************************************************
 *                              数据类型定义
 ******************************************************************************/

/**
 * @brief  软件 I2C 句柄结构体
 * @note   用于简化多次操作同一 I2C 总线时的参数传递
 */
typedef struct {
  GPIO_TypeDef *scl_port; /*!< SCL 引脚端口 */
  uint16_t scl_pin;       /*!< SCL 引脚编号 */
  GPIO_TypeDef *sda_port; /*!< SDA 引脚端口 */
  uint16_t sda_pin;       /*!< SDA 引脚编号 */
  uint32_t delay_us;      /*!< 时序延时（微秒） */
} SOFT_I2C_Handle;

/*******************************************************************************
 *                              基于句柄的 API
 ******************************************************************************/

/**
 * @brief  配置软件 I2C 句柄
 * @param  h: 句柄指针
 * @param  scl_port: SCL 引脚端口
 * @param  scl_pin: SCL 引脚编号
 * @param  sda_port: SDA 引脚端口
 * @param  sda_pin: SDA 引脚编号
 * @param  delay_us: 时序延时（微秒），0则使用默认值
 * @retval None
 */
void SOFT_I2C_ConfigHandle(SOFT_I2C_Handle *h, GPIO_TypeDef *scl_port,
                           uint16_t scl_pin, GPIO_TypeDef *sda_port,
                           uint16_t sda_pin, uint32_t delay_us);

/**
 * @brief  发送起始信号
 * @param  h: 句柄指针
 * @retval None
 */
void SOFT_I2C_Start(SOFT_I2C_Handle *h);

/**
 * @brief  发送停止信号
 * @param  h: 句柄指针
 * @retval None
 */
void SOFT_I2C_Stop(SOFT_I2C_Handle *h);

/**
 * @brief  等待从机应答
 * @param  h: 句柄指针
 * @retval 0=收到 ACK, 1=收到 NACK 或超时
 */
uint8_t SOFT_I2C_WaitAck(SOFT_I2C_Handle *h);

/**
 * @brief  主机发送应答（ACK）
 * @param  h: 句柄指针
 * @retval None
 */
void SOFT_I2C_SendAck(SOFT_I2C_Handle *h);

/**
 * @brief  主机发送非应答（NACK）
 * @param  h: 句柄指针
 * @retval None
 */
void SOFT_I2C_SendNack(SOFT_I2C_Handle *h);

/**
 * @brief  写一个字节
 * @param  h: 句柄指针
 * @param  byte: 要发送的字节
 * @retval None
 */
void SOFT_I2C_WriteByte(SOFT_I2C_Handle *h, uint8_t byte);

/**
 * @brief  读一个字节
 * @param  h: 句柄指针
 * @param  ack: 读取后是否发送 ACK（1=ACK, 0=NACK）
 * @retval 读取到的字节
 */
uint8_t SOFT_I2C_ReadByte(SOFT_I2C_Handle *h, uint8_t ack);

/*******************************************************************************
 *                              直接引脚 API
 ******************************************************************************/

/**
 * @brief  发送起始信号（直接传引脚）
 * @param  scl_port: SCL 引脚端口
 * @param  scl_pin: SCL 引脚编号
 * @param  sda_port: SDA 引脚端口
 * @param  sda_pin: SDA 引脚编号
 * @param  delay_us: 时序延时（微秒）
 * @retval None
 */
void SOFT_I2C_Start_Pins(GPIO_TypeDef *scl_port, uint16_t scl_pin,
                         GPIO_TypeDef *sda_port, uint16_t sda_pin,
                         uint32_t delay_us);

/**
 * @brief  发送停止信号（直接传引脚）
 */
void SOFT_I2C_Stop_Pins(GPIO_TypeDef *scl_port, uint16_t scl_pin,
                        GPIO_TypeDef *sda_port, uint16_t sda_pin,
                        uint32_t delay_us);

/**
 * @brief  等待从机应答（直接传引脚）
 * @retval 0=收到 ACK, 1=收到 NACK 或超时
 */
uint8_t SOFT_I2C_WaitAck_Pins(GPIO_TypeDef *scl_port, uint16_t scl_pin,
                              GPIO_TypeDef *sda_port, uint16_t sda_pin,
                              uint32_t delay_us);

/**
 * @brief  主机发送应答（直接传引脚）
 */
void SOFT_I2C_SendAck_Pins(GPIO_TypeDef *scl_port, uint16_t scl_pin,
                           GPIO_TypeDef *sda_port, uint16_t sda_pin,
                           uint32_t delay_us);

/**
 * @brief  主机发送非应答（直接传引脚）
 */
void SOFT_I2C_SendNack_Pins(GPIO_TypeDef *scl_port, uint16_t scl_pin,
                            GPIO_TypeDef *sda_port, uint16_t sda_pin,
                            uint32_t delay_us);

/**
 * @brief  写一个字节（直接传引脚）
 */
void SOFT_I2C_WriteByte_Pins(GPIO_TypeDef *scl_port, uint16_t scl_pin,
                             GPIO_TypeDef *sda_port, uint16_t sda_pin,
                             uint8_t byte, uint32_t delay_us);

/**
 * @brief  读一个字节（直接传引脚）
 * @param  ack: 读取后是否发送 ACK（1=ACK, 0=NACK）
 * @retval 读取到的字节
 */
uint8_t SOFT_I2C_ReadByte_Pins(GPIO_TypeDef *scl_port, uint16_t scl_pin,
                               GPIO_TypeDef *sda_port, uint16_t sda_pin,
                               uint8_t ack, uint32_t delay_us);


#endif

#ifdef __cplusplus
}
#endif

#endif // SOFT_I2C_H
