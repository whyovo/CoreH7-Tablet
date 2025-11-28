/**
 ******************************************************************************
 * @file    sdram.c
 * @author  菜菜why（B站：菜菜whyy）
 * @brief   SDRAM驱动实现文件
 ******************************************************************************
 * @attention
 *
 * 本文件实现：
 * - SDRAM初始化序列（时钟、预充电、自动刷新、模式设置）
 * - SDRAM读写性能测试（32位/8位数据宽度）
 * - 数据完整性校验
 *
 ******************************************************************************
 */

#include "sdram.h"
#include <string.h>
#include <stdio.h>
#ifdef SDMMC_ENABLE

/*******************************************************************************
 *                              私有全局变量
 *******************************************************************************/

FMC_SDRAM_CommandTypeDef *Command; /*!< FMC SDRAM控制命令结构体指针 */

/*******************************************************************************
 *                              SDRAM初始化函数
 *******************************************************************************/
/**
 * @brief  SDRAM初始化序列
 * @retval None
 *
 * @par    执行流程:
 *         1. 发送CLK_ENABLE命令，开启SDRAM时钟信号
 *         2. 延时1ms，等待时钟稳定
 *         3. 发送PALL命令，对所有Bank执行预充电（Precharge All）
 *         4. 发送AUTOREFRESH命令，自动刷新8次
 *         5. 配置模式寄存器：突发长度1、顺序突发、CAS延迟3、标准模式、单个写突发
 *         6. 配置刷新率为1543（对应64ms刷新周期）
 */
void SDRAM_Initialization_Sequence(void)
{
	__IO uint32_t tmpmrd = 0;

	/* 步骤1：开启SDRAM时钟 */
	Command->CommandMode = FMC_SDRAM_CMD_CLK_ENABLE;  /*!< 时钟使能命令 */
	Command->CommandTarget = FMC_COMMAND_TARGET_BANK; /*!< 选择目标Bank */
	Command->AutoRefreshNumber = 1;
	Command->ModeRegisterDefinition = 0;

	HAL_SDRAM_SendCommand(&SDRAM_HANDLE, Command, SDRAM_TIMEOUT);
	HAL_Delay(1); /*!< 等待时钟稳定 */

	/* 步骤2：预充电所有Bank */
	Command->CommandMode = FMC_SDRAM_CMD_PALL;		  /*!< 预充电命令 */
	Command->CommandTarget = FMC_COMMAND_TARGET_BANK; /*!< 选择目标Bank */
	Command->AutoRefreshNumber = 1;
	Command->ModeRegisterDefinition = 0;

	HAL_SDRAM_SendCommand(&SDRAM_HANDLE, Command, SDRAM_TIMEOUT);

	/* 步骤3：自动刷新 */
	Command->CommandMode = FMC_SDRAM_CMD_AUTOREFRESH_MODE; /*!< 自动刷新命令 */
	Command->CommandTarget = FMC_COMMAND_TARGET_BANK;	   /*!< 选择目标Bank */
	Command->AutoRefreshNumber = 8;						   /*!< 刷新8次 */
	Command->ModeRegisterDefinition = 0;

	HAL_SDRAM_SendCommand(&SDRAM_HANDLE, Command, SDRAM_TIMEOUT);

	/* 步骤4：配置模式寄存器 */
	tmpmrd = (uint32_t)SDRAM_MODEREG_BURST_LENGTH_1 | /*!< 突发长度=1 */
			 SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL |	  /*!< 顺序突发 */
			 SDRAM_MODEREG_CAS_LATENCY_3 |			  /*!< CAS延迟=3 */
			 SDRAM_MODEREG_OPERATING_MODE_STANDARD |  /*!< 标准模式 */
			 SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;	  /*!< 单个写突发 */

	Command->CommandMode = FMC_SDRAM_CMD_LOAD_MODE;	  /*!< 加载模式寄存器命令 */
	Command->CommandTarget = FMC_COMMAND_TARGET_BANK; /*!< 选择目标Bank */
	Command->AutoRefreshNumber = 1;
	Command->ModeRegisterDefinition = tmpmrd;

	HAL_SDRAM_SendCommand(&SDRAM_HANDLE, Command, SDRAM_TIMEOUT);

	/* 步骤5：配置刷新率 */
	HAL_SDRAM_ProgramRefreshRate(&SDRAM_HANDLE, 1543); /*!< 刷新周期：64ms（1543 x 13.8ns ≈ 64ms） */
}

