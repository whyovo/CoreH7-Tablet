/**
 ******************************************************************************
 * @file    jpeg_app.h
 * @author  自定义
 * @brief   JPEG应用层头文件 - 显示和保存JPEG图片
 ******************************************************************************
 */

#ifndef __JPEG_APP_H
#define __JPEG_APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#if defined(JPEG_ENABLE) && defined(FATFS_ENABLE)
#include "jpeg_code.h"
/**
 * @brief  从SD卡读取JPEG文件并在屏幕上显示
 * @param  filename: 文件名（如："1.jpg"）
 * @param  x: 显示起始X坐标
 * @param  y: 显示起始Y坐标
 * @retval 0: 成功, -1: 失败
 * @note   示例: JPEG_App_DisplayFile("1.jpg", 0, 0);
 */
int JPEG_App_DisplayFile(const char *filename, uint16_t x, uint16_t y);

/**
 * @brief  创建纯色JPEG图片并保存到SD卡
 * @param  filename: 保存的文件名（如："3.jpg"）
 * @param  width: 图片宽度（像素）
 * @param  height: 图片高度（像素）
 * @param  color: 图片颜色（RGB565格式）
 * @param  quality: 编码质量
 * @retval 0: 成功, -1: 失败
 * @note   示例: JPEG_App_CreateSolidColorImage("3.jpg", 800, 480,
 * RGB565(255,0,0), JPEG_Quality_High);
 * @note   RGB565(R,G,B) 宏定义: ((R>>3)<<11) | ((G>>2)<<5) | (B>>3)
 */
int JPEG_App_CreateSolidColorImage(const char *filename, uint16_t width,
                                   uint16_t height, uint16_t color,
                                   JPEG_QualityTypeDef quality);

/**
 * @brief  截屏指定图层内容并保存到SD卡
 * @param  filepath: 文件路径 (例如 "0:/capture.jpg")
 *                   - 如果为 NULL，自动在根目录生成 "0:/screenshot_x.jpg"
 *                   - 如果指定路径，则直接保存为该文件名
 * @param  LayerIndex: 图层索引 (0: Layer1/背景层, 1: Layer2/前景层)
 * @retval 0:成功, 1:编码失败, 2:文件打开失败, 3:写入失败
 */
uint8_t JPEG_Save_Screenshot(const char *filepath, uint32_t LayerIndex);

#endif

#ifdef __cplusplus
}
#endif

#endif // __JPEG_APP_H
