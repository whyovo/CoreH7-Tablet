/**
 ******************************************************************************
 * @file    qspi_flash.h
 * @author  菜菜why（B站：菜菜whyy）
 * @brief   QSPI Flash W25Qxx驱动头文件
 ******************************************************************************
 * @attention
 *
 * 本文件提供：
 * - W25Qxx系列Flash的QSPI驱动接口
 * - 擦除操作（扇区/块/整片）
 * - 读写操作（页写入/缓冲区读写）
 * - 内存映射模式支持
 * - Flash性能测试函数
 *
 * 硬件配置：
 * - 使用 QUADSPI_BK1
 * - 默认QSPI驱动时钟：120MHz
 * - 支持芯片：W25Q256JV（32MB）
 *
 * 性能参数（参考W25Q256JV数据手册）：
 * - 扇区擦除(4K)：典型45ms，最大400ms
 * - 块擦除(64K)：典型150ms，最大2000ms
 * - 整片擦除：典型80s，最大400s
 * - 页写入(256B)：典型0.4ms，最大3ms
 *
 * 使用说明：
 * 1. 调用 QSPI_W25Qxx_Init() 初始化
 * 2. 写入前必须先擦除（擦除单位：扇区4K/块64K/整片）
 * 3. 内存映射模式下只能读取，不能写入
 * 4. 建议使用64K块擦除，速度最快
 *
 ******************************************************************************
 */

#ifndef QSPI_w25q64_H
#define QSPI_w25q64_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "init.h"

/*******************************************************************************
 *                              QSPI状态码
 *******************************************************************************/
#define QSPI_W25Qxx_OK 0             /*!< 操作成功 */
#define W25Qxx_ERROR_INIT -1         /*!< 初始化错误 */
#define W25Qxx_ERROR_WriteEnable -2  /*!< 写使能错误 */
#define W25Qxx_ERROR_AUTOPOLLING -3  /*!< 轮询等待超时 */
#define W25Qxx_ERROR_Erase -4        /*!< 擦除错误 */
#define W25Qxx_ERROR_TRANSMIT -5     /*!< 数据传输错误 */
#define W25Qxx_ERROR_MemoryMapped -6 /*!< 内存映射模式错误 */

/*******************************************************************************
 *                              W25Qxx命令集
 *******************************************************************************/
#define W25Qxx_CMD_EnableReset 0x66 /*!< 使能复位 */
#define W25Qxx_CMD_ResetDevice 0x99 /*!< 复位器件 */
#define W25Qxx_CMD_JedecID 0x9F     /*!< 读取JEDEC ID */
#define W25Qxx_CMD_WriteEnable 0X06 /*!< 写使能 */

#define W25Qxx_CMD_SectorErase 0x21    /*!< 扇区擦除（4KB） */
#define W25Qxx_CMD_BlockErase_64K 0xDC /*!< 块擦除（64KB） */
#define W25Qxx_CMD_ChipErase 0xC7      /*!< 整片擦除 */

#define W25Qxx_CMD_QuadInputPageProgram 0x34 /*!< 1-1-4模式页编程 */
#define W25Qxx_CMD_FastReadQuad_IO 0xEC      /*!< 1-4-4模式快速读取 */

/*******************************************************************************
 *                              状态寄存器定义
 *******************************************************************************/
#define W25Qxx_CMD_ReadStatus_REG1 0X05 /*!< 读状态寄存器1 */
#define W25Qxx_Status_REG1_BUSY 0x01    /*!< BUSY标志位（擦除/写入时置1） */
#define W25Qxx_Status_REG1_WEL 0x02     /*!< 写使能标志位 */

/*******************************************************************************
 *                              Flash参数定义
 *******************************************************************************/