/*******************************************************************************
 *                              SDRAM测试函数
 *******************************************************************************/

/**
 * @brief  SDRAM读写功能测试和性能测试
 * @retval SUCCESS - 测试通过，ERROR - 测试失败
 *
 * @par    测试流程:
 *
 *         【第一阶段：32位数据宽度性能测试】
 *         1. 32位写入测试：将0到(SDRAM_Size/4-1)写入SDRAM
 *            - 计算写入时间和吞吐率（MB/s）
 *         2. 32位读取测试：逐个读取已写入的数据
 *            - 计算读取时间和吞吐率（MB/s）
 *
 *         【第二阶段：32位数据校验】
 *         1. 逐个读取SDRAM数据，与写入值比较
 *         2. 若不匹配则返回失败，打印错误位置和读出数据
 *
 *         【第三阶段：8位数据宽度校验】
 *         1. 使用8位宽度写入0到(SDRAM_Size-1)
 *         2. 逐个读取8位数据，与写入值比较
 *         3. 此阶段用于验证NBL0和NBL1字节选择信号是否正常
 *
 * @note   整个测试过程产生大量串口输出，包括：
 *         - 32位写入速度（MB/s）
 *         - 32位读取速度（MB/s）
 *         - 校验结果或错误位置
 */
uint8_t SDRAM_Test(void)
{
	uint32_t i = 0;		   /*!< 循环计数变量 */
	uint32_t *pSDRAM;	   /*!< SDRAM指针（32位） */
	uint32_t ReadData = 0; /*!< 32位读取数据缓存 */
	uint8_t ReadData_8b;   /*!< 8位读取数据缓存 */

	uint32_t ExecutionTime_Begin; /*!< 操作开始时间戳（ms） */
	uint32_t ExecutionTime_End;	  /*!< 操作结束时间戳（ms） */
	uint32_t ExecutionTime;		  /*!< 操作耗时（ms） */
	float ExecutionSpeed;		  /*!< 吞吐速度（MB/s） */
	char buf[256];				  /*!< 输出缓冲区 */

	DEBUG_INFO("SDRAM 读写性能测试开始");
	DEBUG_INFO("进行速度测试...");

	/*===================================================================================
	 * 阶段1：32位数据宽度写入测试
	 *==================================================================================*/

	pSDRAM = (uint32_t *)SDRAM_BANK_ADDR;
	if (pSDRAM == NULL)
	{
		DEBUG_ERROR("SDRAM测试失败：SDRAM基地址无效");
		return ERROR;
	}

	ExecutionTime_Begin = HAL_GetTick(); /*!< 记录开始时间 */

	for (i = 0; i < SDRAM_Size / 4; i++)
	{
		*(__IO uint32_t *)pSDRAM++ = i; /*!< 写入32位数据 */
	}

	ExecutionTime_End = HAL_GetTick();
	ExecutionTime = ExecutionTime_End - ExecutionTime_Begin;

	if (ExecutionTime == 0)
	{
		DEBUG_ERROR("SDRAM测试失败：写入测试耗时为0，请检查系统时钟");
		return ERROR;
	}

	ExecutionSpeed = (float)SDRAM_Size / 1024 / 1024 / ExecutionTime * 1000; /*!< 单位：MB/s */

	snprintf(buf, sizeof(buf), "32位写入测试 - 大小：%u MB，耗时：%u ms，速度：%.2f MB/s",
			 SDRAM_Size / 1024 / 1024, ExecutionTime, ExecutionSpeed);
	DEBUG_INFO(buf);

	/*===================================================================================
	 * 阶段2：32位数据宽度读取测试
	 *==================================================================================*/

	pSDRAM = (uint32_t *)SDRAM_BANK_ADDR;
	ExecutionTime_Begin = HAL_GetTick(); /*!< 记录开始时间 */

	for (i = 0; i < SDRAM_Size / 4; i++)
	{
		ReadData = *(__IO uint32_t *)pSDRAM++; /*!< 读取32位数据 */
	}

	ExecutionTime_End = HAL_GetTick();
	ExecutionTime = ExecutionTime_End - ExecutionTime_Begin;

	if (ExecutionTime == 0)
	{
		DEBUG_ERROR("SDRAM测试失败：读取测试耗时为0，请检查系统时钟");
		return ERROR;
	}

	ExecutionSpeed = (float)SDRAM_Size / 1024 / 1024 / ExecutionTime * 1000; /*!< 单位：MB/s */

	snprintf(buf, sizeof(buf), "32位读取测试 - 大小：%u MB，耗时：%u ms，速度：%.2f MB/s",
			 SDRAM_Size / 1024 / 1024, ExecutionTime, ExecutionSpeed);
	DEBUG_INFO(buf);

	/*===================================================================================
	 * 阶段3：32位数据宽度校验
	 *==================================================================================*/

	DEBUG_INFO("进行32位数据校验...");

	pSDRAM = (uint32_t *)SDRAM_BANK_ADDR;

	for (i = 0; i < SDRAM_Size / 4; i++)
	{
		ReadData = *(__IO uint32_t *)pSDRAM++; /*!< 读取32位数据 */
		if (ReadData != (uint32_t)i)		   /*!< 比较数据 */
		{
			snprintf(buf, sizeof(buf), "32位数据校验失败！地址：0x%x，期望值：%u，读出值：%u",
					 (uint32_t)SDRAM_BANK_ADDR + i * 4, (uint32_t)i, ReadData);
			DEBUG_ERROR(buf);
			return ERROR; /*!< 返回失败 */
		}
	}

	DEBUG_INFO("32位数据宽度校验通过");

	/*===================================================================================
	 * 阶段4：8位数据宽度校验（验证NBL0/NBL1字节选择引脚）
	 *==================================================================================*/

	DEBUG_INFO("进行8位数据宽度写入测试...");

	for (i = 0; i < SDRAM_Size; i++)
	{
		*(__IO uint8_t *)(SDRAM_BANK_ADDR + i) = (uint8_t)i; /*!< 写入8位数据 */
	}

	DEBUG_INFO("8位数据写入完毕，进行读取校验...");

	for (i = 0; i < SDRAM_Size; i++)
	{
		ReadData_8b = *(__IO uint8_t *)(SDRAM_BANK_ADDR + i); /*!< 读取8位数据 */
		if (ReadData_8b != (uint8_t)i)						  /*!< 比较数据 */
		{
			snprintf(buf, sizeof(buf), "8位数据校验失败！地址：0x%x，期望值：%u，读出值：%u，请检查NBL0/NBL1引脚连接",
					 (uint32_t)SDRAM_BANK_ADDR + i, (uint8_t)i, ReadData_8b);
			DEBUG_ERROR(buf);
			return ERROR; /*!< 返回失败 */
		}
	}

	DEBUG_INFO("8位数据宽度校验通过");
	DEBUG_INFO("SDRAM 读写测试全部通过，系统正常");

	return SUCCESS; /*!< 返回成功 */
}

