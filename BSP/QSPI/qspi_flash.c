/**
 ******************************************************************************
 * @file    qspi_flash.c
 * @author  菜菜why（B站：菜菜whyy）
 * @brief   QSPI Flash W25Qxx驱动实现文件
 ******************************************************************************
 * @attention
 *
 * 本文件实现：
 * - W25Qxx Flash的QSPI接口驱动
 * - 支持扇区/块/整片擦除
 * - 支持页写入和缓冲区读写
 * - 支持内存映射模式
 * - 提供性能测试函数
 *
 * 重要说明：
 * 1. 擦除耗时（W25Q256JV数据手册典型值）
 *    - 4KB扇区：45ms
 *    - 64KB块：150ms（推荐）
 *    - 整片：80s
 *
 * 2. 写入耗时（数据手册典型值）
 *    - 256字节/页：0.4ms
 *    - 理论速度：1MB/s（实测约600KB/s）
 *
 * 3. 读取速度影响因素
 *    - QSPI驱动时钟
 *    - 是否使用DMA
 *    - 是否开启Cache
 *    - 编译器优化等级
 *    - 数据存储位置（TCM SRAM / AXI SRAM）
 *
 * 4. 内存映射模式
 *    - 只能读取，不能写入
 *    - 性能仅与QSPI时钟和Cache相关
 *    - 访问地址：0x90000000 + offset
 *
 * 5. 使用建议
 *    - 大数据量擦除优先使用64K块擦除
 *    - 写入前务必完成擦除操作
 *    - Flash使用时间越长，擦除/写入耗时越长
 *
 ******************************************************************************
 */

#include "qspi_flash.h"
#include <string.h>
#ifdef QSPI_FLASH_ENABLE

extern QSPI_HandleTypeDef
    hqspi; // 定义QSPI句柄，这里保留使用cubeMX生成的变量命名，方便用户参考和移植

#define W25Qxx_NumByteToTest 32 * 1024 // 测试数据的长度，64K

int32_t QSPI_Status; // 检测标志位

uint32_t W25Qxx_TestAddr = 0x1A20000;             // 测试地址
uint8_t W25Qxx_WriteBuffer[W25Qxx_NumByteToTest]; //	写数据数组
uint8_t W25Qxx_ReadBuffer[W25Qxx_NumByteToTest];  //	读数据数组

/**
 * @brief  初始化QSPI Flash
 * @retval QSPI_W25Qxx_OK - 初始化成功
 * @retval W25Qxx_ERROR_INIT - 初始化失败
 * @note   读取器件ID并与预期值比对
 */
int8_t QSPI_W25Qxx_Init(void) {
  uint32_t Device_ID; // 器件ID

  QSPI_W25Qxx_Reset();              // 复位器件
  Device_ID = QSPI_W25Qxx_ReadID(); // 读取器件ID

  if (Device_ID == W25Qxx_FLASH_ID) {
    return QSPI_W25Qxx_OK;
  } else {
    DEBUG_ERROR("QSPI Flash ID匹配失败");
    return W25Qxx_ERROR_INIT;
  }
}

/**
 * @brief  使用自动轮询等待Flash就绪
 * @retval QSPI_W25Qxx_OK - 通信正常结束
 * @retval W25Qxx_ERROR_AUTOPOLLING - 轮询等待无响应
 * @note   每次通信后都应调用此函数等待完成
 */
