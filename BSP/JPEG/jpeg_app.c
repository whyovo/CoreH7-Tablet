/**
 ******************************************************************************
 * @file    jpeg_app.c
 * @author  自定义
 * @brief   JPEG应用层实现 - 显示和保存JPEG图片
 ******************************************************************************
 */

#include "jpeg_app.h"
#include "fatfs.h"
#include "jpeg_code.h"
#include "lcd_rgb.h"
#include <stdio.h>
#include <string.h>


#ifdef JPEG_ENABLE
/* RGB565颜色宏定义 */
#define RGB565(R, G, B) (((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3))

/*******************************************************************************
 *                      显示JPEG文件函数
 ******************************************************************************/

/**
 * @brief  从SD卡读取JPEG文件并在屏幕上显示
 * @param  filename: 文件名（如："1.jpg"）
 * @param  x: 显示起始X坐标
 * @param  y: 显示起始Y坐标
 * @retval 0: 成功, -1: 失败
 */
int JPEG_App_DisplayFile(const char *filename, uint16_t x, uint16_t y) {
  static FIL file;
  FRESULT res;
  UINT bytes_read;
  FSIZE_t file_size;

  // 打开文件
  res = f_open(&file, filename, FA_READ);
  if (res != FR_OK) {
    DEBUG_ERROR("文件打开失败");
    return -1;
  }

  // 获取文件大小
  file_size = f_size(&file);
  if (file_size == 0 ||
      file_size > 0x200000) {
    DEBUG_ERROR("文件大小无效");
    f_close(&file);
    return -1;
  }

  // 读取JPEG文件到缓冲区
  res = f_read(&file, (uint8_t *)File_BUFFER, file_size, &bytes_read);
  if (res != FR_OK || bytes_read != file_size) { // 检查读取字节数是否匹配
    DEBUG_ERROR("文件读取失败");
    f_close(&file);
    return -1;
  }

  // 关闭文件
  f_close(&file);

  DEBUG_INFO("JPEG文件读取成功");

  // 启动硬件JPEG解码
  JPEG_Decode_DMA(File_BUFFER, (uint32_t)file_size, JPEG_OUTPUT_DATA_BUFFER);

  // 等待解码完成
  while (JPEG_Decode_WaitingforEnd() != JPEG_OpComplete); 

  DEBUG_INFO("JPEG解码完成");

  // 通过DMA2D转换YCbCr为RGB并显示
  DMA2D_CopyBuffer(x, y, (uint32_t *)JPEG_OUTPUT_DATA_BUFFER,
                   (uint32_t *)LCD_FRAME_BUFFER);

  DEBUG_INFO("JPEG显示完成");

  return 0;
}

/*******************************************************************************
 *                      创建纯色JPEG图片函数
 ******************************************************************************/

/**
 * @brief  创建纯色JPEG图片并保存到SD卡
 * @param  filename: 保存的文件名
 * @param  width: 图片宽度
 * @param  height: 图片高度
 * @param  color: 图片颜色（RGB565格式）
 * @param  quality: 编码质量
 * @retval 0: 成功, -1: 失败
 */
int JPEG_App_CreateSolidColorImage(const char *filename, uint16_t width,
                                   uint16_t height, uint16_t color,
                                   JPEG_QualityTypeDef quality) {
  static FIL file;
  FRESULT res;
  UINT bytes_written;
  uint32_t total_pixels = (uint32_t)width * height;
  uint32_t i;
  uint16_t *pRGB565;

  DEBUG_INFO("开始创建纯色图");

  // 使用 JPEG_OUTPUT_DATA_BUFFER 存放 RGB 数据
  pRGB565 = (uint16_t *)JPEG_OUTPUT_DATA_BUFFER;

  // 生成纯色RGB565数据
  for (i = 0; i < total_pixels; i++) {
    pRGB565[i] = color;
  }

  DEBUG_INFO("RGB数据生成完成");

  // 使用新的编码函数，File_BUFFER作为YCbCr中间缓冲区
  if (JPEG_Encode_RGB565(width, height, 
                         JPEG_OUTPUT_DATA_BUFFER,    // RGB输入
                         File_BUFFER,                 // YCbCr中间缓冲
                         JPEG_ENCODE_OUTPUT_BUFFER,   // JPEG输出
                         quality, 
                         JPEG_Encode_420) != 0) {
    DEBUG_ERROR("JPEG编码失败");
    return -1;
  }

  // 获取编码后的数据大小
  uint32_t encoded_size = JPEG_GetEncodedSize();

  DEBUG_INFO("JPEG编码完成");

  if (encoded_size == 0) {
    DEBUG_ERROR("编码数据大小为0");
    return -1;
  }

  // 打开文件用于写入
  res = f_open(&file, filename, FA_CREATE_ALWAYS | FA_WRITE);
  if (res != FR_OK) {
    DEBUG_ERROR("文件创建失败");
    return -1;
  }

  // 写入JPEG数据到文件
  res = f_write(&file, (uint8_t *)JPEG_ENCODE_OUTPUT_BUFFER, encoded_size,
                &bytes_written);
  if (res != FR_OK || bytes_written != encoded_size) {
    DEBUG_ERROR("文件写入失败");
    f_close(&file);
    return -1;
  }

  // 关闭文件
  f_close(&file);

  DEBUG_INFO("文件保存成功");

  return 0;
}

#endif
