/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : usbd_dfu_if.c
 * @brief          : Usb device for Download Firmware Update.
 *                   支持内部 Flash 和外部 QSPI Flash
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "usbd_dfu_if.h"
#if defined(USB_DEVICE_ENABLE) && (MY_USB_DEVICE_CLASS == USE_USB_DFU)
/* USER CODE BEGIN INCLUDE */
#include "stm32h7xx_hal.h"
#include <string.h>

#ifdef QSPI_FLASH_ENABLE
#include "qspi_flash.h"
#endif

/* USER CODE END INCLUDE */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* 动态 Flash 描述符字符串 */
// static uint8_t FLASH_DESC_STR_BUFFER[256];

/* USER CODE END PV */

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
 * @brief Usb device.
 * @{
 */

/** @defgroup USBD_DFU
 * @brief Usb DFU device module.
 * @{
 */

/** @defgroup USBD_DFU_Private_TypesDefinitions
 * @brief Private types.
 * @{
 */

/* USER CODE BEGIN PRIVATE_TYPES */

/* USER CODE END PRIVATE_TYPES */

/**
 * @}
 */

/** @defgroup USBD_DFU_Private_Defines
 * @brief Private defines.
 * @{
 */
// #define FLASH_DESC_STR "@Internal Flash   /0x08000000/03*016Ka,01*016Kg,01*064Kg,07*128Kg,04*016Kg,01*064Kg,07*128Kg"
/* DFU 描述符格式：
 * @Internal Flash /0x08100000/1024*128Kg
 * 或
 * @QSPI Flash /0x90000000/128*256Kg
 *
 * 格式说明：
 * @ - 开始标记
 * Internal Flash - 存储器名称
 * /0x08100000 - 起始地址
 * /10*128Kg - 大小信息
 *   10 = 块数
 *   128K = 每块大小（K=1024字节，g=可读可写可擦除）
 * 块类型代码: 01=只读, 02=可读写, 03=可擦除, g=可读可写可擦除
 */

#if (MY_DFU_MEDIA_MODE == MODE_INTERNAL_FLASH)
#define FLASH_DESC_STR "@Internal Flash   /0x08100000/10*128Kg"
#elif (MY_DFU_MEDIA_MODE == MODE_EXTERNAL_FLASH)
/*  512个 64KB 块 (32MB total) */
#define FLASH_DESC_STR "@QSPI Flash       /0x90000000/512*64Kg"
#endif

/* USER CODE BEGIN PRIVATE_DEFINES */

/* USER CODE END PRIVATE_DEFINES */

/**
 * @}
 */

/** @defgroup USBD_DFU_Private_Macros
 * @brief Private macros.
 * @{
 */

/* USER CODE BEGIN PRIVATE_MACRO */

/* USER CODE END PRIVATE_MACRO */

/**
 * @}
 */

/** @defgroup USBD_DFU_Private_Variables
 * @brief Private variables.
 * @{
 */

/* USER CODE BEGIN PRIVATE_VARIABLES */

/* USER CODE END PRIVATE_VARIABLES */

/**
 * @}
 */

/** @defgroup USBD_DFU_Exported_Variables
 * @brief Public variables.
 * @{
 */

extern USBD_HandleTypeDef hUsbDeviceFS;

/* USER CODE BEGIN EXPORTED_VARIABLES */

/* USER CODE END EXPORTED_VARIABLES */

/**
 * @}
 */

/** @defgroup USBD_DFU_Private_FunctionPrototypes
 * @brief Private functions declaration.
 * @{
 */

static uint16_t MEM_If_Init_FS(void);
static uint16_t MEM_If_Erase_FS(uint32_t Add);
static uint16_t MEM_If_Write_FS(uint8_t *src, uint8_t *dest, uint32_t Len);
static uint8_t *MEM_If_Read_FS(uint8_t *src, uint8_t *dest, uint32_t Len);
static uint16_t MEM_If_DeInit_FS(void);
static uint16_t MEM_If_GetStatus_FS(uint32_t Add, uint8_t Cmd, uint8_t *buffer);

/* USER CODE BEGIN PRIVATE_FUNCTIONS_DECLARATION */

/* 内部函数 */
#if MY_DFU_MEDIA_MODE == MODE_INTERNAL_FLASH
static int8_t DFU_InternalFlash_Erase(uint32_t addr);
static int8_t DFU_InternalFlash_Write(uint32_t addr, uint8_t *data, uint32_t len);
#elif MY_DFU_MEDIA_MODE == MODE_EXTERNAL_FLASH
static int8_t DFU_ExternalFlash_Erase(uint32_t addr);
static int8_t DFU_ExternalFlash_Write(uint32_t addr, uint8_t *data, uint32_t len);
#endif
/* USER CODE END PRIVATE_FUNCTIONS_DECLARATION */

/**
 * @}
 */

#if defined(__ICCARM__) /* IAR Compiler */
#pragma data_alignment = 4
#endif
__ALIGN_BEGIN USBD_DFU_MediaTypeDef USBD_DFU_fops_FS __ALIGN_END =
    {
        (uint8_t *)FLASH_DESC_STR,
        MEM_If_Init_FS,
        MEM_If_DeInit_FS,
        MEM_If_Erase_FS,
        MEM_If_Write_FS,
        MEM_If_Read_FS,
        MEM_If_GetStatus_FS};

/* Private functions ---------------------------------------------------------*/
#if MY_DFU_MEDIA_MODE == MODE_INTERNAL_FLASH
/**
 * @brief  内部 Flash 擦除（按扇区）
 * @param  addr: 擦除地址
 * @retval USBD_OK 或 USBD_FAIL
 */
static int8_t DFU_InternalFlash_Erase(uint32_t addr)
{
  FLASH_EraseInitTypeDef EraseInit;
  uint32_t SectorError = 0;
  HAL_StatusTypeDef status;

  /* 检查地址有效性 */
  if ((addr < INTERNAL_FLASH_APP_ADDR) || (addr >= INTERNAL_FLASH_END_ADDR))
  {
    return USBD_FAIL;
  }

  /* 配置擦除参数 */
  EraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;
  EraseInit.Banks = FLASH_BANK_1;
  EraseInit.Sector = (addr - INTERNAL_FLASH_APP_ADDR) / 0x20000; /* 每个扇区 128KB */
  EraseInit.NbSectors = 1;
  EraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;

  /* 解锁 Flash */
  HAL_FLASH_Unlock();

  /* 执行擦除 */
  status = HAL_FLASHEx_Erase(&EraseInit, &SectorError);

  /* 锁定 Flash */
  HAL_FLASH_Lock();

  return (status == HAL_OK) ? USBD_OK : USBD_FAIL;
}

/**
 * @brief  内部 Flash 写入
 * @param  addr: 写入地址
 * @param  data: 数据指针
 * @param  len: 数据长度
 * @retval USBD_OK 或 USBD_FAIL
 */
static int8_t DFU_InternalFlash_Write(uint32_t addr, uint8_t *data, uint32_t len)
{
  uint32_t i = 0;
  uint32_t data32;
  HAL_StatusTypeDef status = HAL_OK;

  /* 检查地址有效性 */
  if ((addr < INTERNAL_FLASH_APP_ADDR) ||
      ((addr + len) > INTERNAL_FLASH_END_ADDR))
  {
    return USBD_FAIL;
  }

  /* 解锁 Flash */
  HAL_FLASH_Unlock();

  /* 按 32 位写入 */
  for (i = 0; i < len; i += 4)
  {
    data32 = ((uint32_t)data[i] << 0) |
             ((uint32_t)data[i + 1] << 8) |
             ((uint32_t)data[i + 2] << 16) |
             ((uint32_t)data[i + 3] << 24);

    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD,
                               addr + i, data32);
    if (status != HAL_OK)
    {
      HAL_FLASH_Lock();
      return USBD_FAIL;
    }
  }

  /* 锁定 Flash */
  HAL_FLASH_Lock();

  return USBD_OK;
}
#elif MY_DFU_MEDIA_MODE == MODE_EXTERNAL_FLASH
/**
 * @brief  外部 QSPI Flash 擦除
 * @param  addr: 擦除地址（相对于 QSPI 起始地址 0x00000000）
 * @retval USBD_OK 或 USBD_FAIL
 */
