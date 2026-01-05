#include "uart_wifi_http.h"
#include "uart_wifi.h"
#include <string.h>
#include <stdio.h>

#ifdef UART_WIFI_ENABLE

/* 引入硬件驱动头文件 */
#ifdef LED_ENABLE
#include "led.h"
#endif

typedef struct
{
    char url[32];
    HttpHandlerFunc handler;
    uint32_t args; // 参数存储
} UrlHandler_t;

static UrlHandler_t g_url_handlers[MAX_URL_HANDLERS];
static uint8_t g_handler_count = 0;

/* ================= 用户回调处理函数 ================= */

// 定义命令ID枚举
enum
{
    CMD_LED_ON = 1,
    CMD_LED_OFF,
    CMD_BEEP
};

/**
 * @brief  统一的 Web 命令处理函数
 */
static void Handle_Web_Command(uint8_t id, uint32_t cmd)
{
    switch (cmd)
    {
    case CMD_LED_ON:
        DEBUG_INFO("Command: LED ON");
#ifdef LED_ENABLE
        LED_On(LD1);
#endif
        WIFI_HTTP_Send_Response(id, "<script>alert('LED ON');window.location.href='/';</script>");
        break;

    case CMD_LED_OFF:
        DEBUG_INFO("Command: LED OFF");
#ifdef LED_ENABLE
        LED_Off(LD1);
#endif
        WIFI_HTTP_Send_Response(id, "<script>alert('LED OFF');window.location.href='/';</script>");
        break;

    case CMD_BEEP:
        DEBUG_INFO("Command: BEEP");//不响，就是个占位符

        WIFI_HTTP_Send_Response(id, "<script>window.location.href='/';</script>");
        break;

    default:
        WIFI_HTTP_Send_Response(id, "Unknown Command");
        break;
    }
}

/**
 * @brief  一键启动 HTTP Server 演示
 */
void WIFI_HTTP_Demo_Start(const char *ssid, const char *pwd)
{
    // 启动 HTTP Server (AP模式)
    if (WIFI_HTTP_Server_Init(ssid, pwd))
    {
        // 注册路由回调 (使用同一个函数，传入不同ID)
        WIFI_HTTP_Register_Handler("/led_on", Handle_Web_Command, CMD_LED_ON);
        WIFI_HTTP_Register_Handler("/led_off", Handle_Web_Command, CMD_LED_OFF);
        WIFI_HTTP_Register_Handler("/beep", Handle_Web_Command, CMD_BEEP);

        DEBUG_INFO("HTTP Demo Started. Connect to WiFi: %s", ssid);
    }
}

/**
 * @brief  初始化 HTTP 服务器 (AP模式)
 */
uint8_t WIFI_HTTP_Server_Init(const char *ssid, const char *pwd)
{
    DEBUG_INFO("Starting HTTP Server...");

    //  配置热点参数: SSID, PWD, Ch(1), ECN(3=WPA2_PSK)
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "AT+CWSAP=\"%s\",\"%s\",1,3\r\n", ssid, pwd);
    if (WIFI_SendCmd(cmd, "OK", 2000) != WIFI_OK)
    {
        DEBUG_ERROR("Set CWSAP Failed");
        return 0;
    }

    // 开启多连接模式
    if (WIFI_SendCmd("AT+CIPMUX=1\r\n", "OK", 1000) != WIFI_OK)
    {
        DEBUG_ERROR("Set CIPMUX Failed");
        return 0;
    }

    // 开启服务器 端口80
    if (WIFI_SendCmd("AT+CIPSERVER=1,80\r\n", "OK", 1000) != WIFI_OK)
    {
        DEBUG_ERROR("Start Server Failed");
        return 0;
    }

    // 获取本机IP并打印
    WIFI_SendCmd("AT+CIFSR\r\n", "APIP", 1000);

    DEBUG_INFO("HTTP Server Started! Connect to SSID: %s", ssid);
    return 1;
}

/**
 * @brief  注册 URL 处理函数
 */
void WIFI_HTTP_Register_Handler(const char *url, HttpHandlerFunc handler, uint32_t args)
{
    if (g_handler_count < MAX_URL_HANDLERS)
    {
        strncpy(g_url_handlers[g_handler_count].url, url, 31);
        g_url_handlers[g_handler_count].handler = handler;
        g_url_handlers[g_handler_count].args = args; // 保存参数
        g_handler_count++;
    }
}

