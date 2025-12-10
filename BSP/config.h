/**
 ******************************************************************************
 * @file    config.h
 * @author  菜菜why（B站：菜菜whyy）
 * @brief   项目配置头文件，统一管理所有外设模块的使能开关
 *          通过定义/注释宏来启用/禁用对应模块，减少代码体积
 ******************************************************************************
 * @attention
 *
 * 使用说明：
 * 1. 定义宏即启用该模块，注释宏即禁用该模块
 * 2. 某些模块有依赖关系（见下方注释），需按顺序启用
 * 3. 此文件应在 init.h 之前被 main.c 包含
 *
 ******************************************************************************
 */

#ifndef CONFIG_H
#define CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* 包含 STM32 HAL 库头文件，替换为自己的 */
#include "stm32h7xx.h"
#include "stm32h7xx_hal.h"
/*******************************************************************************
 *                              外设使能开关
 ******************************************************************************/
/**
 * @brief 在此处定义需要启用的外设模块
 * @note  注释掉对应宏即可禁用该模块，减少代码体积
 */
#define DEBUG_ENABLE /*!< 调试输出使能 */
#define LED_ENABLE   /*!< LED驱动使能 */
// #define KEY_ENABLE            /*!< 按键驱动使能 */
// #define BUZZER_ENABLE         /*!< 蜂鸣器驱动使能 */
// #define DIGITAL_SENSOR_ENABLE /*!< 数字传感器驱动使能 */
// #define UI_ENCODER_ENABLE     /*!< UI编码器驱动使能 */
#define LCD_SPI_ENABLE /*!< LCD SPI驱动使能 */
#define LCD_RGB_ENABLE    /*!< LCD RGB驱动使能 */
#define LCD_RGB_TOUCH_ENABLE /*!< LCD RGB触摸驱动使能,触摸屏使用，必须先定义LCD_RGB_ENABLE*/
// #define QSPI_FLASH_ENABLE /*!< QSPI Flash驱动使能 */
#define FLASH_FONT_ENABLE /*!< Flash字体驱动使能,必须优先定义QSPI_FLASH_ENABLE  */
// #define DMIC_ENABLE       /*!< INMP441数字麦克风驱动使能 */
#define DSPEAKER_ENABLE   /*!< MAX98357A数字扬声器驱动使能 */
// #define OLED_HARD_ENABLE  /*!< OLED硬件I2C驱动使能 */
// #define OLED_SOFT_ENABLE  /*!< OLED软件I2C驱动使能 */
#define SDMMC_ENABLE /*!< SDMMC驱动使能 */
#define FATFS_ENABLE /*!< SD卡的FATFS文件系统使能，必须优先定义SDMMC_ENABLE*/ 
#define SDRAM_ENABLE      /*!< SDRAM驱动使能 */ 
#define JPEG_ENABLE     /*!<JPEG驱动使能 */
// #define USB_DEVICE_ENABLE       /*!< USB DEVICE驱动使能 */
// #define USB_HOST_ENABLE         /*!< USB HOST驱动使能 */
/*******************************************************************************
 *                          模块依赖关系检查
 ******************************************************************************/

/* FATFS 依赖于 SDMMC */
#ifdef FATFS_ENABLE
#ifndef SDMMC_ENABLE
#define SDMMC_ENABLE
#warning "FATFS_ENABLE detected, auto-enabling SDMMC_ENABLE"
#endif
#endif

/* DSPEAKER 依赖于 FATFS */
#ifdef DSPEAKER_ENABLE
#ifndef FATFS_ENABLE
#error "DSPEAKER_ENABLE requires FATFS_ENABLE"
#endif
#endif

/* JPEG 依赖于 FATFS 和 SDRAM */
#ifdef JPEG_ENABLE
#ifndef FATFS_ENABLE
#error "JPEG_ENABLE requires FATFS_ENABLE"
#endif
#ifndef SDRAM_ENABLE
#error "JPEG_ENABLE requires SDRAM_ENABLE"
#endif
#endif

/* LCD_RGB_TOUCH 依赖于 LCD_RGB */
#ifdef LCD_RGB_TOUCH_ENABLE
#ifndef LCD_RGB_ENABLE
#error "LCD_RGB_TOUCH_ENABLE requires LCD_RGB_ENABLE"
#endif
#endif

#ifdef DEBUG_ENABLE
#include "DEBUG/debug.h"
#else /* DEBUG_ENABLE 未定义 */
#define Debug_Init() ((void)0)
#define DEBUG_INFO(msg) ((void)0)
#define DEBUG_ERROR(msg) ((void)0)
#endif /* DEBUG_ENABLE */
/*******************************************************************************
 *                              平台抽象层宏
 ******************************************************************************/

#ifndef Delay_us
/* 尝试用 HAL 定义的 SystemCoreClock，失败则给默认值 72000000 */
#if defined(SystemCoreClock) && (SystemCoreClock > 0)
#define __CORE_CLK SystemCoreClock
#else
#define __CORE_CLK 72000000UL /* 72 MHz 默认 */
#endif
/* 1μs 约需要空操作次数 = 主频 / 4 / 1000000
 * 除以4是粗略估算每条__NOP()指令的时钟周期数
 */
#define __NOP_US ((__CORE_CLK / 1000000UL) / 4UL)

#define Delay_us(us)                                                           \
  do {                                                                         \
    volatile uint32_t cnt = (us) * __NOP_US;                                   \
    while (cnt--) {                                                            \
      __NOP();                                                                 \
    }                                                                          \
  } while (0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_H */