int8_t QSPI_W25Qxx_AutoPollingMemReady(void) {
  QSPI_CommandTypeDef s_command;    // QSPI传输配置
  QSPI_AutoPollingTypeDef s_config; // 轮询比较相关配置参数

  s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;     // 1线指令模式
  s_command.AddressMode = QSPI_ADDRESS_NONE;               // 无地址模式
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE; //	无交替字节
  s_command.DdrMode = QSPI_DDR_MODE_DISABLE;               // 禁止DDR模式
  s_command.DdrHoldHalfCycle =
      QSPI_DDR_HHC_ANALOG_DELAY; // DDR模式中数据延迟，这里用不到
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD; //	每次传输数据都发送指令
  s_command.DataMode = QSPI_DATA_1_LINE;         // 1线数据模式
  s_command.DummyCycles = 0;                     //	空周期个数
  s_command.Instruction = W25Qxx_CMD_ReadStatus_REG1; // 读状态信息寄存器

  // 不停的查询 W25Qxx_CMD_ReadStatus_REG1 寄存器，将读取到的状态字节中的
  // W25Qxx_Status_REG1_BUSY 不停的与0作比较
  // 读状态寄存器1的第0位（只读），Busy标志位，当正在擦除/写入数据/写命令时会被置1，空闲或通信结束为0

  s_config.Match = 0;                                  //	匹配值
  s_config.MatchMode = QSPI_MATCH_MODE_AND;            //	与运算
  s_config.Interval = 0x10;                            //	轮询间隔
  s_config.AutomaticStop = QSPI_AUTOMATIC_STOP_ENABLE; // 自动停止模式
  s_config.StatusBytesSize = 1;                        //	状态字节数
  s_config.Mask =
      W25Qxx_Status_REG1_BUSY; // 对在轮询模式下接收的状态字节进行屏蔽，只比较需要用到的位

  // 发送轮询等待命令
  if (HAL_QSPI_AutoPolling(&hqspi, &s_command, &s_config,
                           HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    return W25Qxx_ERROR_AUTOPOLLING; // 轮询等待无响应
  }
  return QSPI_W25Qxx_OK; // 通信正常结束
}

/**
 * @brief  复位Flash器件
 * @retval QSPI_W25Qxx_OK - 复位成功
 * @retval W25Qxx_ERROR_INIT - 复位失败
 * @retval W25Qxx_ERROR_AUTOPOLLING - 轮询超时
 */
int8_t QSPI_W25Qxx_Reset(void) {
  QSPI_CommandTypeDef s_command; // QSPI传输配置

  s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;     // 1线指令模式
  s_command.AddressMode = QSPI_ADDRESS_NONE;               // 无地址模式
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE; // 无交替字节
  s_command.DdrMode = QSPI_DDR_MODE_DISABLE;               // 禁止DDR模式
  s_command.DdrHoldHalfCycle =
      QSPI_DDR_HHC_ANALOG_DELAY; // DDR模式中数据延迟，这里用不到
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD; // 每次传输数据都发送指令
  s_command.DataMode = QSPI_DATA_NONE;           // 无数据模式
  s_command.DummyCycles = 0;                     // 空周期个数
  s_command.Instruction = W25Qxx_CMD_EnableReset; // 执行复位使能命令
  int status1 =
      HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
  // 发送复位使能命令
  if (status1 != HAL_OK) {
    DEBUG_ERROR("QSPI Flash复位使能失败");
    return W25Qxx_ERROR_INIT; // 如果发送失败，返回错误信息
  }
  // 使用自动轮询标志位，等待通信结束
  if (QSPI_W25Qxx_AutoPollingMemReady() != QSPI_W25Qxx_OK) {
    DEBUG_ERROR("QSPI Flash复位使能失败");
    return W25Qxx_ERROR_AUTOPOLLING; // 轮询等待无响应
  }

  s_command.Instruction = W25Qxx_CMD_ResetDevice; // 复位器件命令

  // 发送复位器件命令
  if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    DEBUG_ERROR("QSPI Flash复位使能失败");
    return W25Qxx_ERROR_INIT; // 如果发送失败，返回错误信息
  }
  // 使用自动轮询标志位，等待通信结束
  if (QSPI_W25Qxx_AutoPollingMemReady() != QSPI_W25Qxx_OK) {
    DEBUG_ERROR("QSPI Flash复位使能失败");
    return W25Qxx_ERROR_AUTOPOLLING; // 轮询等待无响应
  }
  return QSPI_W25Qxx_OK; // 复位成功
}

/**
 * @brief  读取Flash器件ID
 * @retval 器件ID（W25Q256为0xef4019）
 * @retval W25Qxx_ERROR_INIT - 通信失败
 * @retval W25Qxx_ERROR_TRANSMIT - 接收失败
 */