/*******************************************************************************
 *                         SDRAM读写功能函数
 *******************************************************************************/

/**
 * @brief  向SDRAM指定地址写入8位数据
 * @param  Address: 目标地址
 * @param  Data: 要写入的8位数据
 * @retval None
 */
void SDRAM_Write_8bit(uint32_t Address, uint8_t Data)
{
	*(__IO uint8_t *)Address = Data;
}

/**
 * @brief  从SDRAM指定地址读取8位数据
 * @param  Address: 目标地址
 * @retval 读取的8位数据
 */
uint8_t SDRAM_Read_8bit(uint32_t Address)
{
	return *(__IO uint8_t *)Address;
}

/**
 * @brief  向SDRAM指定地址写入16位数据
 * @param  Address: 目标地址
 * @param  Data: 要写入的16位数据
 * @retval None
 */
void SDRAM_Write_16bit(uint32_t Address, uint16_t Data)
{
	*(__IO uint16_t *)Address = Data;
}

/**
 * @brief  从SDRAM指定地址读取16位数据
 * @param  Address: 目标地址
 * @retval 读取的16位数据
 */
uint16_t SDRAM_Read_16bit(uint32_t Address)
{
	return *(__IO uint16_t *)Address;
}

/**
 * @brief  向SDRAM指定地址写入32位数据
 * @param  Address: 目标地址
 * @param  Data: 要写入的32位数据
 * @retval None
 */
