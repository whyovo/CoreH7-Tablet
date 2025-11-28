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
 *选择输出方式（0=LCD，1=串口，可扩展）(只推荐用串口！！！)
 * 3. 串口模式下，需在下方配置串口参数（UART编号、TX/RX引脚）
 * 4. 使用 DEBUG_INFO(msg) 输出普通调试信息
 * 5. 使用 DEBUG_ERROR(msg) 输出错误信息（含文件/函数/行号，并进入死循环）
 *
 * 注意事项：
 * - DEBUG_ERROR 会导致程序停止运行，用于捕获严重错误
 * - 未定义 DEBUG_ENABLE 时，调试宏为空操作，不占用代码空间
 * - 使用串口模式，请确保在CubeMX中已配置对应的UART外设
 *
 ******************************************************************************
 */

#ifndef DEBUG_H
#define DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif
#include "init.h"
#include <stdio.h>
#include <string.h>

#ifdef DEBUG_ENABLE

/*******************************************************************************
 *                              调试输出配置
 ******************************************************************************/

#define DEBUG_OUTPUT_MODE                                                      \
  0 /*!< 调试输出模式：0=LCD，1=串口，2=串口重定向printf，3=预留扩展 */

/*******************************************************************************
 *                   串口调试配置
 ******************************************************************************/

#if (DEBUG_OUTPUT_MODE == 1 || DEBUG_OUTPUT_MODE == 2)
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
/* LCD输出模式 */
/**
 * @brief  初始化调试模块
 * @note   设置字体大小和颜色，清空调试区域
 * @retval None
 */
void Debug_Init(void);
#endif

/*******************************************************************************
 *                              调试信息函数
 ******************************************************************************/
#if (DEBUG_OUTPUT_MODE == 0 || DEBUG_OUTPUT_MODE == 1)
/**
 * @brief  输出调试信息（INFO级别）
 * @param  msg: 调试消息字符串
 * @retval None
 */
void Debug_Info(const char *msg);

/**
 * @brief  输出错误信息并停止运行（ERROR级别）
 * @param  msg: 错误消息字符串
 * @param  file: 出错文件名（使用 __FILE__ 宏）
 * @param  func: 出错函数名（使用 __FUNCTION__ 宏）
 * @param  line: 出错行号（使用 __LINE__ 宏）
 * @retval None（函数不会返回，进入死循环）
 */
void Debug_Error(const char *msg, const char *file, const char *func,
                 uint32_t line);

/**
 * @brief  DEBUG_INFO 宏：输出普通调试信息
 * @param  msg: 调试消息字符串
 * @note   仅在定义 DEBUG_ENABLE 时有效
 */
#define DEBUG_INFO(msg) Debug_Info(msg)

/**
 * @brief  DEBUG_ERROR 宏：输出错误信息并停止程序
 * @param  msg: 错误消息字符串
 * @note   仅在定义 DEBUG_ENABLE 时有效
 * @note   会自动记录文件名、函数名、行号，并进入死循环
 */
#define DEBUG_ERROR(msg) Debug_Error((msg), __FILE__, __FUNCTION__, __LINE__)
#elif (DEBUG_OUTPUT_MODE == 2)
void Debug_Info(const char *fmt, ...);

/* Debug_Error：文件/函数/行号自动由宏传入，错误仍会停机 */
void Debug_Error(const char *file, const char *func, uint32_t line,
                 const char *fmt, ...);

/* 可变参数宏转发 */
#define DEBUG_INFO(...) Debug_Info(__VA_ARGS__)
#define DEBUG_ERROR(...)                                                       \
  Debug_Error(__FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif // !DEBUG_H