uint32_t QSPI_W25Qxx_ReadID(void) {
  QSPI_CommandTypeDef s_command; // QSPI传输配置
  uint8_t QSPI_ReceiveBuff[3];   // 存储QSPI读到的数据
  uint32_t W25Qxx_ID;            // 器件的ID

  s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;     // 1线指令模式
  s_command.AddressSize = QSPI_ADDRESS_32_BITS;            // 32位地址
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE; // 无交替字节
  s_command.DdrMode = QSPI_DDR_MODE_DISABLE;               // 禁止DDR模式
  s_command.DdrHoldHalfCycle =
      QSPI_DDR_HHC_ANALOG_DELAY; // DDR模式中数据延迟，这里用不到
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD; // 每次传输数据都发送指令
  s_command.AddressMode = QSPI_ADDRESS_NONE; // 无地址模式
  s_command.DataMode = QSPI_DATA_1_LINE;     // 1线数据模式
  s_command.DummyCycles = 0;                 // 空周期个数
  s_command.NbData = 3;                      // 传输数据的长度
  s_command.Instruction =
      W25Qxx_CMD_JedecID; // 执行读器件ID命令
                          // int status1 = HAL_QSPI_Command(&hqspi, &s_command,
                          // HAL_QPSI_TIMEOUT_DEFAULT_VALUE); int status2 =
                          // HAL_QSPI_Receive(&hqspi, QSPI_ReceiveBuff,
                          // HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
  // 发送指令
  if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    DEBUG_ERROR("QSPI Flash读取ID失败");
    return 0;
  }
  // 接收数据
  if (HAL_QSPI_Receive(&hqspi, QSPI_ReceiveBuff,
                       HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    DEBUG_ERROR("QSPI Flash读取ID失败");
    return 0;
  }
  // 将得到的数据组合成ID
  W25Qxx_ID = (QSPI_ReceiveBuff[0] << 16) | (QSPI_ReceiveBuff[1] << 8) |
              QSPI_ReceiveBuff[2];

  return W25Qxx_ID; // 返回ID
}

/**
 * @brief  进入内存映射模式
 * @retval QSPI_W25Qxx_OK - 配置成功
 * @retval W25Qxx_ERROR_MemoryMapped - 配置失败
 * @note   此模式下只能读取，不能写入
 */
int8_t QSPI_W25Qxx_MemoryMappedMode(void) {
  QSPI_CommandTypeDef s_command;             // QSPI传输配置
  QSPI_MemoryMappedTypeDef s_mem_mapped_cfg; // 内存映射访问参数

  s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;     // 1线指令模式
  s_command.AddressSize = QSPI_ADDRESS_32_BITS;            // 32位地址
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE; // 无交替字节
  s_command.DdrMode = QSPI_DDR_MODE_DISABLE;               // 禁止DDR模式
  s_command.DdrHoldHalfCycle =
      QSPI_DDR_HHC_ANALOG_DELAY; // DDR模式中数据延迟，这里用不到
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD; // 每次传输数据都发送指令
  s_command.AddressMode = QSPI_ADDRESS_4_LINES; // 4线地址模式
  s_command.DataMode = QSPI_DATA_4_LINES;       // 4线数据模式
  s_command.DummyCycles = 6;                    // 空周期个数
  s_command.Instruction =
      W25Qxx_CMD_FastReadQuad_IO; // 1-4-4模式下(1线指令4线地址4线数据)，快速读取指令

  s_mem_mapped_cfg.TimeOutActivation =
      QSPI_TIMEOUT_COUNTER_DISABLE; // 禁用超时计数器, nCS 保持激活状态
  s_mem_mapped_cfg.TimeOutPeriod = 0; // 超时判断周期

  QSPI_W25Qxx_Reset(); // 复位W25Qxx

  if (HAL_QSPI_MemoryMapped(&hqspi, &s_command, &s_mem_mapped_cfg) !=
      HAL_OK) // 进行配置
  {
    DEBUG_ERROR("QSPI内存映射模式切换失败");
    return W25Qxx_ERROR_MemoryMapped; // 设置内存映射模式错误
  }
  return QSPI_W25Qxx_OK; // 配置成功
}