/**
 * @brief  发送 HTTP 响应
 */
void WIFI_HTTP_Send_Response(uint8_t id, const char *html_body)
{
    char header[128];
    uint32_t content_len = strlen(html_body);

    snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html\r\n"
             "Content-Length: %lu\r\n"
             "Connection: close\r\n\r\n",
             content_len);

    WIFI_SendData_Connection(id, (uint8_t *)header, strlen(header));
    HAL_Delay(10); // 稍微延时防止粘包
    WIFI_SendData_Connection(id, (uint8_t *)html_body, content_len);
}

/**
 * @brief  默认主页处理
 */
static void Default_Home_Handler(uint8_t id)
{
    const char *html =
        "<html><head><meta name='viewport' content='width=device-width, initial-scale=1'>"
        "<style>button{width:100%;height:50px;font-size:20px;margin:10px 0;}</style></head>"
        "<body><h1 align='center'>STM32 Control Panel</h1>"
        "<p align='center'>Hello World from STM32!</p>"
        "<hr>"
        "<a href='/led_on'><button>Turn LED ON</button></a>"
        "<a href='/led_off'><button>Turn LED OFF</button></a>"
        "<a href='/beep'><button>Beep Once</button></a>"
        "</body></html>";

    WIFI_HTTP_Send_Response(id, html);
}

/**
 * @brief  HTTP 任务轮询
 * @note   解析格式: +IPD,<id>,<len>:GET /path HTTP/1.1
 */
void WIFI_HTTP_Task(void)
{
    static char rx_line[256];
    static uint16_t rx_idx = 0;
    uint8_t ch;

    // 简单的行解析，寻找 +IPD
    while (WIFI_RingBuf_ReadByte(&ch))
    {
        if (rx_idx < sizeof(rx_line) - 1)
        {
            rx_line[rx_idx++] = ch;
        }

        // 遇到换行或缓冲区满
        if (ch == '\n' || rx_idx >= sizeof(rx_line) - 1)
        {
            rx_line[rx_idx] = '\0';

            // 检查是否是接收到的数据头
            char *ipd_ptr = strstr(rx_line, "+IPD,");
            if (ipd_ptr)
            {
                // 解析 ID
                int id = 0;
                sscanf(ipd_ptr, "+IPD,%d,", &id);

                // 检查是否包含 GET 请求
                char *get_ptr = strstr(rx_line, "GET ");
                if (get_ptr)
                {
                    char *path_start = get_ptr + 4;           // 跳过 "GET "
                    char *path_end = strchr(path_start, ' '); // 找到路径结束的空格

                    if (path_end)
                    {
                        *path_end = '\0'; // 截断路径字符串

                        DEBUG_INFO("HTTP Req ID:%d Path:%s", id, path_start);

                        uint8_t handled = 0;
                        // 遍历注册的处理器
                        for (int i = 0; i < g_handler_count; i++)
                        {
                            if (strcmp(path_start, g_url_handlers[i].url) == 0)
                            {
                                // 调用回调函数时传入保存的 args
                                g_url_handlers[i].handler(id, g_url_handlers[i].args);
                                handled = 1;
                                break;
                            }
                        }

                        // 如果没有匹配或者是根路径，显示主页
                        if (!handled)
                        {
                            Default_Home_Handler(id);
                        }

                    }
                }
            }
            rx_idx = 0; // 重置行缓冲区
        }
    }
}

/**
 * @brief  关闭 HTTP 服务器并清理连接
 */
uint8_t WIFI_HTTP_Server_Stop(void)
{
    DEBUG_INFO("Stopping HTTP Server...");

    // 1. 关闭服务器监听
    // 成功后，模块不再接受新的 TCP 握手
    if (WIFI_SendCmd("AT+CIPSERVER=0\r\n", "OK", 1000) != WIFI_OK)
    {
        DEBUG_ERROR("Close CIPSERVER Failed");
    }

    // 2. 强制关闭所有可能的活动连接 (ID 0-4)
    // 防止旧连接挂死导致无法修改配置
    WIFI_SendCmd("AT+CIPCLOSE=5\r\n", NULL, 500);

    DEBUG_INFO("HTTP Server Stopped.");
    return WIFI_OK;
}

#endif
