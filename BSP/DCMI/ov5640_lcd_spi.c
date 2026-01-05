#include "ov5640_lcd_spi.h"
#include "lcd_spi.h"
#include <stdio.h>

#if defined(OV5640_ENABLE) && defined(LCD_SPI_ENABLE)

uint16_t spi_cam_buf[LCD_Width * LCD_Height] __attribute__((section(".RAM_D1"), aligned(32)));

void OV5640_SPI_Start(void)
{
    // 计算缓冲区大小 (字节) = 宽 * 高 * 2字节/像素 / 4 (32位宽DMA)
    uint32_t buffer_size = (LCD_Width * LCD_Height * 2) / 4;
    // 启动 DCMI DMA 连续传输
    OV5640_DMA_Transmit_Continuous(SPI_Camera_Buffer_Addr, buffer_size);
}

void OV5640_SPI_Task(void)
{

    // 检查一帧数据是否传输完成
    if (OV5640_FrameState)
    {

        OV5640_FrameState = 0;           // 清除标志位

        // 将摄像头缓冲区的数据直接搬运到屏幕
        LCD_CopyBuffer(0, 0, LCD_Width, LCD_Height, (uint16_t *)SPI_Camera_Buffer_Addr);
        // LCD_DisplayString(84, 240, "FPS:");
        // LCD_DisplayNumber(132, 240, OV5640_FPS, 2); // 显示帧率
    }

}
#endif
