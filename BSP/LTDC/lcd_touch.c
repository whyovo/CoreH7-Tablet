/**
 ******************************************************************************
 * @file    lcd_touch.c
 * @author  菜菜why（B站：菜菜whyy）
 * @brief   触摸屏驱动实现文件
 ******************************************************************************
 * @attention
 *
 * 本文件实现：
 * - GT911电容式触摸屏的初始化和复位
 * - 通过模拟IIC进行寄存器读写
 * - 触摸数据的采集、解析和坐标映射
 * - 硬件版本自动识别和分辨率适配
 *
 * 硬件版本判断：
 * - V1.1之前：RST和INT引脚未连接，触摸分辨率1024×600
 * - V1.1及以后：RST和INT引脚已连接，触摸分辨率可识别
 *
 ******************************************************************************
 */

#include "lcd_touch.h"

/*******************************************************************************
 *                              全局变量定义
 *******************************************************************************/

/**
 * @brief 全局触摸信息结构体
 * @note 由Touch_Scan()函数更新，应用层可从此结构体读取触摸数据
 */
volatile TouchStructure touchInfo;

/**
 * @brief 触摸坐标修改标志位
 * @note 用于判断是否需要进行分辨率转换
 *       1 = 需要转换（旧硬件，1024×600 → 800×480）
 *       0 = 无需转换（新硬件或已匹配分辨率）
 */
volatile static uint8_t Modify_Flag = 0;

/*******************************************************************************
 *                              GT911芯片操作函数
 *******************************************************************************/

/**
 * @brief  复位GT911芯片
 * @param  无
 * @retval 无
 * @note   GT911复位时序：
 *         1. INT脚输出低电平（固定IIC地址为0xBA/0xBB）
 *         2. RST脚：高 → 延时 → 低 → 延时 → 高
 *         3. INT脚转输入模式
 */
void GT9XX_Reset(void)
{
	Touch_INT_Out(); /* 将INT引脚配置为输出 */

	/* 初始化引脚状态 */
	HAL_GPIO_WritePin(Touch_INT_PORT, Touch_INT_PIN, GPIO_PIN_RESET); /* INT输出低电平 */
	HAL_GPIO_WritePin(Touch_RST_PORT, Touch_RST_PIN, GPIO_PIN_SET);	  /* RST输出高电平 */
	Touch_IIC_Delay(10000);

	/* 执行复位 */
	/* INT引脚保持低电平，将器件地址设置为0xBA/0xBB */
	HAL_GPIO_WritePin(Touch_RST_PORT, Touch_RST_PIN, GPIO_PIN_RESET); /* 拉低复位引脚 */
	Touch_IIC_Delay(150000);										  /* 延时 */
	HAL_GPIO_WritePin(Touch_RST_PORT, Touch_RST_PIN, GPIO_PIN_SET);	  /* 拉高复位引脚 */
	Touch_IIC_Delay(350000);										  /* 延时 */
	Touch_INT_In();													  /* INT引脚转为输入 */
	Touch_IIC_Delay(20000);											  /* 延时 */
}


/**
 * @brief  GT911寄存器写操作处理
 * @param  addr: 要操作的寄存器地址（16位）
 * @retval SUCCESS - 地址写入成功
 * @retval ERROR - 地址写入失败
 * @note   仅执行IIC起始和地址写入，用于GT9XX_WriteReg/GT9XX_ReadReg内部调用
 */
uint8_t GT9XX_WriteHandle(uint16_t addr)
{
	uint8_t status;

	Touch_IIC_Start();									/* 启动IIC通信 */
	if (Touch_IIC_WriteByte(GT9XX_IIC_WADDR) == ACK_OK) /* 发送写命令 */
	{
		if (Touch_IIC_WriteByte((uint8_t)(addr >> 8)) == ACK_OK) /* 写入高字节地址 */
		{
			if (Touch_IIC_WriteByte((uint8_t)(addr)) == ACK_OK) /* 写入低字节地址 */
			{
				status = SUCCESS; /* 地址写入成功 */
			}
			else
			{
				status = ERROR;
			}
		}
		else
		{
			status = ERROR;
		}
	}
	else
	{
		status = ERROR;
	}
	return status;
}

/**
 * @brief  写一字节数据到GT911
 * @param  addr: 要写入的寄存器地址（16位）
 * @param  value: 要写入的数据
 * @retval SUCCESS - 写入成功
 * @retval ERROR - 写入失败
 * @note   完整的IIC写操作：START → 地址 → 数据 → STOP
 */