void SDRAM_Write_32bit(uint32_t Address, uint32_t Data)
{
	*(__IO uint32_t *)Address = Data;
}

/**
 * @brief  从SDRAM指定地址读取32位数据
 * @param  Address: 目标地址
 * @retval 读取的32位数据
 */
uint32_t SDRAM_Read_32bit(uint32_t Address)
{
	return *(__IO uint32_t *)Address;
}

/**
 * @brief  向SDRAM写入数据块（8位）
 * @param  Address: 起始地址
 * @param  pBuffer: 数据缓冲区指针
 * @param  Length: 要写入的字节数
 * @retval SUCCESS - 写入成功，ERROR - 地址越界
 */
uint8_t SDRAM_WriteBuffer_8bit(uint32_t Address, uint8_t *pBuffer, uint32_t Length)
{
	uint32_t i;

	/* 参数检查 */
	if (pBuffer == NULL || Length == 0)
	{
		return ERROR;
	}

	/* 地址合法性检查 */
	if ((Address < SDRAM_BANK_ADDR) || ((Address + Length - 1) > (SDRAM_BANK_ADDR + SDRAM_Size - 1)))
	{
		DEBUG_ERROR("SDRAM写入失败：地址越界");
		return ERROR;
	}

	/* 逐字节写入 */
	for (i = 0; i < Length; i++)
	{
		*(__IO uint8_t *)(Address + i) = pBuffer[i];
	}

	return SUCCESS;
}

/**
 * @brief  从SDRAM读取数据块（8位）
 * @param  Address: 起始地址
 * @param  pBuffer: 数据缓冲区指针
 * @param  Length: 要读取的字节数
 * @retval SUCCESS - 读取成功，ERROR - 地址越界
 */
uint8_t SDRAM_ReadBuffer_8bit(uint32_t Address, uint8_t *pBuffer, uint32_t Length)
{
	uint32_t i;

	/* 参数检查 */
	if (pBuffer == NULL || Length == 0)
	{
		return ERROR;
	}

	/* 地址合法性检查 */
	if ((Address < SDRAM_BANK_ADDR) || ((Address + Length - 1) > (SDRAM_BANK_ADDR + SDRAM_Size - 1)))
	{
		DEBUG_ERROR("SDRAM读取失败：地址越界");
		return ERROR;
	}

	/* 逐字节读取 */
	for (i = 0; i < Length; i++)
	{
		pBuffer[i] = *(__IO uint8_t *)(Address + i);
	}

	return SUCCESS;
}

