/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : usbd_dfu_if.h
 * @brief          : Header for usbd_dfu_if.c file.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USBD_DFU_IF_H__
#define __USBD_DFU_IF_H__

#ifdef __cplusplus
extern "C"
{
#endif
#include "config.h"
#include "usbd_conf.h"
#if defined(USB_DEVICE_ENABLE) && (MY_USB_DEVICE_CLASS == USE_USB_DFU)
  /* Includes ------------------------------------------------------------------*/
#include "usbd_dfu.h"

  /* USER CODE BEGIN INCLUDE */
#define MODE_INTERNAL_FLASH 0
#define MODE_EXTERNAL_FLASH 1
#define MY_DFU_MEDIA_MODE MODE_EXTERNAL_FLASH

/* ===== 内部 Flash 配置 ===== */
#define INTERNAL_FLASH_APP_ADDR 0x08100000 /* 应用程序起始地址 */
#define INTERNAL_FLASH_END_ADDR 0x08200000 /* Flash 结束地址 */
#define INTERNAL_FLASH_SIZE (INTERNAL_FLASH_END_ADDR - INTERNAL_FLASH_APP_ADDR)

/* ===== 外部 QSPI Flash 配置 ===== */
#define EXTERNAL_FLASH_APP_ADDR 0x00000000 /* QSPI 应用程序起始地址 */
#define EXTERNAL_FLASH_END_ADDR 0x02000000 /* QSPI 结束地址（32MB） */
#define EXTERNAL_FLASH_SIZE (EXTERNAL_FLASH_END_ADDR - EXTERNAL_FLASH_APP_ADDR)

  /* USER CODE END INCLUDE */

  /** @addtogroup STM32_USB_DEVICE_LIBRARY
   * @brief For Usb device.
   * @{
   */

  /** @defgroup USBD_MEDIA USBD_MEDIA
   * @brief Header file for the usbd_dfu_if.c file.
   * @{
   */

  /** @defgroup USBD_MEDIA_Exported_Defines USBD_MEDIA_Exported_Defines
   * @brief Defines.
   * @{
   */

  /* USER CODE BEGIN EXPORTED_DEFINES */

  /* USER CODE END EXPORTED_DEFINES */

  /**
   * @}
   */

  /** @defgroup USBD_MEDIA_Exported_Types USBD_MEDIA_Exported_Types
   * @brief Types.
   * @{
   */

  /* USER CODE BEGIN EXPORTED_TYPES */

  /* USER CODE END EXPORTED_TYPES */

  /**
   * @}
   */

  /** @defgroup USBD_MEDIA_Exported_Macros USBD_MEDIA_Exported_Macros
   * @brief Aliases.
   * @{
   */

  /* USER CODE BEGIN EXPORTED_MACRO */

  /* USER CODE END EXPORTED_MACRO */

  /**
   * @}
   */

  /** @defgroup USBD_MEDIA_Exported_Variables USBD_MEDIA_Exported_Variables
   * @brief Public variables.
   * @{
   */

  /** MEDIA Interface callback. */
  extern USBD_DFU_MediaTypeDef USBD_DFU_fops_FS;

  /* USER CODE BEGIN EXPORTED_VARIABLES */

  /* USER CODE END EXPORTED_VARIABLES */

  /**
   * @}
   */

  /** @defgroup USBD_MEDIA_Exported_FunctionsPrototype USBD_MEDIA_Exported_FunctionsPrototype
   * @brief Public functions declaration.
   * @{
   */

  /* USER CODE BEGIN EXPORTED_FUNCTIONS */

  /* USER CODE END EXPORTED_FUNCTIONS */

  /**
   * @}
   */

  /**
   * @}
   */

  /**
   * @}
   */
#endif

#ifdef __cplusplus
}
#endif

#endif /* __USBD_DFU_IF_H__ */
