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
extern "C" {
#endif

#include "config.h"

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
#include "SPI/lcd_fonts.h"
#include "SPI/lcd_image.h"
#include "SPI/lcd_spi.h"

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

#ifdef DSPEAKER_ENABLE
#include "I2S/audio_player.h"
#include "I2S/mp3_player.h"
#include "I2S/dspeaker.h"
#endif /* audio_player需要开启fatfs！ */

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

#ifdef JPEG_ENABLE
#include "JPEG/jpeg_app.h"
#include "JPEG/jpeg_code.h"
#endif /* jpeg_app需要开启fatfs,sdram！ */

#ifdef USB_DEVICE_ENABLE
#include "USB/usb_device.h"
#endif

#ifdef USB_HOST_ENABLE
#include "USB/usb_host.h"
#endif

#ifdef OV5640_ENABLE
#include "DCMI/dcmi_ov5640.h"
#include "DCMI/ov5640_lcd_rgb.h"
#include "DCMI/ov5640_lcd_spi.h"
#endif

#ifdef RTC_ENABLE
#include "RTC/rtc_app.h"
#endif

#ifdef UART_WIFI_ENABLE
#include "UART/uart_wifi.h"
#include "UART/uart_wifi_aichat.h"
#include "UART/uart_wifi_http.h"
#endif

#ifdef UART_DEV_ENABLE
#include "UART/uart_dev.h"
#endif

#ifdef OLED_I2C_ENABLE
#include "I2C/oled.h"
#endif

#ifdef MPU6050_I2C_ENABLE
#include "I2C/mpu6050.h"
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