static int8_t DFU_ExternalFlash_Erase(uint32_t addr)
{
/* 增加宏检查，防止因宏未定义导致直接返回 FAIL */
#ifndef QSPI_FLASH_ENABLE
#warning "QSPI_FLASH_ENABLE not defined in usbd_dfu_if.c! Check config.h"
#define QSPI_FLASH_ENABLE
#endif

#ifdef QSPI_FLASH_ENABLE
  int8_t result;

  /* 检查地址有效性（移除不必要的零比较） */
  if (addr >= EXTERNAL_FLASH_END_ADDR)
  {
    return USBD_FAIL;
  }

  /* 使用 64KB 块擦除（比 4KB 扇区快） */
  /* addr 必须 64KB 对齐 */
  if ((addr & 0xFFFF) != 0)
  {
    return USBD_FAIL; /* 地址不对齐 */
  }

  /*
   * 使用阻塞式擦除。
   * 注意：QSPI_W25Qxx_BlockErase_64K 内部有 AutoPolling (阻塞)。
   * 只要 SysTick 优先级 > USB 中断优先级，这里会阻塞约 150ms 直到擦除完成。
   */
  result = QSPI_W25Qxx_BlockErase_64K(addr);
  return (result == QSPI_W25Qxx_OK) ? USBD_OK : USBD_FAIL;
#else
  return USBD_FAIL;
#endif
}