/**
 * @brief  发送写使能命令
 * @retval QSPI_W25Qxx_OK - 写使能成功
 * @retval W25Qxx_ERROR_WriteEnable - 写使能失败
 * @retval W25Qxx_ERROR_AUTOPOLLING - 轮询等待超时
 */
int8_t QSPI_W25Qxx_WriteEnable(void) {

  HAL_QSPI_Abort(&hqspi); // 先退出内存映射

  QSPI_CommandTypeDef s_command;    // QSPI传输配置
  QSPI_AutoPollingTypeDef s_config; // 轮询比较相关配置参数

  s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;     // 1线指令模式
  s_command.AddressMode = QSPI_ADDRESS_NONE;               // 无地址模式
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE; // 无交替字节
  s_command.DdrMode = QSPI_DDR_MODE_DISABLE;               // 禁止DDR模式
  s_command.DdrHoldHalfCycle =
      QSPI_DDR_HHC_ANALOG_DELAY; // DDR模式中数据延迟，这里用不到
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD; // 每次传输数据都发送指令
  s_command.DataMode = QSPI_DATA_NONE;           // 无数据模式
  s_command.DummyCycles = 0;                     // 空周期个数
  s_command.Instruction = W25Qxx_CMD_WriteEnable; // 发送写使能命令

  // 发送写使能命令
  if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    DEBUG_ERROR("QSPI Flash写使能失败");
    return W25Qxx_ERROR_WriteEnable; //
  }

  // 不停的查询 W25Qxx_CMD_ReadStatus_REG1 寄存器，将读取到的状态字节中的
  // W25Qxx_Status_REG1_WEL 不停的与 0x02 作比较
  // 读状态寄存器1的第1位（只读），WEL写使能标志位，该标志位为1时，代表可以进行写操作

  s_config.Match = 0x02; // 匹配值
  s_config.Mask =
      W25Qxx_Status_REG1_WEL; // 读状态寄存器1的第1位（只读），WEL写使能标志位，该标志位为1时，代表可以进行写操作
  s_config.MatchMode = QSPI_MATCH_MODE_AND;            // 与运算
  s_config.StatusBytesSize = 1;                        // 状态字节数
  s_config.Interval = 0x10;                            // 轮询间隔
  s_config.AutomaticStop = QSPI_AUTOMATIC_STOP_ENABLE; // 自动停止模式

  s_command.Instruction = W25Qxx_CMD_ReadStatus_REG1; // 读状态信息寄存器
  s_command.DataMode = QSPI_DATA_1_LINE;              // 1线数据模式
  s_command.NbData = 1;                               // 数据长度

  // 发送轮询等待命令
  if (HAL_QSPI_AutoPolling(&hqspi, &s_command, &s_config,
                           HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
    DEBUG_ERROR("QSPI Flash写使能失败");
    return W25Qxx_ERROR_AUTOPOLLING; // 轮询等待无响应
  }
  return QSPI_W25Qxx_OK; // 通信正常结束
}

/**
 * @brief  扇区擦除（4KB）
 * @param  SectorAddress: 扇区地址（4KB对齐）
 * @retval QSPI_W25Qxx_OK - 擦除成功
 * @retval W25Qxx_ERROR_WriteEnable - 写使能失败
 * @retval W25Qxx_ERROR_Erase - 擦除失败
 * @retval W25Qxx_ERROR_AUTOPOLLING - 轮询等待无响应
 * @note   典型耗时：45ms，最大：400ms
 * @note   Flash使用时间越长，擦除所需时间越长
 */
