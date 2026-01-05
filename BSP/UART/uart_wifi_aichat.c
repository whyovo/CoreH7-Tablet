#include "uart_wifi_aichat.h"

#ifdef UART_WIFI_ENABLE

#include "uart_wifi.h"
#include "ai_api_key.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 定义 AI 对话使用的连接 ID (0-4)，避开 HTTP Server 可能使用的 ID */
#define AI_CHAT_ID 4

/* 用于累积打印内容的缓冲区 */
static char g_print_buf[256];
static uint16_t g_print_idx = 0;

/**
 * @brief  刷新打印缓冲区到调试口
 */
static void flush_print_buffer(void)
{
    if (g_print_idx > 0)
    {
        g_print_buf[g_print_idx] = '\0';
        // 打印内容
        DEBUG_INFO("%s", g_print_buf);
        g_print_idx = 0;
        memset(g_print_buf, 0, sizeof(g_print_buf));
    }
}

/**
 * @brief  简单的 JSON 字符串转义
 */
static void json_escape_string(const char *input, char *output)
{
    while (*input)
    {
        if (*input == '"')
        {
            *output++ = '\\';
            *output++ = '"';
        }
        else if (*input == '\n')
        {
            *output++ = '\\';
            *output++ = 'n';
        }
        else if (*input == '\r')
        {
            // 忽略
        }
        else
        {
            *output++ = *input;
        }
        input++;
    }
    *output = '\0';
}

/**
 * @brief  解析流式数据中的 content 内容
 * @param  json_chunk: 包含 "content":"..." 的片段
 */
static void parse_and_print_content(char *json_chunk)
{
    char *content_start = strstr(json_chunk, "\"content\":\"");
    if (content_start)
    {
        content_start += 11; // 跳过 "content":"
        char *p = content_start;

        while (*p)
        {
            if (*p == '"' && *(p - 1) != '\\') // 遇到未转义的引号，结束
            {
                break;
            }

            char ch = 0;

            // 处理 \n
            if (*p == '\\' && *(p + 1) == 'n')
            {
                // 遇到换行符，立即刷新缓冲区，这样 DEBUG_INFO 会换行
                flush_print_buffer();
                p += 2;
                continue;
            }
            // 处理其他转义符
            else if (*p == '\\' && *(p + 1) != '\0')
            {
                p++; // 跳过反斜杠
                ch = *p;
            }
            else
            {
                ch = *p;
            }

            // 存入缓冲区
            if (g_print_idx < sizeof(g_print_buf) - 1)
            {
                g_print_buf[g_print_idx++] = ch;
            }
            else
            {
                // 缓冲区满，刷新
                flush_print_buffer();
                g_print_buf[g_print_idx++] = ch;
            }

            p++;
        }
    }
}

void AI_Chat_Send(const char *question)
{
    DEBUG_INFO(">>> User: %s", question);

    // 0. 确保开启多连接模式
    WIFI_SendCmd("AT+CIPMUX=1\r\n", "OK", 1000);

    // 1. 连接 TCP (使用指定 ID)
    if (WIFI_ConnectTCP_Connection(AI_CHAT_ID, AI_API_URL, AI_API_PORT) != WIFI_OK)
    {
        DEBUG_ERROR("Connect Failed");
        return;
    }

    // 2. 准备请求体
    char *body_buf = (char *)malloc(2048);
    char *escaped_q = (char *)malloc(strlen(question) * 2 + 1);

    if (!body_buf || !escaped_q)
    {
        DEBUG_ERROR("Malloc Failed");
        if (body_buf)
            free(body_buf);
        if (escaped_q)
            free(escaped_q);

        // 失败也要尝试关闭连接
        char close_cmd[32];
        snprintf(close_cmd, sizeof(close_cmd), "AT+CIPCLOSE=%d\r\n", AI_CHAT_ID);
        WIFI_SendCmd(close_cmd, NULL, 500);
        return;
    }

    json_escape_string(question, escaped_q);

    sprintf(body_buf,
            "{\"model\": \"%s\", \"messages\": [{\"role\": \"user\", \"content\": \"%s\"}], \"stream\": true}",
            AI_MODEL_NAME, escaped_q);

    // 3. 准备请求头并发送
    char *header_buf = (char *)malloc(512);
    sprintf(header_buf,
            "POST /v1/chat/completions HTTP/1.1\r\n"
            "Host: %s\r\n"
            "Authorization: Bearer %s\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %d\r\n"
            "\r\n",
            AI_API_URL, AI_API_KEY, strlen(body_buf));

    // DEBUG_INFO("Sending...");

    // 使用指定 ID 发送数据
    WIFI_SendData_Connection(AI_CHAT_ID, (uint8_t *)header_buf, strlen(header_buf));
    HAL_Delay(50);
    WIFI_SendData_Connection(AI_CHAT_ID, (uint8_t *)body_buf, strlen(body_buf));

    free(body_buf);
    free(escaped_q);
    free(header_buf);

    DEBUG_INFO(">>> AI Thinking...");

    // 4. 接收流式响应循环
    WIFI_RingBuf_Clear(); // 清空之前的缓存

    uint32_t last_rx_tick = HAL_GetTick();
    uint8_t is_finished = 0;
    g_print_idx = 0; // 重置打印缓冲区

    // 本地行缓冲区，用于累积数据直到遇到换行符
    char line_buf[1024];
    uint16_t line_idx = 0;

    while ((HAL_GetTick() - last_rx_tick) < 30000)
    {
        uint8_t ch;
        // 轮询环形缓冲区
        while (WIFI_RingBuf_ReadByte(&ch))
        {
            last_rx_tick = HAL_GetTick(); // 收到数据，刷新超时时间

            // 存入行缓冲区
            if (line_idx < sizeof(line_buf) - 1)
            {
                line_buf[line_idx++] = ch;
            }

            // 遇到换行符或缓冲区满，开始解析
            if (ch == '\n' || line_idx >= sizeof(line_buf) - 1)
            {
                line_buf[line_idx] = '\0'; // 字符串结属符

                // 检查是否结束
                if (strstr(line_buf, "[DONE]"))
                {
                    is_finished = 1;
                }
                else
                {
                    // 查找 "data: " 并解析 JSON 内容
                    char *p_data = strstr(line_buf, "data: ");
                    if (p_data)
                    {
                        parse_and_print_content(p_data);
                    }
                }

                // 重置行缓冲区
                line_idx = 0;
            }
        }

        if (is_finished)
            break;
    }

    // 5. 关闭连接 (释放 ID 4)
    char close_cmd[32];
    snprintf(close_cmd, sizeof(close_cmd), "AT+CIPCLOSE=%d\r\n", AI_CHAT_ID);
    WIFI_SendCmd(close_cmd, NULL, 500);

    flush_print_buffer(); // 打印剩余内容
}

#endif