/**
 * @brief  外部 QSPI Flash 写入
 * @param  addr: 写入地址（相对于 QSPI 起始地址 0x00000000）
 * @param  data: 数据指针
 * @param  len: 数据长度
 * @retval USBD_OK 或 USBD_FAIL
 */
static int8_t DFU_ExternalFlash_Write(uint32_t addr, uint8_t *data, uint32_t len)
{
#ifdef QSPI_FLASH_ENABLE
  int8_t result;

  /* 检查地址有效性（移除不必要的零比较） */
  if ((addr + len) > EXTERNAL_FLASH_END_ADDR)
  {
    return USBD_FAIL;
  }

  /* 调用 QSPI Flash 写入函数 */
  result = QSPI_W25Qxx_WriteBuffer(data, addr, len);
  return (result == QSPI_W25Qxx_OK) ? USBD_OK : USBD_FAIL;
#else
  return USBD_FAIL;
#endif
}
#endif
/**
 * @brief  Memory initialization routine.
 * @retval USBD_OK if operation is successful, MAL_FAIL else.
 */
uint16_t MEM_If_Init_FS(void)
{
  /* USER CODE BEGIN 0 */

#if (MY_DFU_MEDIA_MODE == MODE_INTERNAL_FLASH)
  /* 内部 Flash：仅解锁即可 */
  HAL_FLASH_Unlock();
  return USBD_OK;

#elif (MY_DFU_MEDIA_MODE == MODE_EXTERNAL_FLASH)
  /* 外部 QSPI Flash：初始化 QSPI 驱动 */
#ifdef QSPI_FLASH_ENABLE
  /* 必须退出内存映射模式才能进行擦写操作 */
  /* 重新初始化或复位 QSPI 到间接模式 */
  /* 1. 显式退出内存映射模式，防止 QSPI_W25Qxx_Init 内部 Reset 失败 */
  extern QSPI_HandleTypeDef hqspi;
  HAL_QSPI_Abort(&hqspi);

  /* 2. 重新初始化或复位 QSPI 到间接模式 */
  if (QSPI_W25Qxx_Init() != QSPI_W25Qxx_OK)
  {
    return USBD_FAIL;
  }
  return USBD_OK;
#else
  /* 如果宏未定义，也需要返回一个状态，否则是未定义行为 */
  return USBD_FAIL;
#endif

#endif


  /* USER CODE END 0 */
}

/**
 * @brief  De-Initializes Memory
 * @retval USBD_OK if operation is successful, MAL_FAIL else
 */
uint16_t MEM_If_DeInit_FS(void)
{
  /* USER CODE BEGIN 1 */

#if (MY_DFU_MEDIA_MODE == MODE_INTERNAL_FLASH)
  /* 内部 Flash：锁定 */
  HAL_FLASH_Lock();
  return USBD_OK;

#elif (MY_DFU_MEDIA_MODE == MODE_EXTERNAL_FLASH)
  /* 外部 QSPI Flash */
  QSPI_W25Qxx_MemoryMappedMode();
  return USBD_OK;

#endif

  /* USER CODE END 1 */
}

/**
 * @brief  Erase sector.
 * @param  Add: Address of sector to be erased.
 * @retval 0 if operation is successful, MAL_FAIL else.
 */
uint16_t MEM_If_Erase_FS(uint32_t Add)
{
  /* USER CODE BEGIN 2 */

#if (MY_DFU_MEDIA_MODE == MODE_INTERNAL_FLASH)
  return DFU_InternalFlash_Erase(Add);

#elif (MY_DFU_MEDIA_MODE == MODE_EXTERNAL_FLASH)
  /* Add 是 DFU 协议传来的地址 (如 0x90000000)，需要转换为 QSPI 内部偏移地址 */
  /* 使用掩码去除高位：0x90000000 -> 0x00000000 */
  uint32_t qspi_addr = Add & 0x0FFFFFFF;
  return DFU_ExternalFlash_Erase(qspi_addr);

#else
  return USBD_FAIL;
#endif

  /* USER CODE END 2 */
}