int8_t QSPI_W25Qxx_SectorErase(uint32_t SectorAddress) {
  QSPI_CommandTypeDef s_command; // QSPI传输配置

  s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;     // 1线指令模式
  s_command.AddressSize = QSPI_ADDRESS_32_BITS;            // 32位地址
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE; //	无交替字节
  s_command.DdrMode = QSPI_DDR_MODE_DISABLE;               // 禁止DDR模式
  s_command.DdrHoldHalfCycle =
      QSPI_DDR_HHC_ANALOG_DELAY; // DDR模式中数据延迟，这里用不到
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD; // 每次传输数据都发送指令
  s_command.AddressMode = QSPI_ADDRESS_1_LINE;    // 1线地址模式
  s_command.DataMode = QSPI_DATA_NONE;            // 无数据
  s_command.DummyCycles = 0;                      // 空周期个数
  s_command.Address = SectorAddress;              // 要擦除的地址
  s_command.Instruction = W25Qxx_CMD_SectorErase; // 扇区擦除命令

  // 发送写使能
  if (QSPI_W25Qxx_WriteEnable() != QSPI_W25Qxx_OK) {
    DEBUG_ERROR("QSPI Flash写使能失败");
    return W25Qxx_ERROR_WriteEnable; // 写使能失败
  }
  // 发出擦除命令
  if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    DEBUG_ERROR("QSPI Flash擦除命令发送失败");
    return W25Qxx_ERROR_Erase; // 擦除失败
  }
  // 使用自动轮询标志位，等待擦除的结束
  if (QSPI_W25Qxx_AutoPollingMemReady() != QSPI_W25Qxx_OK) {
    DEBUG_ERROR("QSPI Flash擦除失败");
    return W25Qxx_ERROR_AUTOPOLLING; // 轮询等待无响应
  }
  return QSPI_W25Qxx_OK; // 擦除成功
}

/**
 * @brief  块擦除（64KB）
 * @param  SectorAddress: 块地址（64KB对齐）
 * @retval QSPI_W25Qxx_OK - 擦除成功
 * @retval W25Qxx_ERROR_WriteEnable - 写使能失败
 * @retval W25Qxx_ERROR_Erase - 擦除失败
 * @retval W25Qxx_ERROR_AUTOPOLLING - 轮询等待无响应
 * @note   典型耗时：150ms，最大：2000ms
 * @note   推荐使用：大数据量擦除速度最快
 * @note   Flash使用时间越长，擦除所需时间越长
 */
int8_t QSPI_W25Qxx_BlockErase_64K(uint32_t SectorAddress) {
  QSPI_CommandTypeDef s_command; // QSPI传输配置

  s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;     // 1线指令模式
  s_command.AddressSize = QSPI_ADDRESS_32_BITS;            // 32位地址
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE; //	无交替字节
  s_command.DdrMode = QSPI_DDR_MODE_DISABLE;               // 禁止DDR模式
  s_command.DdrHoldHalfCycle =
      QSPI_DDR_HHC_ANALOG_DELAY; // DDR模式中数据延迟，这里用不到
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD; // 每次传输数据都发送指令
  s_command.AddressMode = QSPI_ADDRESS_1_LINE; // 1线地址模式
  s_command.DataMode = QSPI_DATA_NONE;         // 无数据
  s_command.DummyCycles = 0;                   // 空周期个数
  s_command.Address = SectorAddress;           // 要擦除的地址
  s_command.Instruction =
      W25Qxx_CMD_BlockErase_64K; // 块擦除命令，每次擦除64K字节

  // 发送写使能
  if (QSPI_W25Qxx_WriteEnable() != QSPI_W25Qxx_OK) {
    DEBUG_ERROR("QSPI Flash写使能失败");
    return W25Qxx_ERROR_WriteEnable; // 写使能失败
  }
  // 发出擦除命令
  if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    DEBUG_ERROR("QSPI Flash擦除命令发送失败");
    return W25Qxx_ERROR_Erase; // 擦除失败
  }
  // 使用自动轮询标志位，等待擦除的结束
  if (QSPI_W25Qxx_AutoPollingMemReady() != QSPI_W25Qxx_OK) {
    DEBUG_ERROR("QSPI Flash擦除失败");
    return W25Qxx_ERROR_AUTOPOLLING; // 轮询等待无响应
  }
  return QSPI_W25Qxx_OK; // 擦除成功
}

/**
 * @brief  整片擦除
 * @retval QSPI_W25Qxx_OK - 擦除成功
 * @retval W25Qxx_ERROR_WriteEnable - 写使能失败
 * @retval W25Qxx_ERROR_Erase - 擦除失败
 * @retval W25Qxx_ERROR_AUTOPOLLING - 轮询等待无响应
 * @note   典型耗时：80s，最大：400s
 * @note   危险操作：会清除所有数据
 * @note   Flash使用时间越长，擦除所需时间越长
 */
