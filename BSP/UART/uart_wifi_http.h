#ifndef __UART_WIFI_HTTP_H__
#define __UART_WIFI_HTTP_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "config.h"

#ifdef UART_WIFI_ENABLE

/* 最大支持注册的 URL 处理函数数量 */
#define MAX_URL_HANDLERS 10

    /**
     * @brief URL 处理回调函数类型
     * @param connection_id: 当前连接的ID，用于回传数据
     * @param args: 用户注册时传入的参数(用于区分不同功能)
     */
    typedef void (*HttpHandlerFunc)(uint8_t connection_id, uint32_t args);

    /**
     * @brief  初始化 HTTP 服务器 (AP模式)
     * @param  ssid: 热点名称
     * @param  pwd:  热点密码 (最少8位)
     * @return WIFI_OK / WIFI_ERROR
     */
    uint8_t WIFI_HTTP_Server_Init(const char *ssid, const char *pwd);

    /**
     * @brief  注册 URL 处理函数
     * @param  url: 请求路径 (例如 "/led_on")
     * @param  handler: 对应的回调函数
     * @param  args: 传递给回调函数的参数(如命令ID)
     */
    void WIFI_HTTP_Register_Handler(const char *url, HttpHandlerFunc handler, uint32_t args);

    /**
     * @brief  HTTP 任务轮询 (需在 main_while 中调用)
     */
    void WIFI_HTTP_Task(void);

    /**
     * @brief  发送 HTTP 响应 (HTML内容)
     * @param  id: 连接ID
     * @param  html_body: HTML主体内容
     */
    void WIFI_HTTP_Send_Response(uint8_t id, const char *html_body);

    /**
     * @brief  一键启动 HTTP Server 演示 (初始化WIFI + 开启AP + 注册路由)
     * @param  ssid: 热点名称
     * @param  pwd:  热点密码
     */
    void WIFI_HTTP_Demo_Start(const char *ssid, const char *pwd);

    /**
     * @brief  关闭 HTTP 服务器并清理连接
     */
    uint8_t WIFI_HTTP_Server_Stop(void);

#endif

#ifdef __cplusplus
}
#endif

#endif