uint8_t GT9XX_WriteData(uint16_t addr, uint8_t value)
{
	uint8_t status = SUCCESS;

	Touch_IIC_Start(); /* 启动IIC通信 */

	if (GT9XX_WriteHandle(addr) == SUCCESS) /* 写入要操作的寄存器地址 */
	{
		if (Touch_IIC_WriteByte(value) != ACK_OK) /* 写入数据 */
		{
			status = ERROR;
		}
	}
	else
	{
		status = ERROR;
	}
	Touch_IIC_Stop(); /* 停止IIC通信 */

	return status;
}

/**
 * @brief  写多字节数据到GT911
 * @param  addr: 要写入的寄存器地址（16位）
 * @param  cnt: 要写入的字节数
 * @param  value: 数据缓冲区指针
 * @retval SUCCESS - 写入成功
 * @retval ERROR - 写入失败
 * @note   适用于连续写入多字节数据的场景
 */
uint8_t GT9XX_WriteReg(uint16_t addr, uint8_t cnt, uint8_t *value)
{
	uint8_t status;
	uint8_t i;

	Touch_IIC_Start(); /* 启动IIC通信 */

	if (GT9XX_WriteHandle(addr) == SUCCESS) /* 写入要操作的寄存器地址 */
	{
		for (i = 0; i < cnt; i++) /* 逐字节写入数据 */
		{
			Touch_IIC_WriteByte(value[i]);
		}
		Touch_IIC_Stop(); /* 停止IIC通信 */
		status = SUCCESS; /* 写入成功 */
	}
	else
	{
		Touch_IIC_Stop(); /* 停止IIC通信 */
		status = ERROR;	  /* 写入失败 */
	}
	return status;
}

/**
 * @brief  从GT911读多字节数据
 * @param  addr: 要读取的寄存器地址（16位）
 * @param  cnt: 要读取的字节数
 * @param  value: 数据缓冲区指针
 * @retval SUCCESS - 读取成功
 * @retval ERROR - 读取失败
 * @note   IIC读操作流程：
 *         1. START → 写地址 → STOP
 *         2. START → 读命令 → 逐字节读取 → STOP
 */
uint8_t GT9XX_ReadReg(uint16_t addr, uint8_t cnt, uint8_t *value)
{
	uint8_t status;
	uint8_t i;

	status = ERROR;
	Touch_IIC_Start(); /* 启动IIC通信 */

	if (GT9XX_WriteHandle(addr) == SUCCESS) /* 写入要读取的寄存器地址 */
	{
		Touch_IIC_Start(); /* 重新启动IIC通讯 */

		if (Touch_IIC_WriteByte(GT9XX_IIC_RADDR) == ACK_OK) /* 发送读命令 */
		{
			for (i = 0; i < cnt; i++) /* 逐字节读取数据 */
			{
				if (i == (cnt - 1))
				{
					value[i] = Touch_IIC_ReadByte(0); /* 最后一字节发送NACK */
				}
				else
				{
					value[i] = Touch_IIC_ReadByte(1); /* 其他字节发送ACK */
				}
			}
			Touch_IIC_Stop(); /* 停止IIC通信 */
			status = SUCCESS; /* 读取成功 */
		}
	}
	Touch_IIC_Stop(); /* 停止IIC通信 */
	return status;
}

/*******************************************************************************
 *                              触摸屏初始化和扫描函数
 *******************************************************************************/

/**
 * @brief  识别屏幕硬件版本
 * @param  无
 * @retval 无
 * @note   用于兼容不同硬件版本：
 *         - V1.1之前：RST和INT引脚未连接，分辨率1024×600
 *         - V1.1及以后：RST和INT引脚已连接，分辨率可识别
 *         仅对7寸屏有效，其他尺寸屏幕可忽略此函数
 */
