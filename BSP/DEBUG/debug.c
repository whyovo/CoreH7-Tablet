/**
 ******************************************************************************
 * @file    debug.c
 * @author  菜菜why(B站:菜菜whyy)
 * @brief   调试输出实现文件
 ******************************************************************************
 * @attention
 *
 * 本文件实现:
 * - DEBUG_INFO: 输出普通调试信息到LCD/串口(只推荐用串口！！！)
 * - DEBUG_ERROR: 输出错误详情(文件/函数/行号)并进入死循环
 *
 * 注意事项:
 * - 通过 DEBUG_OUTPUT_MODE 宏选择输出方式(0=LCD,1=串口,2/3=扩展)
 * - DEBUG_ERROR 会导致程序停止,用于错误的快速定位
 *
 ******************************************************************************
 */

#include "debug.h"

#ifdef DEBUG_ENABLE

#if (DEBUG_OUTPUT_MODE == 0)
/*****************************************************************************
 *                           LCD 输出模式
 *****************************************************************************/

/* 调试配置参数 */
#define DEBUG_FONT_SIZE 16                                      /*!< 调试信息字体大小 */
#define DEBUG_LINE_HEIGHT 18                                    /*!< 每行高度(像素) */
#define DEBUG_MAX_LINES (LCD_Height / DEBUG_LINE_HEIGHT)        /*!< 最大显示行数:320/18≈17 */
#define DEBUG_CHAR_WIDTH 8                                      /*!< ASCII字符宽度(16号字体约8像素) */
#define DEBUG_CHINESE_WIDTH 16                                  /*!< 中文字符宽度(16号字体) */
#define DEBUG_MAX_CHARS_PER_LINE (LCD_Width / DEBUG_CHAR_WIDTH) /*!< 每行最大ASCII字符数:240/8=30 */

/* 调试输出状态管理 */
static uint16_t debug_current_line = 0; /*!< 当前输出行号(0~DEBUG_MAX_LINES-1) */
static uint8_t debug_initialized = 0;   /*!< 调试模块初始化标志 */
static uint8_t debug_in_progress = 0;   /*!< 递归保护标志 */
/* 日志缓冲区,用于滚动显示 */
static char debug_log_buffer[DEBUG_MAX_LINES][DEBUG_MAX_CHARS_PER_LINE + 1]; /*!< 保存每行日志内容 */
static uint16_t debug_log_count = 0;                                         /*!< 当前日志条数 */

/**
 * @brief  初始化调试模块
 * @note   设置字体、颜色,清空调试区域
 * @retval None
 */
void Debug_Init(void)
{
    LCD_SetTextFont(DEBUG_FONT_SIZE); // 设置字体大小为16
    LCD_SetBackColor(LCD_BLACK);      // 黑色背景
    LCD_SetColor(LCD_WHITE);          // 白色文字
    LCD_Clear();                      // 清屏

    debug_current_line = 0;
    debug_log_count = 0;
    debug_initialized = 1;

    /* 清空日志缓冲区 */
    for (int i = 0; i < DEBUG_MAX_LINES; i++)
    {
        debug_log_buffer[i][0] = '\0';
    }
}

/**
 * @brief  刷新屏幕显示所有日志
 * @note   重新绘制所有缓冲区中的日志
 * @retval None
 */
static void Debug_RefreshScreen(void)
{
    LCD_Clear(); // 清屏

    /* 重新显示所有日志 */
    for (uint16_t i = 0; i < debug_log_count && i < DEBUG_MAX_LINES; i++)
    {
        uint16_t y = i * DEBUG_LINE_HEIGHT;
        LCD_DisplayText(0, y, debug_log_buffer[i]);
    }
}

/**
 * @brief  滚动调试日志(整体上移一行)
 * @note   当日志写满屏幕时自动调用
 * @retval None
 */