int8_t QSPI_W25Qxx_ChipErase(void) {
  QSPI_CommandTypeDef s_command;    // QSPI传输配置
  QSPI_AutoPollingTypeDef s_config; // 轮询等待配置参数

  s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;     // 1线指令模式
  s_command.AddressSize = QSPI_ADDRESS_32_BITS;            // 32位地址
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE; //	无交替字节
  s_command.DdrMode = QSPI_DDR_MODE_DISABLE;               // 禁止DDR模式
  s_command.DdrHoldHalfCycle =
      QSPI_DDR_HHC_ANALOG_DELAY; // DDR模式中数据延迟，这里用不到
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD; // 每次传输数据都发送指令
  s_command.AddressMode = QSPI_ADDRESS_NONE; // 无地址
  s_command.DataMode = QSPI_DATA_NONE;       // 无数据
  s_command.DummyCycles = 0;                 // 空周期个数
  s_command.Instruction = W25Qxx_CMD_ChipErase; // 擦除命令，进行整片擦除

  // 发送写使能
  if (QSPI_W25Qxx_WriteEnable() != QSPI_W25Qxx_OK) {
    DEBUG_ERROR("QSPI Flash写使能失败");
    return W25Qxx_ERROR_WriteEnable; // 写使能失败
  }
  // 发出擦除命令
  if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    DEBUG_ERROR("QSPI Flash擦除命令发送失败");
    return W25Qxx_ERROR_Erase; // 擦除失败
  }

  // 不停的查询 W25Qxx_CMD_ReadStatus_REG1 寄存器，将读取到的状态字节中的
  // W25Qxx_Status_REG1_BUSY 不停的与0作比较
  // 读状态寄存器1的第0位（只读），Busy标志位，当正在擦除/写入数据/写命令时会被置1，空闲或通信结束为0

  s_config.Match = 0;                                  //	匹配值
  s_config.MatchMode = QSPI_MATCH_MODE_AND;            //	与运算
  s_config.Interval = 0x10;                            //	轮询间隔
  s_config.AutomaticStop = QSPI_AUTOMATIC_STOP_ENABLE; // 自动停止模式
  s_config.StatusBytesSize = 1;                        //	状态字节数
  s_config.Mask =
      W25Qxx_Status_REG1_BUSY; // 对在轮询模式下接收的状态字节进行屏蔽，只比较需要用到的位

  s_command.Instruction = W25Qxx_CMD_ReadStatus_REG1; // 读状态信息寄存器
  s_command.DataMode = QSPI_DATA_1_LINE;              // 1线数据模式
  s_command.NbData = 1;                               // 数据长度

  // W25Q256整片擦除的典型参考时间为20s，最大时间为100s，这里的超时等待值
  // W25Qxx_ChipErase_TIMEOUT_MAX 为 100S
  if (HAL_QSPI_AutoPolling(&hqspi, &s_command, &s_config,
                           W25Qxx_ChipErase_TIMEOUT_MAX) != HAL_OK) {
    DEBUG_ERROR("QSPI Flash擦除失败");
    return W25Qxx_ERROR_AUTOPOLLING; // 轮询等待无响应
  }
  return QSPI_W25Qxx_OK;
}

/**
 * @brief  页写入（最大256字节）
 * @param  pBuffer: 数据缓冲区指针
 * @param  WriteAddr: 写入地址
 * @param  NumByteToWrite: 写入字节数（≤256）
 * @retval QSPI_W25Qxx_OK - 写入成功
 * @retval W25Qxx_ERROR_WriteEnable - 写使能失败
 * @retval W25Qxx_ERROR_TRANSMIT - 传输失败
 * @retval W25Qxx_ERROR_AUTOPOLLING - 轮询等待无响应
 * @note   写入前必须先擦除
 * @note   典型耗时：0.4ms/页，最大：3ms
 * @note   Flash使用时间越长，写入所需时间越长
 */