/**
 * @brief  向SDRAM写入数据块（32位）
 * @param  Address: 起始地址（必须4字节对齐）
 * @param  pBuffer: 数据缓冲区指针（32位指针）
 * @param  Length: 要写入的32位数据个数
 * @retval SUCCESS - 写入成功，ERROR - 地址越界或未对齐
 */
uint8_t SDRAM_WriteBuffer_32bit(uint32_t Address, uint32_t *pBuffer, uint32_t Length)
{
	uint32_t i;

	/* 参数检查 */
	if (pBuffer == NULL || Length == 0)
	{
		return ERROR;
	}

	/* 地址对齐检查 */
	if (Address & 0x03)
	{
		DEBUG_ERROR("SDRAM 32位写入失败：地址未4字节对齐");
		return ERROR;
	}

	/* 地址合法性检查 */
	if ((Address < SDRAM_BANK_ADDR) || ((Address + Length * 4 - 1) > (SDRAM_BANK_ADDR + SDRAM_Size - 1)))
	{
		DEBUG_ERROR("SDRAM 32位写入失败：地址越界");
		return ERROR;
	}

	/* 逐32位写入 */
	for (i = 0; i < Length; i++)
	{
		*(__IO uint32_t *)(Address + i * 4) = pBuffer[i];
	}

	return SUCCESS;
}

/**
 * @brief  从SDRAM读取数据块（32位）
 * @param  Address: 起始地址（必须4字节对齐）
 * @param  pBuffer: 数据缓冲区指针（32位指针）
 * @param  Length: 要读取的32位数据个数
 * @retval SUCCESS - 读取成功，ERROR - 地址越界或未对齐
 */
uint8_t SDRAM_ReadBuffer_32bit(uint32_t Address, uint32_t *pBuffer, uint32_t Length)
{
	uint32_t i;

	/* 参数检查 */
	if (pBuffer == NULL || Length == 0)
	{
		return ERROR;
	}

	/* 地址对齐检查 */
	if (Address & 0x03)
	{
		DEBUG_ERROR("SDRAM 32位读取失败：地址未4字节对齐");
		return ERROR;
	}

	/* 地址合法性检查 */
	if ((Address < SDRAM_BANK_ADDR) || ((Address + Length * 4 - 1) > (SDRAM_BANK_ADDR + SDRAM_Size - 1)))
	{
		DEBUG_ERROR("SDRAM 32位读取失败：地址越界");
		return ERROR;
	}

	/* 逐32位读取 */
	for (i = 0; i < Length; i++)
	{
		pBuffer[i] = *(__IO uint32_t *)(Address + i * 4);
	}

	return SUCCESS;
}

/**
 * @brief  使用指定值填充SDRAM内存区域
 * @param  Address: 起始地址
 * @param  Value: 填充值（8位）
 * @param  Length: 要填充的字节数
 * @retval SUCCESS - 填充成功，ERROR - 地址越界
 */
uint8_t SDRAM_Fill_8bit(uint32_t Address, uint8_t Value, uint32_t Length)
{
	uint32_t i;

	/* 地址合法性检查 */
	if ((Address < SDRAM_BANK_ADDR) || ((Address + Length - 1) > (SDRAM_BANK_ADDR + SDRAM_Size - 1)) || (Length == 0))
	{
		DEBUG_ERROR("SDRAM填充失败：地址越界或长度为0");
		return ERROR;
	}

	/* 填充内存 */
	for (i = 0; i < Length; i++)
	{
		*(__IO uint8_t *)(Address + i) = Value;
	}

	return SUCCESS;
}

/**
 * @brief  清空SDRAM内存区域（填充为0）
 * @param  Address: 起始地址
 * @param  Length: 要清空的字节数
 * @retval SUCCESS - 清空成功，ERROR - 地址越界
 */
uint8_t SDRAM_Clear(uint32_t Address, uint32_t Length)
{
	return SDRAM_Fill_8bit(Address, 0, Length);
}

#endif
