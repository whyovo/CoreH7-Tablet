/**
 ******************************************************************************
 * @file    touch_iic.h
 * @author  菜菜why（B站：菜菜whyy）
 * @brief   触摸屏IIC通信驱动头文件
 ******************************************************************************
 * @attention
 *
 * 本文件提供：
 * - 触摸屏模拟IIC接口驱动
 * - GPIO初始化配置
 * - IIC通信协议实现（启动/停止/应答/读写）
 * - 触摸屏INT/RST引脚管理
 *
 * 性能参数：
 * - IIC通信速度：100KHz
 * - 延迟单位：20个循环（可根据实际调整）
 * - 适用芯片：GT911 等电容式触摸屏
 *
 * 通信流程：
 * 1. 调用 Touch_IIC_GPIO_Config() 初始化GPIO
 * 2. 触摸屏触发INT中断，读取坐标数据
 * 3. 使用 Touch_IIC_WriteByte()/Touch_IIC_ReadByte() 进行数据交互
 * 4. 通过 Touch_INT_In()/Touch_INT_Out() 切换INT引脚工作模式
 *
 * IIC协议说明：
 * - 起始信号：SCL高时，SDA从高到低跳变
 * - 停止信号：SCL高时，SDA从低到高跳变
 * - 应答信号：SCL高时，SDA为低电平
 * - 非应答信号：SCL高时，SDA为高电平
 * - 数据传输：高位先行，SCL低时改变SDA电平，SCL高时读取SDA
 *
 ******************************************************************************
 */

#ifndef __TOUCH_IIC_H
#define __TOUCH_IIC_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "init.h"

/*******************************************************************************
 *                              IIC返回状态码
 *******************************************************************************/
#define IIC_OK 1	/*!< 操作成功 */
#define IIC_ERROR 0 /*!< 操作失败 */

/*******************************************************************************
 *                              GPIO引脚配置宏
 *******************************************************************************/

/* SCL引脚配置 */
#define Touch_IIC_SCL_CLK_ENABLE __HAL_RCC_GPIOH_CLK_ENABLE() /*!< SCL时钟使能 */
#define Touch_IIC_SCL_PORT GPIOH							  /*!< SCL端口 */
#define Touch_IIC_SCL_PIN GPIO_PIN_4						  /*!< SCL引脚（PH4） */

/* SDA引脚配置 */
#define Touch_IIC_SDA_CLK_ENABLE __HAL_RCC_GPIOH_CLK_ENABLE() /*!< SDA时钟使能 */
#define Touch_IIC_SDA_PORT GPIOH							  /*!< SDA端口 */
#define Touch_IIC_SDA_PIN GPIO_PIN_5						  /*!< SDA引脚（PH5） */

/* INT引脚配置（中断/数据引脚） */
#define Touch_INT_CLK_ENABLE __HAL_RCC_GPIOI_CLK_ENABLE() /*!< INT时钟使能 */
#define Touch_INT_PORT GPIOI							  /*!< INT端口 */
#define Touch_INT_PIN GPIO_PIN_11						  /*!< INT引脚（PI11） */

/* RST引脚配置（复位引脚） */
#define Touch_RST_CLK_ENABLE __HAL_RCC_GPIOI_CLK_ENABLE() /*!< RST时钟使能 */
#define Touch_RST_PORT GPIOI							  /*!< RST端口 */
#define Touch_RST_PIN GPIO_PIN_8						  /*!< RST引脚（PI8） */

	/*******************************************************************************
	 *                              IIC通信参数定义
	 *******************************************************************************/

#define ACK_OK 1  /*!< 应答正常 */
#define ACK_ERR 0 /*!< 应答错误/无应答 */

#define IIC_DelayVaule 20 /*!< IIC延时值（可调参数） */

/*******************************************************************************
 *                              IO口操作宏
 *******************************************************************************/

/**
 * @brief  SCL引脚电平控制宏
 * @param  a: 1-输出高电平，0-输出低电平
 */
#define Touch_IIC_SCL(a)                                                        \
	if (a)                                                                      \
		HAL_GPIO_WritePin(Touch_IIC_SCL_PORT, Touch_IIC_SCL_PIN, GPIO_PIN_SET); \
	else                                                                        \
		HAL_GPIO_WritePin(Touch_IIC_SCL_PORT, Touch_IIC_SCL_PIN, GPIO_PIN_RESET)