void PanelRecognition(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	Touch_INT_CLK_ENABLE; /* 初始化IO口时钟 */
	Touch_RST_CLK_ENABLE;

	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;		 /* 输入模式 */
	GPIO_InitStruct.Pull = GPIO_PULLDOWN;		 /* 下拉输入 */
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; /* 低速 */
	GPIO_InitStruct.Pin = Touch_INT_PIN;		 /* 初始化INT引脚 */

	HAL_GPIO_Init(Touch_INT_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = Touch_RST_PIN; /* 初始化RST引脚 */
	HAL_GPIO_Init(Touch_RST_PORT, &GPIO_InitStruct);

	Touch_IIC_Delay(4000); /* 延时 */

	/* 旧版本硬件的RST和INT引脚无上拉，新版有上拉处理 */
	if ((HAL_GPIO_ReadPin(Touch_RST_PORT, Touch_RST_PIN) != 1) &&
		(HAL_GPIO_ReadPin(Touch_INT_PORT, Touch_INT_PIN) != 1))
	{
		/* V1.1之前硬件：分辨率1024×600，需转换到800×480 */
		Modify_Flag = 1;
	}
}

/**
 * @brief  初始化触摸屏
 * @param  无
 * @retval SUCCESS - 初始化成功
 * @retval ERROR - 初始化失败，未检测到GT911芯片
 * @note   初始化流程：
 *         1. 识别硬件版本
 *         2. 初始化IIC接口
 *         3. 复位GT911芯片
 *         4. 读取芯片ID和配置版本
 *         5. 打印初始化信息
 *         6. 识别触摸分辨率
 */
uint8_t Touch_Init(void)
{
	uint8_t GT9XX_Info[11]; /* 触摸屏IC信息（11字节） */
	uint8_t cfgVersion = 0; /* 触摸配置版本 */

	/* 识别硬件版本（仅对V1.1之前的硬件有效） */
	PanelRecognition();

        Touch_IIC_GPIO_Config(); /* 初始化IIC引脚 */
	  GT9XX_Reset();			 /* 复位GT911芯片 */

	/* 读取触摸屏IC信息和配置版本 */
	GT9XX_ReadReg(GT9XX_ID_ADDR, 11, GT9XX_Info);  /* 读IC信息 */
	GT9XX_ReadReg(GT9XX_CFG_ADDR, 1, &cfgVersion); /* 读配置版本 */

	/* 验证芯片ID（第一个字符应为'9'） */
	if (GT9XX_Info[0] == '9')
	{
		/* 打印初始化信息 */

		/* 识别触摸分辨率（用于分辨率适配） */
		if (((GT9XX_Info[7] << 8) + GT9XX_Info[6]) == TOUCH_CHIP_WIDTH)
		{
			/* V1.1之前硬件版本：分辨率1024×600 */
			Modify_Flag = 1;
		}
		else if (((GT9XX_Info[7] << 8) + GT9XX_Info[6]) == SCREEN_WIDTH)
		{
			/* 新硬件版本：分辨率800×480 */
			Modify_Flag = 0;
		}

		return SUCCESS;
	}
	else
	{
		DEBUG_ERROR("Touch Error: 未检测到GT911芯片"); /* 错误：未检测到GT911芯片 */
		return ERROR;
	}
}

/**
 * @brief  触摸屏扫描
 * @param  无
 * @retval 无
 * @note   触摸数据格式（从0x814E开始）：
 *         - [0]：触摸状态和点数 & 0x0f = 触摸点数
 *         - [1]：保留
 *         - [2-9]：第1个触摸点（X低→X高→Y低→Y高→其他数据）
 *         - [10-17]：第2个触摸点
 *         - ...以此类推
 */
void Touch_Scan(void)
{
	uint8_t touchData[2 + 8 * TOUCH_MAX]; /* 存储触摸数据 */
	uint8_t i = 0;

	/* 读取触摸数据寄存器 */
	GT9XX_ReadReg(GT9XX_READ_ADDR, 2 + 8 * TOUCH_MAX, touchData);

	/* 清除触摸芯片的数据有效标志位 */
	GT9XX_WriteData(GT9XX_READ_ADDR, 0);

	/* 取当前的触摸点数 */
	touchInfo.num = touchData[0] & 0x0f;

	/* 判断触摸点数是否有效（1-5个触摸点） */
	if ((touchInfo.num >= 1) && (touchInfo.num <= 5))
	{
		/* 提取每个触摸点的坐标 */
		for (i = 0; i < touchInfo.num; i++)
		{
			/* 从寄存器中提取X、Y坐标（小端序） */
			touchInfo.y[i] = (touchData[5 + 8 * i] << 8) | touchData[4 + 8 * i];
			touchInfo.x[i] = (touchData[3 + 8 * i] << 8) | touchData[2 + 8 * i];

			/* 分辨率转换（仅对旧硬件版本） */
			if (Modify_Flag == 1)
			{
				/* 将1024×600分辨率的坐标转换到800×480 */
				touchInfo.y[i] *= TOUCH_Y_SCALE;
				touchInfo.x[i] *= TOUCH_X_SCALE;
			}
		}
		touchInfo.flag = 1; /* 有触摸事件 */
	}
	else
	{
		touchInfo.flag = 0; /* 无触摸事件 */
	}
}

/**
 * @brief  发送GT911配置参数（可选）
 * @param  无
 * @retval 无
 * @note   保留函数，用于修改GT911工作参数
 */
void GT9XX_SendCfg(void)
{
	/* 用户可在此添加配置参数发送代码 */
}

/**
 * @brief  读取GT911配置参数（可选）
 * @param  无
 * @retval 无
 * @note   保留函数，用于读取GT911当前配置
 */
void GT9XX_ReadCfg(void)
{
	/* 用户可在此添加配置参数读取代码 */
}

/****** End of File ******/
