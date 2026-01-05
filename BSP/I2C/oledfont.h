/**
 * @file oledfont.h
 * @author 菜菜why（B站：菜菜whyy）
 * @brief OLED 字符集与字库定义头文件
 *
 * @attention
 * - 本文件定义了用于 OLED 显示的 ASCII 字符及汉字点阵数据。
 *  【汉字】取模方式：阴码、逆向、逐行式、C51格式
 * 【单色图标】
 *  1.取模参数：阴码、逆向、逐行式、C51格式
 *  2.数据格式：1位/像素 (黑白两色)
 */

#ifndef __OLEDFONT_H__
#define __OLEDFONT_H__

#include "config.h"

#ifdef OLED_I2C_ENABLE

/**
 * @brief 6x8 ASCII 字符集点阵
 * @note 包含标准 ASCII 可打印字符。
 *       每个字符宽度为 6 像素，高度为 8 像素（占用 1 页）。
 */
extern const unsigned char F6x8[][6];

/**
 * @brief 16x8 ASCII 字符集点阵
 * @note 包含标准 ASCII 可打印字符。
 *       每个字符宽度为 8 像素，高度为 16 像素（占用 2 页）。
 */
extern const unsigned char F8X16[];

/**
 * @brief 16x16 汉字点阵字库
 * @note 存储特定汉字的点阵数据。
 *       每个汉字占用 16x16 像素，数据量为 32 字节。
 */
extern const unsigned char Hzk[][32]; // filepath: e:\CODE_b\stm32ai\auto_stm32_test\Templates\I2C\oledfont.h

#endif

#endif