/**
 * @brief  Memory write routine.
 * @param  src: Pointer to the source buffer. Address to be written to.
 * @param  dest: Pointer to the destination buffer.
 * @param  Len: Number of data to be written (in bytes).
 * @retval USBD_OK if operation is successful, MAL_FAIL else.
 */
uint16_t MEM_If_Write_FS(uint8_t *src, uint8_t *dest, uint32_t Len)
{
  /* USER CODE BEGIN 3 */

#if (MY_DFU_MEDIA_MODE == MODE_INTERNAL_FLASH)
  return DFU_InternalFlash_Write((uint32_t)dest, src, Len);

#elif (MY_DFU_MEDIA_MODE == MODE_EXTERNAL_FLASH)
  /* dest 是来自 DFU 协议的虚拟地址，转换为 QSPI 实际地址 */
  uint32_t qspi_addr = (uint32_t)dest & 0x01FFFFFF; /* 移除高位地址映射 */
  return DFU_ExternalFlash_Write(qspi_addr, src, Len);

#else
  return USBD_FAIL;
#endif

  /* USER CODE END 3 */
}

/**
 * @brief  Memory read routine.
 * @param  src: Pointer to the source buffer. Address to be written to.
 * @param  dest: Pointer to the destination buffer.
 * @param  Len: Number of data to be read (in bytes).
 * @retval Pointer to the physical address where data should be read.
 */
uint8_t *MEM_If_Read_FS(uint8_t *src, uint8_t *dest, uint32_t Len)
{
  /* USER CODE BEGIN 4 */

#if (MY_DFU_MEDIA_MODE == MODE_INTERNAL_FLASH)
  /* 内部 Flash：直接内存复制 */
  memcpy(dest, src, Len);
  return dest;

#elif (MY_DFU_MEDIA_MODE == MODE_EXTERNAL_FLASH)
  /* 外部 QSPI Flash：调用 QSPI 读取 */
#ifdef QSPI_FLASH_ENABLE

  uint32_t qspi_addr = (uint32_t)src & 0x01FFFFFF; /* 移除高位地址映射 */
  if (QSPI_W25Qxx_ReadBuffer(dest, qspi_addr, Len) == QSPI_W25Qxx_OK)
  {
    return dest;
  }
  return NULL; /* 读取失败 */
#else
  return NULL;
#endif

#else
  return NULL;
#endif

  /* USER CODE END 4 */
}

/**
 * @brief  Get status routine
 * @param  Add: Address to be read from
 * @param  Cmd: Number of data to be read (in bytes)
 * @param  buffer: used for returning the time necessary for a program or an erase operation
 * @retval USBD_OK if operation is successful
 */
uint16_t MEM_If_GetStatus_FS(uint32_t Add, uint8_t Cmd, uint8_t *buffer)
{
  /* USER CODE BEGIN 5 */
  UNUSED(Add);

  switch (Cmd)
  {
  case DFU_MEDIA_PROGRAM:
    /* 编程时间（单位：毫秒） */
#if (MY_DFU_MEDIA_MODE == MODE_INTERNAL_FLASH)
    /* 内部 Flash：约 0.4ms/256B */
    buffer[0] = 0x01;
    buffer[1] = 0x00;
    buffer[2] = 0x00;
#elif (MY_DFU_MEDIA_MODE == MODE_EXTERNAL_FLASH)

    buffer[0] = 0x00;
    buffer[1] = 0x00;
    buffer[2] = 0x00;
#endif
    break;

  case DFU_MEDIA_ERASE:
    /* 擦除时间（单位：毫秒） */
#if (MY_DFU_MEDIA_MODE == MODE_INTERNAL_FLASH)
    /* 内部 Flash：128KB 扇区约 50ms */
    buffer[0] = 50;
    buffer[1] = 0x00;
    buffer[2] = 0x00;
#elif (MY_DFU_MEDIA_MODE == MODE_EXTERNAL_FLASH)
    /*
     * QSPI Flash：
     * 由于 DFU_ExternalFlash_Erase 是阻塞式的（返回时已擦除完毕），
     * 这里必须返回 0 时间。告诉主机"我已经完成了，不需要再等"。
     * 如果返回 500ms，主机空等 500ms 后再查询，虽然理论上可行，但容易掩盖状态机问题。
     */
    buffer[0] = 0x00;
    buffer[1] = 0x00;
    buffer[2] = 0x00;
#endif
    break;

  default:
    break;
  }
  return USBD_OK;
  /* USER CODE END 5 */
}

/* USER CODE BEGIN PRIVATE_FUNCTIONS_IMPLEMENTATION */

/* USER CODE END PRIVATE_FUNCTIONS_IMPLEMENTATION */

/**
 * @}
 */

/**
 * @}
 */

#endif
