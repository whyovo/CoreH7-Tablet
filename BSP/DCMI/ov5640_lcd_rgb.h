#ifndef __OV5640_LCD_RGB_H
#define __OV5640_LCD_RGB_H

#include "config.h"

#if defined(OV5640_ENABLE) && defined(LCD_RGB_ENABLE)

#include "dcmi_ov5640.h"

/* RGB 屏显存地址 (通常在 SDRAM) */
/* 直接将摄像头数据 DMA 到显存，实现 0 拷贝显示 */
#define RGB_Camera_Buffer SDRAM_BANK_ADDR

/**
 * @brief  启动 RGB 屏摄像头显示
 */
void OV5640_RGB_Start(void);

/**
 * @brief  截图当前摄像头画面变成jpg到SD卡
 * @param  filename: 保存的文件名地址（如："0:/capture.jpg"）,写NULL则运用默认配置
 */
void OV5640_RGB_Capture(const char *filename);

#endif

#endif
