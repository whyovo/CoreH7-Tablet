/**
 ******************************************************************************
 * @file    touch_iic.c
 * @author  菜菜why（B站：菜菜whyy）
 * @brief   触摸屏IIC通信驱动实现文件
 ******************************************************************************
 */

#include "touch_iic.h"

/*******************************************************************************
 *                              GPIO初始化函数
 *******************************************************************************/

/**
 * @brief  初始化IIC的GPIO口
 * @param  无
 * @retval 无
 * @note   初始化SCL和SDA为开漏输出，INT和RST为推挽输出
 */
void Touch_IIC_GPIO_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* 使能GPIO时钟 */
	Touch_IIC_SCL_CLK_ENABLE; /* SCL时钟使能 */
	Touch_IIC_SDA_CLK_ENABLE; /* SDA时钟使能 */
	Touch_INT_CLK_ENABLE;	  /* INT时钟使能 */
	Touch_RST_CLK_ENABLE;	  /* RST时钟使能 */

	/* 配置SCL引脚（开漏输出） */
	GPIO_InitStruct.Pin = Touch_IIC_SCL_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;	 /* 开漏输出 */
	GPIO_InitStruct.Pull = GPIO_NOPULL;			 /* 不带上下拉 */
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; /* 低速 */
	HAL_GPIO_Init(Touch_IIC_SCL_PORT, &GPIO_InitStruct);

	/* 配置SDA引脚（开漏输出） */
	GPIO_InitStruct.Pin = Touch_IIC_SDA_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;	 /* 开漏输出 */
	GPIO_InitStruct.Pull = GPIO_NOPULL;			 /* 不带上下拉 */
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; /* 低速 */
	HAL_GPIO_Init(Touch_IIC_SDA_PORT, &GPIO_InitStruct);

	/* 配置INT引脚（推挽输出） */
	GPIO_InitStruct.Pin = Touch_INT_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;	 /* 推挽输出 */
	GPIO_InitStruct.Pull = GPIO_PULLUP;			 /* 上拉 */
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; /* 低速 */
	HAL_GPIO_Init(Touch_INT_PORT, &GPIO_InitStruct);

	/* 配置RST引脚（推挽输出） */
	GPIO_InitStruct.Pin = Touch_RST_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;	 /* 推挽输出 */
	GPIO_InitStruct.Pull = GPIO_PULLUP;			 /* 上拉 */
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; /* 低速 */
	HAL_GPIO_Init(Touch_RST_PORT, &GPIO_InitStruct);

	/* 初始化引脚电平 */
	HAL_GPIO_WritePin(Touch_IIC_SCL_PORT, Touch_IIC_SCL_PIN, GPIO_PIN_SET); /* SCL=1 */
	HAL_GPIO_WritePin(Touch_IIC_SDA_PORT, Touch_IIC_SDA_PIN, GPIO_PIN_SET); /* SDA=1 */
	HAL_GPIO_WritePin(Touch_INT_PORT, Touch_INT_PIN, GPIO_PIN_RESET);		/* INT=0 */
	HAL_GPIO_WritePin(Touch_RST_PORT, Touch_RST_PIN, GPIO_PIN_SET);			/* RST=1 */
}

/*******************************************************************************
 *                              延时函数
 *******************************************************************************/

/**
 * @brief  简单延时函数
 * @param  a: 延时时间（单位循环次数）
 * @retval 无
 * @note   用于产生IIC通信的时序延迟
 */
void Touch_IIC_Delay(uint32_t a)
{
	volatile uint16_t i;
	while (a--)
	{
		for (i = 0; i < 8; i++)
			;
	}
}

/*******************************************************************************
 *                              INT引脚模式切换函数
 *******************************************************************************/

/**
 * @brief  配置INT引脚为输出模式
 * @param  无
 * @retval 无
 * @note   用于MCU向触摸屏通过IIC发送数据时
 */
void Touch_INT_Out(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;	 /* 输出模式 */
	GPIO_InitStruct.Pull = GPIO_PULLUP;			 /* 上拉 */
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; /* 低速 */
	GPIO_InitStruct.Pin = Touch_INT_PIN;		 /* INT引脚 */

	HAL_GPIO_Init(Touch_INT_PORT, &GPIO_InitStruct);
}

/**
 * @brief  配置INT引脚为输入模式
 * @param  无
 * @retval 无
 * @note   用于触摸屏通过IIC向MCU发送数据时
 */
void Touch_INT_In(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;		 /* 输入模式 */
	GPIO_InitStruct.Pull = GPIO_NOPULL;			 /* 浮空 */
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; /* 低速 */
	GPIO_InitStruct.Pin = Touch_INT_PIN;		 /* INT引脚 */

	HAL_GPIO_Init(Touch_INT_PORT, &GPIO_InitStruct);
}

/*******************************************************************************
 *                              IIC通信时序函数
 *******************************************************************************/

/**
 * @brief  IIC起始信号
 * @param  无
 * @retval 无
 * @note   在SCL处于高电平期间，SDA由高到低跳变产生起始信号
 */
void Touch_IIC_Start(void)
{
	Touch_IIC_SDA(1);
	Touch_IIC_SCL(1);
	Touch_IIC_Delay(IIC_DelayVaule);

	Touch_IIC_SDA(0);
	Touch_IIC_Delay(IIC_DelayVaule);
	Touch_IIC_SCL(0);
	Touch_IIC_Delay(IIC_DelayVaule);
}

