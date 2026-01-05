#ifndef __OV5640_LCD_SPI_H
#define __OV5640_LCD_SPI_H

#include "config.h"

#if defined(OV5640_ENABLE) && defined(LCD_SPI_ENABLE)

#include "dcmi_ov5640.h"

/* 定义 SPI 摄像头缓冲区地址 */

extern uint16_t spi_cam_buf[];
#define SPI_Camera_Buffer_Addr ((uint32_t)spi_cam_buf)


/**
 * @brief  启动 SPI 屏摄像头采集
 */
void OV5640_SPI_Start(void);

/**
 * @brief  SPI 摄像头刷新任务 (需在主循环调用)
 */
void OV5640_SPI_Task(void);

#endif

#endif