/**
 * @brief  SDA引脚电平控制宏
 * @param  a: 1-输出高电平，0-输出低电平
 */
#define Touch_IIC_SDA(a)                                                        \
	if (a)                                                                      \
		HAL_GPIO_WritePin(Touch_IIC_SDA_PORT, Touch_IIC_SDA_PIN, GPIO_PIN_SET); \
	else                                                                        \
		HAL_GPIO_WritePin(Touch_IIC_SDA_PORT, Touch_IIC_SDA_PIN, GPIO_PIN_RESET)

	/*******************************************************************************
	 *                              函数声明
	 *******************************************************************************/

	/**
	 * @brief  初始化IIC的GPIO口
	 * @retval 无
	 * @note   - 初始化SCL和SDA为开漏输出模式
	 *         - 初始化INT和RST为推挽输出模式
	 *         - 初始时SCL/SDA输出高电平，INT输出低电平，RST输出高电平
	 */
	void Touch_IIC_GPIO_Config(void);

	/**
	 * @brief  IIC延时函数
	 * @param  a: 延时时间（单位：IIC_DelayVaule循环次数）
	 * @retval 无
	 * @note   用于生成IIC时序延迟，不需要使用定时器
	 */
	void Touch_IIC_Delay(uint32_t a);

	/**
	 * @brief  配置INT引脚为输出模式
	 * @retval 无
	 * @note   用于触摸屏向MCU发送数据时的IIC模式
	 */
	void Touch_INT_Out(void);

	/**
	 * @brief  配置INT引脚为输入模式
	 * @retval 无
	 * @note   用于触摸屏接收MCU发送的数据时的IIC模式
	 */
	void Touch_INT_In(void);

	/**
	 * @brief  IIC起始信号
	 * @retval 无
	 * @note   在SCL处于高电平期间，SDA由高到低跳变产生起始信号
	 */
	void Touch_IIC_Start(void);

	/**
	 * @brief  IIC停止信号
	 * @retval 无
	 * @note   在SCL处于高电平期间，SDA由低到高跳变产生停止信号
	 */
	void Touch_IIC_Stop(void);

	/**
	 * @brief  发送IIC应答信号
	 * @retval 无
	 * @note   在SCL为高电平期间，SDA输出低电平产生应答信号
	 */
	void Touch_IIC_ACK(void);

	/**
	 * @brief  发送IIC非应答信号
	 * @retval 无
	 * @note   在SCL为高电平期间，SDA保持高电平产生非应答信号
	 */
	void Touch_IIC_NoACK(void);

	/**
	 * @brief  等待接收设备发出的应答信号
	 * @retval ACK_OK - 设备响应正常，SDA为低电平
	 * @retval ACK_ERR - 设备无响应，SDA仍为高电平
	 * @note   在SCL为高电平期间检测SDA电平
	 */
	uint8_t Touch_IIC_WaitACK(void);

	/**
	 * @brief  通过IIC写一字节数据
	 * @param  IIC_Data: 要写入的8位数据
	 * @retval ACK_OK - 写入成功，设备响应正常
	 * @retval ACK_ERR - 写入失败，设备无响应
	 * @note   - 高位先行
	 *         - 写入每一位时，先改变SDA，后产生SCL脉冲
	 *         - 最后一位之后释放SDA，等待设备应答
	 */
	uint8_t Touch_IIC_WriteByte(uint8_t IIC_Data);

	/**
	 * @brief  通过IIC读一字节数据
	 * @param  ACK_Mode: 应答模式
	 *                   - 1: 读取后发送应答信号，继续接收数据
	 *                   - 0: 读取后发送非应答信号，结束接收
	 * @retval 读取到的8位数据
	 * @note   - 高位先行
	 *         - 在SCL为高电平期间读取SDA电平
	 *         - 应在接收最后一字节数据时传入0，表示发送非应答信号
	 */
	uint8_t Touch_IIC_ReadByte(uint8_t ACK_Mode);

#ifdef __cplusplus
}
#endif

#endif /* __TOUCH_IIC_H */
