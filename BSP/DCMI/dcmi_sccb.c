/**
 ******************************************************************************
 * @file    dcmi_sccb.c
 * @author  菜菜why(B站:菜菜whyy)
 * @brief   SCCB (Serial Camera Control Bus) 软件 I2C 驱动实现文件
 ******************************************************************************
 * @attention
 *
 * 实现说明：
 * 1. 使用 GPIO 模拟 I2C 时序 (软件 I2C)
 * 2. 支持 8位地址 (OV2640) 和 16位地址 (OV5640) 的读写操作
 * 3. 提供基础的 Start, Stop, ACK, Byte Write/Read 功能
 *
 ******************************************************************************
 */

#include "dcmi_sccb.h"
#ifdef OV5640_ENABLE
/*******************************************************************************
 *                              导出函数实现
 ******************************************************************************/

/**
 * @brief  初始化IIC的GPIO口,推挽输出
 * @note   由于IIC通信速度不高，这里的IO口速度配置为2M即可
 * @retval None
 */
void SCCB_GPIO_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	SCCB_SCL_CLK_ENABLE; // 初始化IO口时钟
	SCCB_SDA_CLK_ENABLE;

	GPIO_InitStruct.Pin = SCCB_SCL_PIN;			 // SCL引脚
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;	 // 开漏输出
	GPIO_InitStruct.Pull = GPIO_NOPULL;			 // 不带上下拉
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; // 速度等级
	HAL_GPIO_Init(SCCB_SCL_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = SCCB_SDA_PIN; // SDA引脚
	HAL_GPIO_Init(SCCB_SDA_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; // 推挽输出
	GPIO_InitStruct.Pull = GPIO_PULLUP;			// 上拉

	HAL_GPIO_WritePin(SCCB_SCL_PORT, SCCB_SCL_PIN, GPIO_PIN_SET); // SCL输出高电平
	HAL_GPIO_WritePin(SCCB_SDA_PORT, SCCB_SDA_PIN, GPIO_PIN_SET); // SDA输出高电平
}

/**
 * @brief  简单延时函数
 * @param  a: 延时时间
 * @note   为了移植的简便性且对延时精度要求不高，所以不需要使用定时器做延时
 * @retval None
 */
void SCCB_Delay(uint32_t a)
{
	volatile uint16_t i;
	while (a--)
	{
		for (i = 0; i < 6; i++)
			;
	}
}

/**
 * @brief  IIC起始信号
 * @note   在SCL处于高电平期间，SDA由高到低跳变为起始信号
 * @retval None
 */
void SCCB_Start(void)
{
	SCCB_SDA(1);
	SCCB_SCL(1);
	SCCB_Delay(SCCB_DelayVaule);

	SCCB_SDA(0);
	SCCB_Delay(SCCB_DelayVaule);
	SCCB_SCL(0);
	SCCB_Delay(SCCB_DelayVaule);
}

/**
 * @brief  IIC停止信号
 * @note   在SCL处于高电平期间，SDA由低到高跳变为停止信号
 * @retval None
 */
void SCCB_Stop(void)
{
	SCCB_SCL(0);
	SCCB_Delay(SCCB_DelayVaule);
	SCCB_SDA(0);
	SCCB_Delay(SCCB_DelayVaule);

	SCCB_SCL(1);
	SCCB_Delay(SCCB_DelayVaule);
	SCCB_SDA(1);
	SCCB_Delay(SCCB_DelayVaule);
}

/**
 * @brief  IIC应答信号
 * @note   在SCL为高电平期间，SDA引脚输出为低电平，产生应答信号
 * @retval None
 */
void SCCB_ACK(void)
{
	SCCB_SCL(0);
	SCCB_Delay(SCCB_DelayVaule);
	SCCB_SDA(0);
	SCCB_Delay(SCCB_DelayVaule);
	SCCB_SCL(1);
	SCCB_Delay(SCCB_DelayVaule);

	SCCB_SCL(0); // SCL输出低时，SDA应立即拉高，释放总线
	SCCB_SDA(1);

	SCCB_Delay(SCCB_DelayVaule);
}

/**
 * @brief  IIC非应答信号
 * @note   在SCL为高电平期间，若SDA引脚为高电平，产生非应答信号
 * @retval None
 */
void SCCB_NoACK(void)
{
	SCCB_SCL(0);
	SCCB_Delay(SCCB_DelayVaule);
	SCCB_SDA(1);
	SCCB_Delay(SCCB_DelayVaule);
	SCCB_SCL(1);
	SCCB_Delay(SCCB_DelayVaule);

	SCCB_SCL(0);
	SCCB_Delay(SCCB_DelayVaule);
}

/**
 * @brief  等待接收设备发出应答信号
 * @note   在SCL为高电平期间，若检测到SDA引脚为低电平，则接收设备响应正常
 * @retval ACK_OK: 应答正常, ACK_ERR: 无应答
 */
uint8_t SCCB_WaitACK(void)
{
	SCCB_SDA(1);
	SCCB_Delay(SCCB_DelayVaule);
	SCCB_SCL(1);
	SCCB_Delay(SCCB_DelayVaule);

	if (HAL_GPIO_ReadPin(SCCB_SDA_PORT, SCCB_SDA_PIN) != 0) // 判断设备是否有做出响应
	{
		SCCB_SCL(0);
		SCCB_Delay(SCCB_DelayVaule);
		return ACK_ERR; // 无应答
	}
	else
	{
		SCCB_SCL(0);
		SCCB_Delay(SCCB_DelayVaule);
		return ACK_OK; // 应答正常
	}
}

/**
 * @brief  写一字节数据
 * @param  IIC_Data: 要写入的8位数据
 * @note   高位在前
 * @retval ACK_OK: 设备响应正常, ACK_ERR: 设备响应错误
 */
uint8_t SCCB_WriteByte(uint8_t IIC_Data)
{
	uint8_t i;

	for (i = 0; i < 8; i++)
	{
		SCCB_SDA(IIC_Data & 0x80);

		SCCB_Delay(SCCB_DelayVaule);
		SCCB_SCL(1);
		SCCB_Delay(SCCB_DelayVaule);
		SCCB_SCL(0);
		if (i == 7)
		{
			SCCB_SDA(1);
		}
		IIC_Data <<= 1;
	}

	return SCCB_WaitACK(); // 等待设备响应
}

/**
 * @brief  读一字节数据
 * @param  ACK_Mode: 响应模式，输入1则发出应答信号，输入0发出非应答信号
 * @note   1.高位在前
 *         2.应在主机接收最后一字节数据时发送非应答信号
 * @retval 读到的数据
 */
uint8_t SCCB_ReadByte(uint8_t ACK_Mode)
{
	uint8_t IIC_Data = 0;
	uint8_t i = 0;

	for (i = 0; i < 8; i++)
	{
		IIC_Data <<= 1;

		SCCB_SCL(1);
		SCCB_Delay(SCCB_DelayVaule);
		IIC_Data |= (HAL_GPIO_ReadPin(SCCB_SDA_PORT, SCCB_SDA_PIN) & 0x01);
		SCCB_SCL(0);
		SCCB_Delay(SCCB_DelayVaule);
	}

	if (ACK_Mode == 1) //	应答信号
		SCCB_ACK();
	else
		SCCB_NoACK(); // 非应答信号

	return IIC_Data;
}

/**
 * @brief  对指定的寄存器(8位地址)执行写操作，OV2640用到
 * @param  addr: 要进行操作的寄存器(8位地址)
 * @retval SUCCESS: 操作成功, ERROR: 操作失败
 */
uint8_t SCCB_WriteHandle(uint8_t addr)
{
	uint8_t status; // 状态标志位

	SCCB_Start();										 // 启动IIC通信
	if (SCCB_WriteByte(OV2640_DEVICE_ADDRESS) == ACK_OK) // 写数据指令
	{
		if (SCCB_WriteByte((uint8_t)(addr)) != ACK_OK)
		{
			status = ERROR; // 操作失败
		}
	}
	status = SUCCESS; // 操作成功
	return status;
}

/**
 * @brief  对指定的寄存器(8位地址)写一字节数据，OV2640用到
 * @param  addr: 要写入的寄存器(8位地址)
 * @param  value: 要写入的数据
 * @retval SUCCESS: 操作成功, ERROR: 操作失败
 */
uint8_t SCCB_WriteReg(uint8_t addr, uint8_t value)
{
	uint8_t status;

	SCCB_Start(); // 启动IIC通讯

	if (SCCB_WriteHandle(addr) == SUCCESS) // 写入要操作的寄存器
	{
		if (SCCB_WriteByte(value) != ACK_OK) // 写数据
		{
			status = ERROR;
		}
	}
	SCCB_Stop(); // 停止通讯

	status = SUCCESS; // 写入成功
	return status;
}

/**
 * @brief  对指定的寄存器(8位地址)读取一字节数据，OV2640用到
 * @param  addr: 要读取的寄存器(8位地址)
 * @retval 读到的数据
 */
uint8_t SCCB_ReadReg(uint8_t addr)
{
	uint8_t value = 0;

	SCCB_Start(); // 启动IIC通信

	if (SCCB_WriteHandle(addr) == SUCCESS) // 写入要操作的寄存器
	{
		SCCB_Stop();  // 停止IIC通信
		SCCB_Start(); // 重新启动IIC通讯

		if (SCCB_WriteByte(OV2640_DEVICE_ADDRESS | 0X01) == ACK_OK) // 发送读命令
		{
			value = SCCB_ReadByte(0); // 读到最后一个数据时发送 非应答信号
		}
		SCCB_Stop(); // 停止IIC通信
	}

	return value;
}

/**
 * @brief  对指定的寄存器(16位地址)执行写操作，OV5640用到
 * @param  addr: 要进行操作的寄存器(16位地址)
 * @retval SUCCESS: 操作成功, ERROR: 操作失败
 */
uint8_t SCCB_WriteHandle_16Bit(uint16_t addr)
{
	uint8_t status; // 状态标志位

	SCCB_Start();										 // 启动IIC通信
	if (SCCB_WriteByte(OV5640_DEVICE_ADDRESS) == ACK_OK) // 写数据指令
	{
		if (SCCB_WriteByte((uint8_t)(addr >> 8)) == ACK_OK) // 写入16位地址
		{
			if (SCCB_WriteByte((uint8_t)(addr)) != ACK_OK)
			{
				status = ERROR; // 操作失败
			}
		}
	}
	status = SUCCESS; // 操作成功
	return status;
}

/**
 * @brief  对指定的寄存器(16位地址)写一字节数据，OV5640用到
 * @param  addr: 要写入的寄存器(16位地址)
 * @param  value: 要写入的数据
 * @retval SUCCESS: 操作成功, ERROR: 操作失败
 */
uint8_t SCCB_WriteReg_16Bit(uint16_t addr, uint8_t value)
{
	uint8_t status;

	SCCB_Start(); // 启动IIC通讯

	if (SCCB_WriteHandle_16Bit(addr) == SUCCESS) // 写入要操作的寄存器
	{
		if (SCCB_WriteByte(value) != ACK_OK) // 写数据
		{
			status = ERROR;
		}
	}
	SCCB_Stop(); // 停止通讯

	status = SUCCESS; // 写入成功
	return status;
}

/**
 * @brief  对指定的寄存器(16位地址)读取一字节数据，OV5640用到
 * @param  addr: 要读取的寄存器(16位地址)
 * @retval 读到的数据
 */
uint8_t SCCB_ReadReg_16Bit(uint16_t addr)
{
	uint8_t value = 0;

	SCCB_Start(); // 启动IIC通信

	if (SCCB_WriteHandle_16Bit(addr) == SUCCESS) // 写入要操作的寄存器
	{
		SCCB_Stop();  // 停止IIC通信
		SCCB_Start(); // 重新启动IIC通讯

		if (SCCB_WriteByte(OV5640_DEVICE_ADDRESS | 0X01) == ACK_OK) // 发送读命令
		{
			value = SCCB_ReadByte(0); // 读到最后一个数据时发送 非应答信号
		}
		SCCB_Stop(); // 停止IIC通信
	}

	return value;
}

/**
 * @brief  对指定的寄存器(16位地址)批量写数据，OV5640 写入自动对焦固件时用到
 * @param  addr: 要写入的寄存器(16位地址)
 * @param  pData: 数据区
 * @param  size: 要传输数据的大小
 * @retval SUCCESS: 操作成功, ERROR: 操作失败
 */
uint8_t SCCB_WriteBuffer_16Bit(uint16_t addr, uint8_t *pData, uint32_t size)
{
	uint8_t status;
	uint32_t i;

	SCCB_Start(); // 启动IIC通讯

	if (SCCB_WriteHandle_16Bit(addr) == SUCCESS) // 写入要操作的寄存器
	{
		for (i = 0; i < size; i++)
		{
			SCCB_WriteByte(*pData); // 写数据
			pData++;
		}
	}
	SCCB_Stop(); // 停止通讯

	status = SUCCESS; // 写入成功
	return status;
}

#endif

