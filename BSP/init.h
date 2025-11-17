/**
 ******************************************************************************
 * @file    init.h
 * @author  菜菜why（B站：菜菜whyy）
 * @brief   初始化头文件，统一管理外设初始化与主循环任务
 *          提供平台抽象层宏定义，便于移植到不同HAL库
 ******************************************************************************
 * @attention
 *
 * 使用说明：
 * 1. 在此文件中定义需要启用的外设模块（LED_ENABLE / KEY_ENABLE 等）
 * 2. 平台抽象宏（GPIO_WritePin等）默认映射到STM32 HAL库，可重定义以适配其他平台
 * 3. init_all() 完成所有外设初始化，main_while() 在主循环中周期调用
 *
 ******************************************************************************
 */

#ifndef INIT_H
#define INIT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "stm32h7xx_hal.h"

/*******************************************************************************
 *                              外设使能开关
 ******************************************************************************/
/**
 * @brief 在此处定义需要启用的外设模块
 * @note  注释掉对应宏即可禁用该模块，减少代码体积
 */
#define DEBUG_ENABLE /*!< 调试输出使能 */
#define LED_ENABLE /*!< LED驱动使能 */
// #define KEY_ENABLE            /*!< 按键驱动使能 */
// #define BUZZER_ENABLE         /*!< 蜂鸣器驱动使能 */
// #define DIGITAL_SENSOR_ENABLE /*!< 数字传感器驱动使能 */
// #define UI_ENCODER_ENABLE     /*!< UI编码器驱动使能 */
#define LCD_SPI_ENABLE    /*!< LCD SPI驱动使能 */
#define LCD_RGB_ENABLE    /*!< LCD RGB驱动使能 */
#define LCD_RGB_TOUCH_ENABLE /*!< LCD RGB触摸驱动使能,触摸屏使用，必须先定义 LCD_RGB_ENABLE*/
//#define QSPI_FLASH_ENABLE /*!< QSPI Flash驱动使能 */
#define FLASH_FONT_ENABLE /*!< Flash字体驱动使能,必须优先定义QSPI_FLASH_ENABLE */
// #define DMIC_ENABLE       /*!< INMP441数字麦克风驱动使能 */
// #define OLED_HARD_ENABLE  /*!< OLED硬件I2C驱动使能 */
// #define OLED_SOFT_ENABLE  /*!< OLED软件I2C驱动使能 */
#define SDMMC_ENABLE      /*!< SDMMC驱动使能 */
#define FATFS_ENABLE      /*!< SD卡的FATFS文件系统使能，必须优先定义SDMMC_ENABLE */
#define SDRAM_ENABLE      /*!< SDRAM驱动使能 */
/*******************************************************************************
 *                              头文件包含（自动包含）
 ******************************************************************************/
/**
 * @note 根据上面的使能开关自动包含对应的驱动头文件，无需手动修改
 */
#ifdef LED_ENABLE
#include "GPIO/led.h"
#endif /* LED_ENABLE */

#ifdef KEY_ENABLE
#include "GPIO/key.h"
#endif /* KEY_ENABLE */

#ifdef BUZZER_ENABLE
#include "GPIO/active_buzzer.h"
#endif /* BUZZER_ENABLE */

#ifdef DIGITAL_SENSOR_ENABLE
#include "GPIO/digital_output_sensor.h"
#endif /* DIGITAL_SENSOR_ENABLE */

#ifdef UI_ENCODER_ENABLE
#include "GPIO/ui_encoder.h"
#endif /* UI_ENCODER_ENABLE */

#ifdef LCD_SPI_ENABLE
#include "SPI/lcd_spi.h"
#include "SPI/lcd_fonts.h"
#include "SPI/lcd_image.h"
#endif

#ifdef LCD_RGB_ENABLE
#include "LTDC/lcd_rgb.h"
#include "SPI/lcd_fonts.h"
#include "SPI/lcd_image.h"
#endif

#ifdef LCD_RGB_TOUCH_ENABLE
#include "LTDC/lcd_touch.h"
#include "LTDC/touch_iic.h"
#endif

#ifdef FLASH_FONT_ENABLE
#include "QSPI/flash_font.h"
#endif

#ifdef QSPI_FLASH_ENABLE
#include "QSPI/qspi_flash.h"
#endif

#ifdef DMIC_ENABLE
#include "I2S/dmic.h"
#endif /* DMIC_ENABLE */

#ifdef OLED_HARD_ENABLE
#include "I2C/oled_hard.h"
#endif

#ifdef OLED_SOFT_ENABLE
#include "I2C/oled_soft.h"
#endif

#ifdef FATFS_ENABLE
#ifndef SDMMC_ENABLE
#define SDMMC_ENABLE
#endif
#include "SDIO/fatfs.h"
#endif

#ifdef SDMMC_ENABLE
#include "SDIO/sdmmc_sd.h"
#endif

#ifdef SDRAM_ENABLE
#include "FMC/sdram.h"
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

#ifndef GPIO_WritePin
#define GPIO_WritePin(port, pin, state) HAL_GPIO_WritePin((port), (pin), (state))
#endif

#ifndef GPIO_TogglePin
#define GPIO_TogglePin(port, pin) HAL_GPIO_TogglePin((port), (pin))
#endif

#ifndef GPIO_ReadPin
#define GPIO_ReadPin(port, pin) HAL_GPIO_ReadPin((port), (pin))
#endif

#ifndef GetTick
#define GetTick() HAL_GetTick()
#endif

#ifndef Delay_ms
#define Delay_ms(ms) HAL_Delay(ms)
#endif

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

#define Delay_us(us)                             \
    do                                           \
    {                                            \
        volatile uint32_t cnt = (us) * __NOP_US; \
        while (cnt--)                            \
        {                                        \
            __NOP();                             \
        }                                        \
    } while (0)
#endif

    /*******************************************************************************
     *                              导出函数
     ******************************************************************************/

    /**
     * @brief  初始化所有启用的外设
     * @note   应在主函数系统时钟配置后、进入主循环前调用
     * @note   会根据 LED_ENABLE / KEY_ENABLE 等宏有条件编译
     * @retval None
     */
    void init_all(void);

    /**
     * @brief  主循环周期任务
     * @note   在 while(1) 中周期调用，用于驱动非阻塞任务（如按键扫描）
     * @retval None
     */
    void main_while(void);

#ifdef __cplusplus
}
#endif

#endif // !INIT_H
