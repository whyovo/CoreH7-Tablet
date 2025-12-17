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
#define DEBUG_FONT_SIZE 16   /*!< 调试信息字体大小 */
#define DEBUG_LINE_HEIGHT 16 /*!< 每行高度(像素) */
#define DEBUG_MAX_LINES \
  (LCD_Height / DEBUG_LINE_HEIGHT) /*!< 最大显示行数 */
#define DEBUG_CHAR_WIDTH 8         /*!< ASCII字符宽度*/
#define DEBUG_CHINESE_WIDTH 16     /*!< 中文字符宽度 */
#define DEBUG_MAX_CHARS_PER_LINE 120/*!< 每行最大ASCII字符数 */

/* 调试输出状态管理 */
static uint16_t debug_current_line =
    0;                                /*!< 当前输出行号(0~DEBUG_MAX_LINES-1) */
static uint8_t debug_initialized = 0; /*!< 调试模块初始化标志 */
static uint8_t debug_in_progress = 0; /*!< 递归保护标志 */
/* 日志缓冲区,用于滚动显示 */
static char debug_log_buffer[DEBUG_MAX_LINES][DEBUG_MAX_CHARS_PER_LINE +
                                              1]; /*!< 保存每行日志内容 */
static uint16_t debug_log_count = 0;              /*!< 当前日志条数 */

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
    strncpy(debug_log_buffer[i], debug_log_buffer[i + 1],
            sizeof(debug_log_buffer[i]) - 1);
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

#ifdef IS_GB2312
      /* GB2312/GBK 编码：固定2字节 */
      i++;
#else
      /* UTF-8 编码：变长 */
      uint8_t c = (uint8_t)str[i];
      if ((c & 0xE0) == 0xC0)
        i += 1; // 2字节
      else if ((c & 0xF0) == 0xE0)
        i += 2; // 3字节
      else if ((c & 0xF8) == 0xF0)
        i += 3; // 4字节
                // else 1字节 (异常)
#endif
    }
    else
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
  strncpy(debug_log_buffer[debug_current_line], line_text,
          sizeof(debug_log_buffer[debug_current_line]) - 1);
  debug_log_buffer[debug_current_line]
                  [sizeof(debug_log_buffer[debug_current_line]) - 1] = '\0';

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
 * @brief  内部函数：将纯字符串输出到LCD（处理换行和滚动）
 */