static void Debug_Scroll(void)
{
    /* 将所有日志向上移动一行(丢弃第一行) */
    for (uint16_t i = 0; i < DEBUG_MAX_LINES - 1; i++)
    {
        strncpy(debug_log_buffer[i], debug_log_buffer[i + 1], sizeof(debug_log_buffer[i]) - 1);
        debug_log_buffer[i][sizeof(debug_log_buffer[i]) - 1] = '\0';
    }

    /* 清空最后一行 */
    debug_log_buffer[DEBUG_MAX_LINES - 1][0] = '\0';

    /* 更新当前行号和日志数 */
    if (debug_log_count > 0)
    {
        debug_log_count--;
    }
    if (debug_current_line > 0)
    {
        debug_current_line--;
    }

    /* 刷新屏幕显示 */
    Debug_RefreshScreen();
}

/**
 * @brief  计算字符串显示宽度(考虑中英文混合)
 * @param  str: 字符串
 * @param  len: 字符串长度
 * @retval 像素宽度
 */
static uint16_t Debug_GetStringWidth(const char *str, uint16_t len)
{
    uint16_t width = 0;
    for (uint16_t i = 0; i < len; i++)
    {
        if ((uint8_t)str[i] >= 0x80) // 中文字符(多字节)
        {
            width += DEBUG_CHINESE_WIDTH;
            i++; // 跳过下一个字节
            if (i < len && (uint8_t)str[i] >= 0x80)
                i++; // UTF-8可能是3字节
        }
        else // ASCII字符
        {
            width += DEBUG_CHAR_WIDTH;
        }
    }
    return width;
}

/**
 * @brief  添加一行日志到缓冲区并显示
 * @param  line_text: 日志内容
 * @retval None
 */
static void Debug_AddLine(const char *line_text)
{
    /* 检查是否需要滚动 */
    if (debug_current_line >= DEBUG_MAX_LINES)
    {
        Debug_Scroll();
    }

    /* 保存到缓冲区 */
    strncpy(debug_log_buffer[debug_current_line], line_text, sizeof(debug_log_buffer[debug_current_line]) - 1);
    debug_log_buffer[debug_current_line][sizeof(debug_log_buffer[debug_current_line]) - 1] = '\0';

    /* 计算当前行的Y坐标并显示 */
    uint16_t y = debug_current_line * DEBUG_LINE_HEIGHT;
    LCD_DisplayText(0, y, debug_log_buffer[debug_current_line]);

    /* 更新行号和日志计数 */
    debug_current_line++;
    if (debug_log_count < DEBUG_MAX_LINES)
    {
        debug_log_count++;
    }
}

/**
 * @brief  输出调试信息(INFO级别)
 * @param  msg: 调试消息字符串
 * @note   通过LCD输出格式化的调试信息,支持自动滚动和自动换行
 * @retval None
 */
void Debug_Info(const char *msg)
{
    if (msg == NULL)
        return;

    /* 递归保护：如果正在调试输出中，直接返回 */
    if (debug_in_progress)
        return;

    debug_in_progress = 1; /* 设置保护标志 */

    /* 首次使用时自动初始化 */
    if (!debug_initialized)
    {
        Debug_Init();
    }

    /* 准备时间戳(显示毫秒) */
    char timestamp[16];
    snprintf(timestamp, sizeof(timestamp), "[%lums]", HAL_GetTick());

    /* 计算时间戳宽度 */
    uint16_t timestamp_width = Debug_GetStringWidth(timestamp, strlen(timestamp));

    /* 处理消息,支持自动换行 */
    const char *p = msg;
    char line_buf[DEBUG_MAX_CHARS_PER_LINE + 1];

    while (*p != '\0')
    {
        uint16_t line_width = 0;
        uint16_t char_count = 0;

        /* 第一行需要加时间戳 */
        if (p == msg)
        {
            strcpy(line_buf, timestamp);
            char_count = strlen(timestamp);
            line_width = timestamp_width;
        }
        else
        {
            line_buf[0] = '\0';
            char_count = 0;
            line_width = 0;
        }

        /* 填充当前行,直到超出屏幕宽度 */
        while (*p != '\0')
        {
            uint16_t char_width;
            uint16_t char_bytes = 1;

            /* 判断字符类型和宽度 */
            if ((uint8_t)*p >= 0x80) // 中文或多字节字符
            {
                char_width = DEBUG_CHINESE_WIDTH;
                // 计算UTF-8字节数
                if (((uint8_t)*p & 0xE0) == 0xC0)
                    char_bytes = 2;
                else if (((uint8_t)*p & 0xF0) == 0xE0)
                    char_bytes = 3;
                else if (((uint8_t)*p & 0xF8) == 0xF0)
                    char_bytes = 4;
            }
            else // ASCII字符
            {
                char_width = DEBUG_CHAR_WIDTH;
            }

            /* 检查是否会超出屏幕宽度 */
            if (line_width + char_width > LCD_Width)
            {
                break; // 当前行已满,需要换行
            }

            /* 添加字符到行缓冲 */
            for (uint16_t i = 0; i < char_bytes && *p != '\0'; i++)
            {
                if (char_count < DEBUG_MAX_CHARS_PER_LINE)
                {
                    line_buf[char_count++] = *p++;
                }
                else
                {
                    break;
                }
            }

            line_width += char_width;
        }

        /* 结束当前行 */
        line_buf[char_count] = '\0';

        /* 添加到显示缓冲区 */
        Debug_AddLine(line_buf);
    }

    debug_in_progress = 0; /* 清除保护标志 */
}

