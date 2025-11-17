/**
 ******************************************************************************
 * @file    sdram.h
 * @author  菜菜why（B站：菜菜whyy）
 * @brief   SDRAM驱动头文件
 ******************************************************************************
 * @attention
 *
 * 使用方法:
 * 1. 在初始化阶段调用 SDRAM_Initialization_Sequence() 配置SDRAM参数
 * 2. 可选：调用 SDRAM_Test() 进行SDRAM读写测试和性能测试
 *
 * 配置说明:
 * - SDRAM_Size: 定义SDRAM容量（默认16MB）
 * - SDRAM_BANK_ADDR: SDRAM映射地址（通过FMC配置）
 * - 刷新率自动配置为1543，适用于64ms刷新周期
 *
 * 测试功能:
 * - 32位数据宽度读写测试和性能测试
 * - 8位数据宽度读写测试（验证NBL0/NBL1连接）
 * - 自动计算读写速度（单位：MB/s）
 *
 ******************************************************************************
 */

#ifndef __SDRAM_H
#define __SDRAM_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "init.h"
    /*******************************************************************************
     *                              SDRAM句柄宏定义
     *******************************************************************************/

    /**
     * @brief SDRAM句柄宏定义，来自 Core/Src/fmc.c 的全局变量 hsdram1
     */
    #define SDRAM_HANDLE hsdram1
    extern SDRAM_HandleTypeDef hsdram1;

    /*******************************************************************************
     *                              SDRAM配置参数
     *******************************************************************************/

#define SDRAM_Size 16 * 1024 * 1024 /*!< SDRAM容量定义（16MB） */

#define SDRAM_BANK_ADDR ((uint32_t)0xC0000000)             /*!< FMC SDRAM数据基地址 */
#define FMC_COMMAND_TARGET_BANK FMC_SDRAM_CMD_TARGET_BANK1 /*!< SDRAM的Bank选择 */
#define SDRAM_TIMEOUT ((uint32_t)0x1000)                   /*!< 操作超时时间 */

    /*******************************************************************************
     *                              SDRAM模式寄存器配置
     *******************************************************************************/