#define W25Qxx_PageSize 256                  /*!< 页大小：256字节 */
#define W25Qxx_FlashSize 0x2000000           /*!< Flash大小：32MB（W25Q256） */
#define W25Qxx_FLASH_ID 0Xef4019             /*!< W25Q256 JEDEC ID */
#define W25Qxx_ChipErase_TIMEOUT_MAX 400000U /*!< 整片擦除超时时间：400s */
#define W25Qxx_Mem_Addr 0x90000000           /*!< 内存映射模式基地址 */

    /*******************************************************************************
     *                              基本功能函数
     *******************************************************************************/

    /**
     * @brief  初始化QSPI Flash
     * @retval QSPI_W25Qxx_OK - 初始化成功
     * @retval W25Qxx_ERROR_INIT - 初始化失败（ID不匹配）
     */
    int8_t QSPI_W25Qxx_Init(void);

    /**
     * @brief  复位Flash器件
     * @retval QSPI_W25Qxx_OK - 复位成功
     * @retval W25Qxx_ERROR_INIT - 复位失败
     * @retval W25Qxx_ERROR_AUTOPOLLING - 轮询超时
     */
    int8_t QSPI_W25Qxx_Reset(void);

    /**
     * @brief  读取Flash器件ID
     * @retval 器件ID（W25Q256为0xef4019）
     * @retval W25Qxx_ERROR_INIT - 读取失败
     */
    uint32_t QSPI_W25Qxx_ReadID(void);

    /*******************************************************************************
     *                              内存映射模式
     *******************************************************************************/

    /**
     * @brief  进入内存映射模式
     * @note   此模式下只能读取，不能写入
     * @note   读取地址：W25Qxx_Mem_Addr + offset
     * @retval QSPI_W25Qxx_OK - 配置成功
     * @retval W25Qxx_ERROR_MemoryMapped - 配置失败
     */
    int8_t QSPI_W25Qxx_MemoryMappedMode(void);

    /*******************************************************************************
     *                              擦除操作
     *******************************************************************************/

    /**
     * @brief  扇区擦除（4KB）
     * @param  SectorAddress: 扇区地址（4KB对齐）
     * @note   典型耗时：45ms，最大：400ms
     * @retval QSPI_W25Qxx_OK - 擦除成功
     * @retval W25Qxx_ERROR_* - 擦除失败
     */
    int8_t QSPI_W25Qxx_SectorErase(uint32_t SectorAddress);

    /**
     * @brief  块擦除（64KB）
     * @param  SectorAddress: 块地址（64KB对齐）
     * @note   典型耗时：150ms，最大：2000ms
     * @note   推荐使用：大数据量擦除速度最快
     * @retval QSPI_W25Qxx_OK - 擦除成功
     * @retval W25Qxx_ERROR_* - 擦除失败
     */
    int8_t QSPI_W25Qxx_BlockErase_64K(uint32_t SectorAddress);

    /**
     * @brief  整片擦除
     * @note   典型耗时：80s，最大：400s
     * @note   危险操作：会清除所有数据
     * @retval QSPI_W25Qxx_OK - 擦除成功
     * @retval W25Qxx_ERROR_* - 擦除失败
     */
    int8_t QSPI_W25Qxx_ChipErase(void);

    /*******************************************************************************
     *                              读写操作
     *******************************************************************************/

    /**
     * @brief  页写入（最大256字节）
     * @param  pBuffer: 数据缓冲区指针
     * @param  WriteAddr: 写入地址
     * @param  NumByteToWrite: 写入字节数（≤256）
     * @note   写入前必须先擦除
     * @note   典型耗时：0.4ms/页
     * @retval QSPI_W25Qxx_OK - 写入成功
     * @retval W25Qxx_ERROR_* - 写入失败
     */
    int8_t QSPI_W25Qxx_WritePage(uint8_t *pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);

    /**
     * @brief  缓冲区写入（任意长度）
     * @param  pData: 数据缓冲区指针
     * @param  WriteAddr: 写入地址
     * @param  Size: 写入字节数（≤Flash容量）
     * @note   写入前必须先擦除
     * @note   自动跨页处理
     * @retval QSPI_W25Qxx_OK - 写入成功
     * @retval W25Qxx_ERROR_* - 写入失败
     */
    int8_t QSPI_W25Qxx_WriteBuffer(uint8_t *pData, uint32_t WriteAddr, uint32_t Size);

    /**
     * @brief  缓冲区读取（任意长度）
     * @param  pBuffer: 数据缓冲区指针
     * @param  ReadAddr: 读取地址
     * @param  NumByteToRead: 读取字节数（≤Flash容量）
     * @note   使用1-4-4模式快速读取
     * @note   读取速度受QSPI时钟、DMA、Cache影响
     * @retval QSPI_W25Qxx_OK - 读取成功
     * @retval W25Qxx_ERROR_* - 读取失败
     */
    int8_t QSPI_W25Qxx_ReadBuffer(uint8_t *pBuffer, uint32_t ReadAddr, uint32_t NumByteToRead);

#ifdef __cplusplus
}
#endif

#endif // QSPI_w25q64_H
