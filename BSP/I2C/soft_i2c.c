/**
 *******************************************************************************
 * @file    soft_i2c.c
 * @author  菜菜why(B站:菜菜whyy)
 * @brief   软件 I2C 中间层实现
 *******************************************************************************
 * @attention
 *
 * 本文件实现：
 * - I2C 基本时序：START、STOP、ACK、NACK
 * - 字节读写：WriteByte、ReadByte
 * - 两套 API：句柄方式 + 直接引脚方式
 *
 * 技术要点：
 * - 使用 GPIO 开漏模式模拟 I2C 总线
 * - 严格遵循 I2C 时序要求（数据稳定时钟采样）
 * - 超时保护机制（防止死循环）
 *
 *******************************************************************************
 */

#include "soft_i2c.h"
#if defined(OLED_I2C_ENABLE) || defined(MPU6050_I2C_ENABLE)
/*******************************************************************************
 *                              内部辅助函数
 ******************************************************************************/

/**
 * @brief  内部：初始化 GPIO 为开漏输出模式
 */
static void __gpio_init(GPIO_TypeDef *port, uint16_t pin)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* 1. 开启 GPIO 时钟 */
  if (port == GPIOA)
    __HAL_RCC_GPIOA_CLK_ENABLE();
  else if (port == GPIOB)
    __HAL_RCC_GPIOB_CLK_ENABLE();
  else if (port == GPIOC)
    __HAL_RCC_GPIOC_CLK_ENABLE();
  else if (port == GPIOD)
    __HAL_RCC_GPIOD_CLK_ENABLE();
  else if (port == GPIOE)
    __HAL_RCC_GPIOE_CLK_ENABLE();
  else if (port == GPIOF)
    __HAL_RCC_GPIOF_CLK_ENABLE();
  else if (port == GPIOG)
    __HAL_RCC_GPIOG_CLK_ENABLE();
  else if (port == GPIOH)
    __HAL_RCC_GPIOH_CLK_ENABLE();
#if defined(GPIOI)
  else if (port == GPIOI)
    __HAL_RCC_GPIOI_CLK_ENABLE();
#endif
#if defined(GPIOJ)
  else if (port == GPIOJ)
    __HAL_RCC_GPIOJ_CLK_ENABLE();
#endif
#if defined(GPIOK)
  else if (port == GPIOK)
    __HAL_RCC_GPIOK_CLK_ENABLE();
#endif

  /* 2. 配置引脚模式 */
  GPIO_InitStruct.Pin = pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD; /* 开漏输出 */
  GPIO_InitStruct.Pull = GPIO_PULLUP;         /* 上拉（I2C总线必须上拉） */
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(port, &GPIO_InitStruct);

  /* 3. 默认输出高电平（释放总线） */
  HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
}

/**
 * @brief  内部：写 SCL 引脚并延时
 */