static void Debug_LCD_PrintMsg(const char *msg)
{
  if (msg == NULL)
    return;
  if (debug_in_progress)
    return;
  debug_in_progress = 1;

  if (!debug_initialized)
    Debug_Init();

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

#ifdef IS_GB2312
        /* GB2312/GBK 编码：固定2字节 */
        char_bytes = 2;
#else
        /* UTF-8 编码：根据首字节判断 */
        if (((uint8_t)*p & 0xE0) == 0xC0)
          char_bytes = 2;
        else if (((uint8_t)*p & 0xF0) == 0xE0)
          char_bytes = 3;
        else if (((uint8_t)*p & 0xF8) == 0xF0)
          char_bytes = 4;
        else
          char_bytes = 1; // 无法识别的头字节，当做单字节处理
#endif
      }
      else // ASCII字符
      {
        char_width = DEBUG_CHAR_WIDTH;
        char_bytes = 1;
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

/* 实现 printf 风格的 Debug_Info */
void Debug_Info(const char *fmt, ...)
{
  char buf[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  Debug_LCD_PrintMsg(buf);
}

/* 实现 printf 风格的 Debug_Error */
void Debug_Error(const char *file, const char *func, uint32_t line, const char *fmt, ...)
{
  debug_in_progress = 1;

  char msg_buf[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg_buf, sizeof(msg_buf), fmt, args);
  va_end(args);

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

  if (msg_buf[0] != '\0')
  {
    LCD_DisplayText(0, y, "Msg:");
    y += DEBUG_LINE_HEIGHT;
    // 简单处理消息显示，不做复杂换行计算以简化代码，实际可复用上面的逻辑
    LCD_DisplayText(0, y, msg_buf);
    y += DEBUG_LINE_HEIGHT;
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

  char line_buf[64];
  snprintf(line_buf, sizeof(line_buf), "Line:%lu", line);
  LCD_DisplayText(0, y, line_buf);
  y += DEBUG_LINE_HEIGHT;

  LCD_DisplayText(0, y, "System halted.");

  /* 进入死循环,防止程序继续执行 */
  while (1)
  {
#ifdef LED_ENABLE
#include "led.h"
    LED_Toggle_All();
    HAL_Delay(200);
#endif
  }
}

#elif (DEBUG_OUTPUT_MODE == 1)
/*****************************************************************************
 *                           串口输出模式
 *****************************************************************************/

void Debug_Info(const char *fmt, ...)
{
  char buf[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  char final_buf[300];
  int len = snprintf(final_buf, sizeof(final_buf), "[%lums][INFO] %s\r\n",
                     HAL_GetTick(), buf);
  if (len > 0)
  {
    HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)final_buf, len, HAL_MAX_DELAY);
  }
}

void Debug_Error(const char *file, const char *func, uint32_t line, const char *fmt, ...)
{
  char msg_buf[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg_buf, sizeof(msg_buf), fmt, args);
  va_end(args);

  char error_buf[512];
  int len;

  // 构建完整错误信息
  const char *filename = file;
  if (file != NULL)
  {
    const char *p = file;
    while (*p)
    {
      if (*p == '\\' || *p == '/')
        filename = p + 1;
      p++;
    }
  }

  len = snprintf(error_buf, sizeof(error_buf),
                 "\r\n[%lums] ERROR\r\n===================\r\nMsg:\r\n%s\r\nFile:%s\r\nFunc:%s\r\nLine:%lu\r\n===================\r\nSystem halted.\r\n",
                 HAL_GetTick(), msg_buf, filename ? filename : "N/A", func ? func : "N/A", line);

  if (len > 0)
  {
    HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)error_buf, len, HAL_MAX_DELAY);
  }

  while (1)
  {
#ifdef LED_ENABLE
		#include "led.h"
    LED_Toggle_All();
    HAL_Delay(200);
#endif
  }
}

#elif (DEBUG_OUTPUT_MODE == 2)
/*****************************************************************************
 *                           USB CDC 输出模式
 *****************************************************************************/
#include "usbd_cdc_if.h"

/* 声明 USB_printf，确保在 usbd_cdc_if.c 中已定义 */
extern void USB_printf(const char *format, ...);

void Debug_Info(const char *fmt, ...)
{
  char buf[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  /* 使用 USB_printf 输出带时间戳的信息 */
  USB_printf("[%lums][INFO] %s\r\n", HAL_GetTick(), buf);
}

void Debug_Error(const char *file, const char *func, uint32_t line, const char *fmt, ...)
{
  char msg_buf[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(msg_buf, sizeof(msg_buf), fmt, args);
  va_end(args);

  const char *filename = file;
  if (file != NULL)
  {
    const char *p = file;
    while (*p)
    {
      if (*p == '\\' || *p == '/')
        filename = p + 1;
      p++;
    }
  }

  USB_printf("\r\n[%lums] ERROR\r\n", HAL_GetTick());
  USB_printf("===================\r\n");
  USB_printf("Msg:\r\n%s\r\n", msg_buf);
  USB_printf("File:%s\r\n", filename ? filename : "N/A");
  USB_printf("Func:%s\r\n", func ? func : "N/A");
  USB_printf("Line:%lu\r\n", line);
  USB_printf("===================\r\n");
  USB_printf("System halted.\r\n");

  while (1)
  {
#ifdef LED_ENABLE
		#include "led.h"
    LED_Toggle_All();
    HAL_Delay(200);
#endif
  }
}

#endif /* DEBUG_OUTPUT_MODE */

#endif /* DEBUG_ENABLE */