int8_t QSPI_W25Qxx_WritePage(uint8_t *pBuffer, uint32_t WriteAddr,
                             uint16_t NumByteToWrite) {
  QSPI_CommandTypeDef s_command; // QSPI传输配置

  s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;     // 1线指令模式
  s_command.AddressSize = QSPI_ADDRESS_32_BITS;            // 32位地址
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE; // 无交替字节
  s_command.DdrMode = QSPI_DDR_MODE_DISABLE;               // 禁止DDR模式
  s_command.DdrHoldHalfCycle =
      QSPI_DDR_HHC_ANALOG_DELAY; // DDR模式中数据延迟，这里用不到
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD; // 每次传输数据都发送指令
  s_command.AddressMode = QSPI_ADDRESS_1_LINE; // 1线地址模式
  s_command.DataMode = QSPI_DATA_4_LINES;      // 4线数据模式
  s_command.DummyCycles = 0;                   // 空周期个数
  s_command.NbData = NumByteToWrite; // 数据长度，最大只能256字节
  s_command.Address = WriteAddr;     // 要写入 W25Qxx 的地址
  s_command.Instruction =
      W25Qxx_CMD_QuadInputPageProgram; // 1-1-4模式下(1线指令1线地址4线数据)，页编程指令

  // 写使能
  if (QSPI_W25Qxx_WriteEnable() != QSPI_W25Qxx_OK) {
    DEBUG_ERROR("QSPI Flash写使能失败");
    return W25Qxx_ERROR_WriteEnable; // 写使能失败
  }
  // 写命令
  if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    DEBUG_ERROR("QSPI Flash写命令发送失败");
    return W25Qxx_ERROR_TRANSMIT; // 传输数据错误
  }
  // 开始传输数据
  if (HAL_QSPI_Transmit(&hqspi, pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    DEBUG_ERROR("QSPI Flash写数据失败");
    return W25Qxx_ERROR_TRANSMIT; // 传输数据错误
  }
  // 使用自动轮询标志位，等待写入的结束
  if (QSPI_W25Qxx_AutoPollingMemReady() != QSPI_W25Qxx_OK) {
    DEBUG_ERROR("QSPI Flash写入失败");
    return W25Qxx_ERROR_AUTOPOLLING; // 轮询等待无响应
  }
  return QSPI_W25Qxx_OK; // 写数据成功
}

/**
 * @brief  缓冲区写入（任意长度）
 * @param  pBuffer: 数据缓冲区指针
 * @param  WriteAddr: 写入地址
 * @param  Size: 写入字节数（≤Flash容量）
 * @retval QSPI_W25Qxx_OK - 写入成功
 * @retval W25Qxx_ERROR_WriteEnable - 写使能失败
 * @retval W25Qxx_ERROR_TRANSMIT - 传输失败
 * @retval W25Qxx_ERROR_AUTOPOLLING - 轮询等待无响应
 * @note   写入前必须先擦除
 * @note   自动跨页处理
 * @note   Flash使用时间越长，写入所需时间越长
 */
