/**
 ******************************************************************************
 * @file    debug.h
 * @author  菜菜why（B站：菜菜whyy）
 * @brief   调试输出头文件，提供INFO和ERROR级别的调试宏
 ******************************************************************************
 * @attention
 *
 * 使用方法：
 * 1. 在 init.h 中定义 DEBUG_ENABLE 宏以启用调试功能
 * 2. 设置 DEBUG_OUTPUT_MODE
 *    0=LCD, 1=串口, 2=USB CDC
 * 3. 串口模式下，需在下方配置串口参数（UART编号）
 * 4. 使用 DEBUG_INFO(fmt, ...) 输出普通调试信息，支持 printf 格式
 * 5. 使用 DEBUG_ERROR(fmt, ...) 输出错误信息（含文件/函数/行号，并进入死循环）
 *
 ******************************************************************************
 */

#ifndef DEBUG_H
#define DEBUG_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <string.h>
#include <stdarg.h> // 引入可变参数支持

#include "config.h"

#ifdef DEBUG_ENABLE

  /*******************************************************************************
   *                              调试输出配置
   ******************************************************************************/

#define DEBUG_OUTPUT_MODE 0 /*!< 调试输出模式：0=LCD，1=串口，2=USB CDC (原串口重定向已移除) */

  /*******************************************************************************
   *                   串口调试配置 (仅模式1有效)
   ******************************************************************************/

#if (DEBUG_OUTPUT_MODE == 1)
#define DEBUG_UART huart1
  /* 声明串口句柄 */
  extern UART_HandleTypeDef huart1;
#endif

/*******************************************************************************
 *                              初始化函数
 ******************************************************************************/
#if (DEBUG_OUTPUT_MODE == 0)
#include "lcd_fonts.h"
#include "lcd_image.h"
#include "lcd_spi.h"
  /* LCD输出模式初始化 */
  void Debug_Init(void);
#endif

  /*******************************************************************************
   *                              调试信息函数
   ******************************************************************************/

  /**
   * @brief  输出调试信息（INFO级别），支持 printf 格式
   * @param  fmt: 格式化字符串
   * @param  ...: 可变参数
   */
  void Debug_Info(const char *fmt, ...);

  /**
   * @brief  输出错误信息并停止运行（ERROR级别），支持 printf 格式
   *         通常由 DEBUG_ERROR 宏调用以自动传入文件/行号
   */
  void Debug_Error(const char *file, const char *func, uint32_t line,
                   const char *fmt, ...);

/**
 * @brief  DEBUG_INFO 宏：输出普通调试信息
 * @note   仅在定义 DEBUG_ENABLE 时有效
 */
#define DEBUG_INFO(...) Debug_Info(__VA_ARGS__)

/**
 * @brief  DEBUG_ERROR 宏：输出错误信息并停止程序
 * @note   仅在定义 DEBUG_ENABLE 时有效
 * @note   会自动记录文件名、函数名、行号，并进入死循环
 */
#define DEBUG_ERROR(...) \
  Debug_Error(__FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

#endif /* DEBUG_ENABLE */

#ifdef __cplusplus
}
#endif

#endif // !DEBUG_H