/**
 * @brief  输出错误信息并停止运行(ERROR级别)
 * @param  msg: 错误消息字符串
 * @param  file: 出错文件名
 * @param  func: 出错函数名
 * @param  line: 出错行号
 * @note   输出完整的错误上下文后进入死循环,用于捕获严重错误
 * @retval None(函数不会返回)
 */
void Debug_Error(const char *msg, const char *file, const char *func, uint32_t line)
{
    debug_in_progress = 1; /* 设置保护标志，防止后续任何DEBUG调用 */

    uint16_t y = 0;

    /* 清屏并配置错误显示样式 */
    LCD_SetBackColor(LCD_RED);        // 红色背景
    LCD_SetColor(LCD_WHITE);          // 白色文字
    LCD_SetTextFont(DEBUG_FONT_SIZE); // 使用相同字体大小
    LCD_Clear();                      // 用红色背景重新清屏

    /* 准备时间戳 */
    char timestamp[16];
    snprintf(timestamp, sizeof(timestamp), "[%lums]", HAL_GetTick());

    /* 输出错误标题(带时间戳) */
    char title_buf[64];
    snprintf(title_buf, sizeof(title_buf), "%s ERROR", timestamp);
    LCD_DisplayText(0, y, title_buf);
    y += DEBUG_LINE_HEIGHT;

    LCD_DisplayText(0, y, "===================");
    y += DEBUG_LINE_HEIGHT;

    /* 输出错误消息(支持自动换行) */
    if (msg != NULL)
    {
        /* 添加标签 */
        LCD_DisplayText(0, y, "Msg:");
        y += DEBUG_LINE_HEIGHT;

        /* 自动换行显示消息 */
        const char *p = msg;
        char line_buf[DEBUG_MAX_CHARS_PER_LINE + 1];

        while (*p != '\0' && y < LCD_Height - DEBUG_LINE_HEIGHT)
        {
            uint16_t line_width = 0;
            uint16_t char_count = 0;

            /* 填充当前行,直到超出屏幕宽度 */
            while (*p != '\0')
            {
                uint16_t char_width;
                uint16_t char_bytes = 1;

                /* 判断字符类型和宽度 */
                if ((uint8_t)*p >= 0x80) // 中文或多字节字符
                {
                    char_width = DEBUG_CHINESE_WIDTH;
                    // 计算UTF-8字节数
                    if (((uint8_t)*p & 0xE0) == 0xC0)
                        char_bytes = 2;
                    else if (((uint8_t)*p & 0xF0) == 0xE0)
                        char_bytes = 3;
                    else if (((uint8_t)*p & 0xF8) == 0xF0)
                        char_bytes = 4;
                }
                else // ASCII字符
                {
                    char_width = DEBUG_CHAR_WIDTH;
                }

                /* 检查是否会超出屏幕宽度 */
                if (line_width + char_width > LCD_Width)
                {
                    break; // 当前行已满,需要换行
                }

                /* 添加字符到行缓冲 */
                for (uint16_t i = 0; i < char_bytes && *p != '\0'; i++)
                {
                    if (char_count < DEBUG_MAX_CHARS_PER_LINE)
                    {
                        line_buf[char_count++] = *p++;
                    }
                    else
                    {
                        break;
                    }
                }

                line_width += char_width;
            }

            /* 结束当前行 */
            line_buf[char_count] = '\0';

            /* 显示当前行 */
            LCD_DisplayText(0, y, line_buf);
            y += DEBUG_LINE_HEIGHT;
        }
    }

    /* 输出文件名(提取文件名部分,去除路径) */
    if (file != NULL && y < LCD_Height - DEBUG_LINE_HEIGHT)
    {
        const char *filename = file;
        const char *p = file;
        while (*p)
        {
            if (*p == '\\' || *p == '/')
                filename = p + 1;
            p++;
        }

        char file_buf[64];
        snprintf(file_buf, sizeof(file_buf), "File:%s", filename);
        LCD_DisplayText(0, y, file_buf);
        y += DEBUG_LINE_HEIGHT;
    }

    /* 输出函数名 */
    if (func != NULL && y < LCD_Height - DEBUG_LINE_HEIGHT)
    {
        char func_buf[64];
        snprintf(func_buf, sizeof(func_buf), "Func:%s", func);
        LCD_DisplayText(0, y, func_buf);
        y += DEBUG_LINE_HEIGHT;
    }

    /* 输出行号 */
    if (y < LCD_Height - DEBUG_LINE_HEIGHT)
    {
        char line_buf[64];
        snprintf(line_buf, sizeof(line_buf), "Line:%lu", line);
        LCD_DisplayText(0, y, line_buf);
        y += DEBUG_LINE_HEIGHT;
    }

    if (y < LCD_Height - DEBUG_LINE_HEIGHT)
    {
        LCD_DisplayText(0, y, "===================");
        y += DEBUG_LINE_HEIGHT;
    }

    if (y < LCD_Height - DEBUG_LINE_HEIGHT)
    {
        LCD_DisplayText(0, y, "System halted.");
    }

    /* 进入死循环,防止程序继续执行 */
    while (1)
    {
#ifdef LED_ENABLE
			  #include "led.h"
        LED_Toggle_All();
        Delay_ms(200);
#endif
    }
}