int8_t QSPI_W25Qxx_WriteBuffer(uint8_t *pBuffer, uint32_t WriteAddr,
                               uint32_t Size) {
  uint32_t end_addr, current_size, current_addr;
  uint8_t *write_data; // 要写入的数据

  current_size =
      W25Qxx_PageSize - (WriteAddr % W25Qxx_PageSize); // 计算当前页还剩余的空间

  if (current_size > Size) // 判断当前页剩余的空间是否足够写入所有数据
  {
    current_size = Size; // 如果足够，则直接获取当前长度
  }

  current_addr = WriteAddr;    // 获取要写入的地址
  end_addr = WriteAddr + Size; // 计算结束地址
  write_data = pBuffer;        // 获取要写入的数据

  do {
    // 发送写使能
    if (QSPI_W25Qxx_WriteEnable() != QSPI_W25Qxx_OK) {
      DEBUG_ERROR("QSPI Flash写使能失败");
      return W25Qxx_ERROR_WriteEnable;
    }

    // 按页写入数据
    else if (QSPI_W25Qxx_WritePage(write_data, current_addr, current_size) !=
             QSPI_W25Qxx_OK) {
      DEBUG_ERROR("QSPI Flash写入失败");
      return W25Qxx_ERROR_TRANSMIT;
    }

    // 使用自动轮询标志位，等待写入的结束
    else if (QSPI_W25Qxx_AutoPollingMemReady() != QSPI_W25Qxx_OK) {
      DEBUG_ERROR("QSPI Flash写入失败");
      return W25Qxx_ERROR_AUTOPOLLING;
    }

    else // 按页写入数据成功，进行下一次写数据的准备工作
    {
      current_addr += current_size; // 计算下一次要写入的地址
      write_data += current_size; // 获取下一次要写入的数据存储区地址
      // 计算下一次写数据的长度
      current_size = ((current_addr + W25Qxx_PageSize) > end_addr)
                         ? (end_addr - current_addr)
                         : W25Qxx_PageSize;
    }
  } while (current_addr < end_addr); // 判断数据是否全部写入完毕
  return QSPI_W25Qxx_OK; // 写入数据成功
}

/**
 * @brief  缓冲区读取（任意长度）
 * @param  pBuffer: 数据缓冲区指针
 * @param  ReadAddr: 读取地址
 * @param  NumByteToRead: 读取字节数（≤Flash容量）
 * @retval QSPI_W25Qxx_OK - 读取成功
 * @retval W25Qxx_ERROR_TRANSMIT - 传输失败
 * @retval W25Qxx_ERROR_AUTOPOLLING - 轮询等待无响应
 * @note   使用1-4-4模式快速读取（Fast Read Quad I/O, 0xEB指令）
 * @note   读取速度受QSPI时钟、DMA、Cache、编译器优化等级影响
 * @note   数据存储位置（TCM SRAM / AXI SRAM）也会影响性能
 * @note   W25Q256JV最高驱动频率为133MHz
 */
int8_t QSPI_W25Qxx_ReadBuffer(uint8_t *pBuffer, uint32_t ReadAddr,
                              uint32_t NumByteToRead) {
  QSPI_CommandTypeDef s_command; // QSPI传输配置

  s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;     // 1线指令模式
  s_command.AddressSize = QSPI_ADDRESS_32_BITS;            // 32位地址
  s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE; // 无交替字节
  s_command.DdrMode = QSPI_DDR_MODE_DISABLE;               // 禁止DDR模式
  s_command.DdrHoldHalfCycle =
      QSPI_DDR_HHC_ANALOG_DELAY; // DDR模式中数据延迟，这里用不到
  s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD; // 每次传输数据都发送指令
  s_command.AddressMode = QSPI_ADDRESS_4_LINES; // 4线地址模式
  s_command.DataMode = QSPI_DATA_4_LINES;       // 4线数据模式
  s_command.DummyCycles = 6;                    // 空周期个数
  s_command.NbData = NumByteToRead; // 数据长度，最大不能超过flash芯片的大小
  s_command.Address = ReadAddr; // 要读取 W25Qxx 的地址
  s_command.Instruction =
      W25Qxx_CMD_FastReadQuad_IO; // 1-4-4模式下(1线指令4线地址4线数据)，快速读取指令

  // 发送读取命令
  if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    DEBUG_ERROR("QSPI Flash读取ID失败");
    return W25Qxx_ERROR_TRANSMIT; // 传输数据错误
  }

  //	接收数据

  if (HAL_QSPI_Receive(&hqspi, pBuffer, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) !=
      HAL_OK) {
    DEBUG_ERROR("QSPI Flash读取数据失败");
    return W25Qxx_ERROR_TRANSMIT; // 传输数据错误
  }

  // 使用自动轮询标志位，等待接收的结束
  if (QSPI_W25Qxx_AutoPollingMemReady() != QSPI_W25Qxx_OK) {
    DEBUG_ERROR("QSPI Flash读取数据失败");
    return W25Qxx_ERROR_AUTOPOLLING; // 轮询等待无响应
  }
  return QSPI_W25Qxx_OK; // 读取数据成功
}

#endif