/**
 * @brief  IIC停止信号
 * @param  无
 * @retval 无
 * @note   在SCL处于高电平期间，SDA由低到高跳变产生停止信号
 */
void Touch_IIC_Stop(void)
{
	Touch_IIC_SCL(0);
	Touch_IIC_Delay(IIC_DelayVaule);
	Touch_IIC_SDA(0);
	Touch_IIC_Delay(IIC_DelayVaule);

	Touch_IIC_SCL(1);
	Touch_IIC_Delay(IIC_DelayVaule);
	Touch_IIC_SDA(1);
	Touch_IIC_Delay(IIC_DelayVaule);
}

/**
 * @brief  IIC应答信号
 * @param  无
 * @retval 无
 * @note   MCU向从设备发送应答信号
 */
void Touch_IIC_ACK(void)
{
	Touch_IIC_SCL(0);
	Touch_IIC_Delay(IIC_DelayVaule);
	Touch_IIC_SDA(0);
	Touch_IIC_Delay(IIC_DelayVaule);
	Touch_IIC_SCL(1);
	Touch_IIC_Delay(IIC_DelayVaule);

	Touch_IIC_SCL(0); /* SCL输出低时，SDA应立即拉高，释放总线 */
	Touch_IIC_SDA(1);

	Touch_IIC_Delay(IIC_DelayVaule);
}

/**
 * @brief  IIC非应答信号
 * @param  无
 * @retval 无
 * @note   MCU向从设备发送非应答信号，结束数据接收
 */
void Touch_IIC_NoACK(void)
{
	Touch_IIC_SCL(0);
	Touch_IIC_Delay(IIC_DelayVaule);
	Touch_IIC_SDA(1);
	Touch_IIC_Delay(IIC_DelayVaule);
	Touch_IIC_SCL(1);
	Touch_IIC_Delay(IIC_DelayVaule);

	Touch_IIC_SCL(0);
	Touch_IIC_Delay(IIC_DelayVaule);
}

/**
 * @brief  等待IIC从设备的应答信号
 * @param  无
 * @retval ACK_OK - 设备响应正常，SDA为低电平
 * @retval ACK_ERR - 设备无响应，SDA仍为高电平
 * @note   在SCL为高电平期间检测SDA电平
 */
uint8_t Touch_IIC_WaitACK(void)
{
	Touch_IIC_SDA(1);
	Touch_IIC_Delay(IIC_DelayVaule);
	Touch_IIC_SCL(1);
	Touch_IIC_Delay(IIC_DelayVaule);

	if (HAL_GPIO_ReadPin(Touch_IIC_SDA_PORT, Touch_IIC_SDA_PIN) != 0) /* 判断设备是否有做出响应 */
	{
		Touch_IIC_SCL(0);
		Touch_IIC_Delay(IIC_DelayVaule);
		return ACK_ERR; /* 无应答 */
	}
	else
	{
		Touch_IIC_SCL(0);
		Touch_IIC_Delay(IIC_DelayVaule);
		return ACK_OK; /* 应答正常 */
	}
}

/*******************************************************************************
 *                              IIC字节读写函数
 *******************************************************************************/

/**
 * @brief  通过IIC写一字节数据
 * @param  IIC_Data: 要写入的8位数据
 * @retval ACK_OK - 写入成功，设备响应正常
 * @retval ACK_ERR - 写入失败，设备无响应
 * @note   高位先行，MSB = 数据的第7位
 */
uint8_t Touch_IIC_WriteByte(uint8_t IIC_Data)
{
	uint8_t i;

	for (i = 0; i < 8; i++)
	{
		Touch_IIC_SDA(IIC_Data & 0x80); /* 发送数据的最高位 */

		Touch_IIC_Delay(IIC_DelayVaule);
		Touch_IIC_SCL(1);
		Touch_IIC_Delay(IIC_DelayVaule);
		Touch_IIC_SCL(0);
		if (i == 7)
		{
			Touch_IIC_SDA(1); /* 第8位后释放SDA，为应答信号做准备 */
		}
		IIC_Data <<= 1; /* 左移一位，准备发送下一位 */
	}

	return Touch_IIC_WaitACK(); /* 等待设备响应 */
}

/**
 * @brief  通过IIC读一字节数据
 * @param  ACK_Mode: 应答模式
 *                   - 1: 读取后发送应答信号，继续接收数据
 *                   - 0: 读取后发送非应答信号，结束接收
 * @retval 读取到的8位数据
 * @note   高位先行，MSB = 数据的第7位
 */
uint8_t Touch_IIC_ReadByte(uint8_t ACK_Mode)
{
	uint8_t IIC_Data = 0;
	uint8_t i = 0;

	for (i = 0; i < 8; i++)
	{
		IIC_Data <<= 1; /* 左移一位，为读取新数据做准备 */

		Touch_IIC_SCL(1);
		Touch_IIC_Delay(IIC_DelayVaule);
		IIC_Data |= (HAL_GPIO_ReadPin(Touch_IIC_SDA_PORT, Touch_IIC_SDA_PIN) & 0x01); /* 读取SDA */
		Touch_IIC_SCL(0);
		Touch_IIC_Delay(IIC_DelayVaule);
	}

	if (ACK_Mode == 1) /* 应答信号 */
	{
		Touch_IIC_ACK();
	}
	else /* 非应答信号 */
	{
		Touch_IIC_NoACK();
	}

	return IIC_Data;
}

/****** End of File ******/