#elif (DEBUG_OUTPUT_MODE == 1)
/*****************************************************************************
 *                           串口输出模式
 *****************************************************************************/

/**
 * @brief  输出调试信息(INFO级别)
 * @param  msg: 调试消息字符串
 * @note   通过串口输出格式化的调试信息
 * @retval None
 */
void Debug_Info(const char *msg)
{
    if (msg == NULL)
        return;

    char debug_buf[128];
    int len = snprintf(debug_buf, sizeof(debug_buf), "[%lums][INFO] %s\r\n", HAL_GetTick(), msg);
    if (len > 0)
    {
        HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)debug_buf, len, HAL_MAX_DELAY);
    }
}

/**
 * @brief  输出错误信息并停止运行(ERROR级别)
 * @param  msg: 错误消息字符串
 * @param  file: 出错文件名
 * @param  func: 出错函数名
 * @param  line: 出错行号
 * @note   输出完整的错误上下文后进入死循环
 * @retval None(函数不会返回)
 */
void Debug_Error(const char *msg, const char *file, const char *func, uint32_t line)
{
    char error_buf[128];
    int len;

    /* 准备时间戳 */
    char timestamp[16];
    snprintf(timestamp, sizeof(timestamp), "[%lums]", HAL_GetTick());

    /* 输出错误标题(带时间戳) */
    len = snprintf(error_buf, sizeof(error_buf), "\r\n%s ERROR\r\n", timestamp);
    if (len > 0)
        HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)error_buf, len, HAL_MAX_DELAY);

    /* 输出分隔线 */
    const char *separator = "===================\r\n";
    HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)separator, strlen(separator), HAL_MAX_DELAY);

    /* 输出错误消息 */
    if (msg != NULL)
    {
        /* 输出标签 */
        const char *msg_label = "Msg:\r\n";
        HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)msg_label, strlen(msg_label), HAL_MAX_DELAY);

        /* 输出消息内容 */
        len = snprintf(error_buf, sizeof(error_buf), "%s\r\n", msg);
        if (len > 0)
            HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)error_buf, len, HAL_MAX_DELAY);
    }

    /* 输出文件名(提取文件名部分,去除路径) */
    if (file != NULL)
    {
        const char *filename = file;
        const char *p = file;
        while (*p)
        {
            if (*p == '\\' || *p == '/')
                filename = p + 1;
            p++;
        }

        len = snprintf(error_buf, sizeof(error_buf), "File:%s\r\n", filename);
        if (len > 0)
            HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)error_buf, len, HAL_MAX_DELAY);
    }

    /* 输出函数名 */
    if (func != NULL)
    {
        len = snprintf(error_buf, sizeof(error_buf), "Func:%s\r\n", func);
        if (len > 0)
            HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)error_buf, len, HAL_MAX_DELAY);
    }

    /* 输出行号 */
    len = snprintf(error_buf, sizeof(error_buf), "Line:%lu\r\n", line);
    if (len > 0)
        HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)error_buf, len, HAL_MAX_DELAY);

    /* 输出分隔线 */
    HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)separator, strlen(separator), HAL_MAX_DELAY);

    /* 输出结束提示 */
    const char *halt_msg = "System halted.\r\n";
    HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)halt_msg, strlen(halt_msg), HAL_MAX_DELAY);

    /* 进入死循环,防止程序继续执行 */
    while (1)
    {
#ifdef LED_ENABLE
        LED_Toggle_All();
        Delay_ms(200);
#endif
    }
}