#define SDRAM_MODEREG_BURST_LENGTH_1 ((uint16_t)0x0000)             /*!< 突发长度=1 */
#define SDRAM_MODEREG_BURST_LENGTH_2 ((uint16_t)0x0001)             /*!< 突发长度=2 */
#define SDRAM_MODEREG_BURST_LENGTH_4 ((uint16_t)0x0002)             /*!< 突发长度=4 */
#define SDRAM_MODEREG_BURST_LENGTH_8 ((uint16_t)0x0004)             /*!< 突发长度=8 */
#define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL ((uint16_t)0x0000)      /*!< 顺序突发 */
#define SDRAM_MODEREG_BURST_TYPE_INTERLEAVED ((uint16_t)0x0008)     /*!< 交错突发 */
#define SDRAM_MODEREG_CAS_LATENCY_2 ((uint16_t)0x0020)              /*!< CAS延迟=2 */
#define SDRAM_MODEREG_CAS_LATENCY_3 ((uint16_t)0x0030)              /*!< CAS延迟=3 */
#define SDRAM_MODEREG_OPERATING_MODE_STANDARD ((uint16_t)0x0000)    /*!< 标准操作模式 */
#define SDRAM_MODEREG_WRITEBURST_MODE_PROGRAMMED ((uint16_t)0x0000) /*!< 写突发模式：编程 */
#define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE ((uint16_t)0x0200)     /*!< 写突发模式：单个 */

    /*******************************************************************************
     *                              SDRAM导出函数
     *******************************************************************************/

    /**
     * @brief  SDRAM初始化序列
     * @note   配置SDRAM时序参数、发送各种控制命令、设置刷新率
     * @note   必须在FMC硬件初始化之后调用
     * @note   内部使用宏 SDRAM_HANDLE 访问句柄，无需传参
     * @retval None
     *
     * @par    初始化步骤:
     *         1. 开启SDRAM时钟
     *         2. 预充电所有Bank
     *         3. 自动刷新8次
     *         4. 加载模式寄存器
     *         5. 配置刷新率
     */
    void SDRAM_Initialization_Sequence(void);

    /**
     * @brief  SDRAM读写测试
     * @note   执行包括32位和8位数据宽度的读写测试
     * @note   测试过程中会打印性能数据和错误信息到串口
     * @note   用时约数秒，建议在启动时执行
     * @retval SUCCESS(0) - 测试通过，ERROR(1) - 测试失败
     *
     * @par    测试项目:
     *         1. 32位数据宽度写入性能测试
     *         2. 32位数据宽度读取性能测试
     *         3. 32位数据宽度读写校验
     *         4. 8位数据宽度读写校验（测试NBL0/NBL1引脚）
     */
    uint8_t SDRAM_Test(void);

    /*******************************************************************************
     *                         SDRAM读写功能函数
     *******************************************************************************/

    /**
     * @brief  向SDRAM指定地址写入8位数据
     * @param  Address: 目标地址
     * @param  Data: 要写入的8位数据
     * @retval None
     */
    void SDRAM_Write_8bit(uint32_t Address, uint8_t Data);

    /**
     * @brief  从SDRAM指定地址读取8位数据
     * @param  Address: 目标地址
     * @retval 读取的8位数据
     */
    uint8_t SDRAM_Read_8bit(uint32_t Address);

    /**
     * @brief  向SDRAM指定地址写入16位数据
     * @param  Address: 目标地址
     * @param  Data: 要写入的16位数据
     * @retval None
     */
    void SDRAM_Write_16bit(uint32_t Address, uint16_t Data);

    /**
     * @brief  从SDRAM指定地址读取16位数据
     * @param  Address: 目标地址
     * @retval 读取的16位数据
     */
    uint16_t SDRAM_Read_16bit(uint32_t Address);

    /**
     * @brief  向SDRAM指定地址写入32位数据
     * @param  Address: 目标地址
     * @param  Data: 要写入的32位数据
     * @retval None
     */
    void SDRAM_Write_32bit(uint32_t Address, uint32_t Data);

    /**
     * @brief  从SDRAM指定地址读取32位数据
     * @param  Address: 目标地址
     * @retval 读取的32位数据
     */
    uint32_t SDRAM_Read_32bit(uint32_t Address);

    /**
     * @brief  向SDRAM写入数据块（8位）
     * @param  Address: 起始地址
     * @param  pBuffer: 数据缓冲区指针
     * @param  Length: 要写入的字节数
     * @retval SUCCESS - 写入成功，ERROR - 地址越界
     */
    uint8_t SDRAM_WriteBuffer_8bit(uint32_t Address, uint8_t *pBuffer, uint32_t Length);

    /**
     * @brief  从SDRAM读取数据块（8位）
     * @param  Address: 起始地址
     * @param  pBuffer: 数据缓冲区指针
     * @param  Length: 要读取的字节数
     * @retval SUCCESS - 读取成功，ERROR - 地址越界
     */
    uint8_t SDRAM_ReadBuffer_8bit(uint32_t Address, uint8_t *pBuffer, uint32_t Length);

    /**
     * @brief  向SDRAM写入数据块（32位）
     * @param  Address: 起始地址（必须4字节对齐）
     * @param  pBuffer: 数据缓冲区指针（32位指针）
     * @param  Length: 要写入的32位数据个数
     * @retval SUCCESS - 写入成功，ERROR - 地址越界或未对齐
     */
    uint8_t SDRAM_WriteBuffer_32bit(uint32_t Address, uint32_t *pBuffer, uint32_t Length);

    /**
     * @brief  从SDRAM读取数据块（32位）
     * @param  Address: 起始地址（必须4字节对齐）
     * @param  pBuffer: 数据缓冲区指针（32位指针）
     * @param  Length: 要读取的32位数据个数
     * @retval SUCCESS - 读取成功，ERROR - 地址越界或未对齐
     */
    uint8_t SDRAM_ReadBuffer_32bit(uint32_t Address, uint32_t *pBuffer, uint32_t Length);

    /**
     * @brief  使用指定值填充SDRAM内存区域
     * @param  Address: 起始地址
     * @param  Value: 填充值（8位）
     * @param  Length: 要填充的字节数
     * @retval SUCCESS - 填充成功，ERROR - 地址越界
     */
    uint8_t SDRAM_Fill_8bit(uint32_t Address, uint8_t Value, uint32_t Length);

    /**
     * @brief  清空SDRAM内存区域（填充为0）
     * @param  Address: 起始地址
     * @param  Length: 要清空的字节数
     * @retval SUCCESS - 清空成功，ERROR - 地址越界
     */
    uint8_t SDRAM_Clear(uint32_t Address, uint32_t Length);

#ifdef __cplusplus
}
#endif

#endif // !__SDRAM_H