static inline void __scl_write(GPIO_TypeDef *port, uint16_t pin, uint8_t state,
                               uint32_t d)
{
  HAL_GPIO_WritePin(port, pin, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
  Delay_us(d);
}

/**
 * @brief  内部：写 SDA 引脚并延时
 */
static inline void __sda_write(GPIO_TypeDef *port, uint16_t pin, uint8_t state,
                               uint32_t d)
{
  HAL_GPIO_WritePin(port, pin, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
  Delay_us(d);
}

/**
 * @brief  内部：读 SDA 引脚电平
 * @retval 0 或 1
 */
static inline uint8_t __sda_read(GPIO_TypeDef *port, uint16_t pin)
{
  return (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET) ? 1 : 0;
}

/*******************************************************************************
 *                              基于句柄的实现
 ******************************************************************************/

/**
 * @brief  配置软件 I2C 句柄
 */
void SOFT_I2C_ConfigHandle(SOFT_I2C_Handle *h, GPIO_TypeDef *scl_port,
                           uint16_t scl_pin, GPIO_TypeDef *sda_port,
                           uint16_t sda_pin, uint32_t delay_us)
{
  if (h == NULL)
  {
    DEBUG_ERROR("SOFT_I2C_ConfigHandle: 句柄指针为空");
    return;
  }

  h->scl_port = scl_port;
  h->scl_pin = scl_pin;
  h->sda_port = sda_port;
  h->sda_pin = sda_pin;
  h->delay_us = (delay_us == 0) ? SOFT_I2C_DEFAULT_DELAY_US : delay_us;

  /* 自动初始化 GPIO 引脚，无需 CubeMX 配置 */
  __gpio_init(scl_port, scl_pin);
  __gpio_init(sda_port, sda_pin);
}

/**
 * @brief  发送起始信号
 * @note   时序：SDA 从高到低，SCL 保持高电平
 */
void SOFT_I2C_Start(SOFT_I2C_Handle *h)
{
  if (h == NULL)
  {
    DEBUG_ERROR("SOFT_I2C_Start: 句柄指针为空");
    return;
  }
  __sda_write(h->sda_port, h->sda_pin, 1, h->delay_us); /* SDA = 1 */
  __scl_write(h->scl_port, h->scl_pin, 1, h->delay_us); /* SCL = 1 */
  __sda_write(h->sda_port, h->sda_pin, 0,
              h->delay_us); /* SDA 下降沿（START 信号） */
  __scl_write(h->scl_port, h->scl_pin, 0,
              h->delay_us); /* SCL = 0（为数据传输做准备） */
}

/**
 * @brief  发送停止信号
 * @note   时序：SDA 从低到高，SCL 保持高电平
 */
void SOFT_I2C_Stop(SOFT_I2C_Handle *h)
{
  if (h == NULL)
  {
    DEBUG_ERROR("SOFT_I2C_Stop: 句柄指针为空");
    return;
  }
  __sda_write(h->sda_port, h->sda_pin, 0, h->delay_us); /* SDA = 0 */
  __scl_write(h->scl_port, h->scl_pin, 1, h->delay_us); /* SCL = 1 */
  __sda_write(h->sda_port, h->sda_pin, 1,
              h->delay_us); /* SDA 上升沿（STOP 信号） */
}

/**
 * @brief  等待从机应答
 * @note   释放 SDA，拉高 SCL 并读取 SDA
 * @retval 0=收到 ACK（SDA=0），1=收到 NACK 或超时
 */
uint8_t SOFT_I2C_WaitAck(SOFT_I2C_Handle *h)
{
  if (h == NULL)
  {
    DEBUG_ERROR("SOFT_I2C_WaitAck: 句柄指针为空");
    return 1;
  }

  uint32_t timeout = 0;

  /* 释放 SDA（开漏模式，写 1 即为释放总线） */
  __sda_write(h->sda_port, h->sda_pin, 1, h->delay_us);
  /* 拉高 SCL */
  __scl_write(h->scl_port, h->scl_pin, 1, h->delay_us);

  /* 等待 SDA 被从机拉低（ACK） */
  while (__sda_read(h->sda_port, h->sda_pin))
  {
    if (++timeout > 5000) /* 超时保护（循环计数，粗略） */
    {
      DEBUG_ERROR("SOFT_I2C_WaitAck: 超时");
      SOFT_I2C_Stop(h); /* 超时则发送停止信号 */
      return 1;         /* NACK/超时 */
    }
  }

  /* 拉低 SCL（结束应答位） */
  __scl_write(h->scl_port, h->scl_pin, 0, h->delay_us);
  return 0; /* ACK */
}

/**
 * @brief  主机发送应答（ACK）
 * @note   SDA=0，SCL 产生时钟脉冲
 */
void SOFT_I2C_SendAck(SOFT_I2C_Handle *h)
{
  if (h == NULL)
  {
    DEBUG_ERROR("SOFT_I2C_SendAck: 句柄指针为空");
    return;
  }
  __scl_write(h->scl_port, h->scl_pin, 0, h->delay_us);
  __sda_write(h->sda_port, h->sda_pin, 0, h->delay_us); /* SDA = 0（ACK） */
  __scl_write(h->scl_port, h->scl_pin, 1, h->delay_us); /* SCL 脉冲 */
  __scl_write(h->scl_port, h->scl_pin, 0, h->delay_us);
}

/**
 * @brief  主机发送非应答（NACK）
 * @note   SDA=1，SCL 产生时钟脉冲
 */
void SOFT_I2C_SendNack(SOFT_I2C_Handle *h)
{
  if (h == NULL)
  {
    DEBUG_ERROR("SOFT_I2C_SendNack: 句柄指针为空");
    return;
  }
  __scl_write(h->scl_port, h->scl_pin, 0, h->delay_us);
  __sda_write(h->sda_port, h->sda_pin, 1, h->delay_us); /* SDA = 1（NACK） */
  __scl_write(h->scl_port, h->scl_pin, 1, h->delay_us); /* SCL 脉冲 */
  __scl_write(h->scl_port, h->scl_pin, 0, h->delay_us);
}

/**
 * @brief  写一个字节
 * @note   MSB 先发送，SCL 低电平时改变 SDA，高电平时从机采样
 */
void SOFT_I2C_WriteByte(SOFT_I2C_Handle *h, uint8_t byte)
{
  if (h == NULL)
  {
    DEBUG_ERROR("SOFT_I2C_WriteByte: 句柄指针为空");
    return;
  }

  for (uint8_t i = 0; i < 8; ++i)
  {
    __scl_write(h->scl_port, h->scl_pin, 0, h->delay_us); /* SCL = 0 */
    __sda_write(h->sda_port, h->sda_pin, (byte & 0x80) ? 1 : 0,
                h->delay_us); /* 写最高位 */
    __scl_write(h->scl_port, h->scl_pin, 1,
                h->delay_us); /* SCL = 1（从机采样） */
    byte <<= 1;               /* 左移准备下一位 */
  }
  __scl_write(h->scl_port, h->scl_pin, 0,
              h->delay_us); /* SCL = 0（为应答位做准备） */
}

/**
 * @brief  读一个字节
 * @param  ack: 读取后是否发送 ACK（1=ACK, 0=NACK）
 * @note   MSB 先接收，SCL 高电平时主机采样 SDA
 * @retval 读取到的字节
 */
uint8_t SOFT_I2C_ReadByte(SOFT_I2C_Handle *h, uint8_t ack)
{
  if (h == NULL)
  {
    DEBUG_ERROR("SOFT_I2C_ReadByte: 句柄指针为空");
    return 0;
  }

  uint8_t val = 0;

  /* 释放 SDA（开漏模式，由从机控制） */
  __sda_write(h->sda_port, h->sda_pin, 1, h->delay_us);

  for (uint8_t i = 0; i < 8; ++i)
  {
    __scl_write(h->scl_port, h->scl_pin, 0, h->delay_us); /* SCL = 0 */
    __scl_write(h->scl_port, h->scl_pin, 1,
                h->delay_us); /* SCL = 1（主机采样） */
    val <<= 1;
    if (__sda_read(h->sda_port, h->sda_pin))
      val |= 1; /* 读取 SDA */
  }
  __scl_write(h->scl_port, h->scl_pin, 0, h->delay_us); /* SCL = 0 */

  /* 发送应答 */
  if (ack)
    SOFT_I2C_SendAck(h);
  else
    SOFT_I2C_SendNack(h);

  return val;
}

/*******************************************************************************
 *                              直接引脚 API（调用句柄版本）
 ******************************************************************************/

void SOFT_I2C_Start_Pins(GPIO_TypeDef *scl_port, uint16_t scl_pin,
                         GPIO_TypeDef *sda_port, uint16_t sda_pin,
                         uint32_t delay_us)
{
  SOFT_I2C_Handle h;
  SOFT_I2C_ConfigHandle(&h, scl_port, scl_pin, sda_port, sda_pin, delay_us);
  SOFT_I2C_Start(&h);
}

void SOFT_I2C_Stop_Pins(GPIO_TypeDef *scl_port, uint16_t scl_pin,
                        GPIO_TypeDef *sda_port, uint16_t sda_pin,
                        uint32_t delay_us)
{
  SOFT_I2C_Handle h;
  SOFT_I2C_ConfigHandle(&h, scl_port, scl_pin, sda_port, sda_pin, delay_us);
  SOFT_I2C_Stop(&h);
}

uint8_t SOFT_I2C_WaitAck_Pins(GPIO_TypeDef *scl_port, uint16_t scl_pin,
                              GPIO_TypeDef *sda_port, uint16_t sda_pin,
                              uint32_t delay_us)
{
  SOFT_I2C_Handle h;
  SOFT_I2C_ConfigHandle(&h, scl_port, scl_pin, sda_port, sda_pin, delay_us);
  return SOFT_I2C_WaitAck(&h);
}

void SOFT_I2C_SendAck_Pins(GPIO_TypeDef *scl_port, uint16_t scl_pin,
                           GPIO_TypeDef *sda_port, uint16_t sda_pin,
                           uint32_t delay_us)
{
  SOFT_I2C_Handle h;
  SOFT_I2C_ConfigHandle(&h, scl_port, scl_pin, sda_port, sda_pin, delay_us);
  SOFT_I2C_SendAck(&h);
}

void SOFT_I2C_SendNack_Pins(GPIO_TypeDef *scl_port, uint16_t scl_pin,
                            GPIO_TypeDef *sda_port, uint16_t sda_pin,
                            uint32_t delay_us)
{
  SOFT_I2C_Handle h;
  SOFT_I2C_ConfigHandle(&h, scl_port, scl_pin, sda_port, sda_pin, delay_us);
  SOFT_I2C_SendNack(&h);
}

void SOFT_I2C_WriteByte_Pins(GPIO_TypeDef *scl_port, uint16_t scl_pin,
                             GPIO_TypeDef *sda_port, uint16_t sda_pin,
                             uint8_t byte, uint32_t delay_us)
{
  SOFT_I2C_Handle h;
  SOFT_I2C_ConfigHandle(&h, scl_port, scl_pin, sda_port, sda_pin, delay_us);
  SOFT_I2C_WriteByte(&h, byte);
}

uint8_t SOFT_I2C_ReadByte_Pins(GPIO_TypeDef *scl_port, uint16_t scl_pin,
                               GPIO_TypeDef *sda_port, uint16_t sda_pin,
                               uint8_t ack, uint32_t delay_us)
{
  SOFT_I2C_Handle h;
  SOFT_I2C_ConfigHandle(&h, scl_port, scl_pin, sda_port, sda_pin, delay_us);
  return SOFT_I2C_ReadByte(&h, ack);
}



#endif