#elif (DEBUG_OUTPUT_MODE == 2)
/*****************************************************************************
 *                           printf 重定向模式（MODE 2）
 * 说明：Debug_Info/Debug_Error 使用 printf 输出，支持 printf 风格参数。
 *****************************************************************************/
#include <stdarg.h>

void Debug_Info(const char *fmt, ...)
{
    if (fmt == NULL)
        return;

    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    /* 带时间戳的 INFO 输出 */
    printf("[%lums][INFO] %s\r\n", HAL_GetTick(), buf);
}

void Debug_Error(const char *file, const char *func, uint32_t line, const char *fmt, ...)
{
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    /* 输出错误头与时间戳 */
    printf("\r\n[%lums] ERROR\r\n", HAL_GetTick());
    printf("===================\r\n");

    /* 输出消息体 */
    if (buf[0] != '\0')
    {
        printf("Msg:\r\n");
        printf("%s\r\n", buf);
    }

    /* 提取并输出文件名（去掉路径） */
    if (file != NULL)
    {
        const char *filename = file;
        const char *p = file;
        while (*p)
        {
            if (*p == '\\' || *p == '/')
                filename = p + 1;
            p++;
        }
        printf("File:%s\r\n", filename);
    }

    /* 输出函数名和行号 */
    if (func != NULL)
    {
        printf("Func:%s\r\n", func);
    }
    printf("Line:%lu\r\n", (unsigned long)line);

    printf("===================\r\n");
    printf("System halted.\r\n");

    /* 停机（与原实现一致，可在此处闪烁 LED） */
    while (1)
    {
#ifdef LED_ENABLE
        LED_Toggle_All();
        Delay_ms(200);
#endif
    }
}
#elif (DEBUG_OUTPUT_MODE == 3)
/*****************************************************************************
 *                           预留扩展模式3
 *****************************************************************************/

void Debug_Info(const char *msg)
{
    /* 预留扩展 */
    (void)msg;
}

void Debug_Error(const char *msg, const char *file, const char *func, uint32_t line)
{
    /* 预留扩展 */
    (void)msg;
    (void)file;
    (void)func;
    (void)line;
    while (1)
        ;
}

#endif /* DEBUG_OUTPUT_MODE */

#endif /* DEBUG_ENABLE */
