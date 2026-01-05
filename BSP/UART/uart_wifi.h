/**
 ******************************************************************************
 * @file    uart_wifi.h
 * @author  菜菜why（B站：菜菜whyy）
 * @brief   ESP8266/ESP32 AT指令驱动头文件
 ******************************************************************************
 */

#ifndef __UART_WIFI_H__
#define __UART_WIFI_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "config.h"

#ifdef UART_WIFI_ENABLE
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

    /* ================= 配置区域 ================= */

    /*
     * 定义使用的 UART 句柄
     * 注意：请确保在 CubeMX 中：
     * 1. 开启了 UART 的 DMA RX 通道 (Mode: Circular !!!)
     * 2. 开启了 UART 的全局中断 (NVIC)
     */
    extern UART_HandleTypeDef huart2;
#define WIFI_UART_HANDLE huart2

/* 接收缓冲区大小 (建议 4KB 或更大) */
#define WIFI_RX_BUF_SIZE 4096
/* 默认超时时间 (ms)*/
#define WIFI_RX_TIMEOUT 5000

    /* ================= 类型定义 ================= */

    typedef enum
    {
        WIFI_OK = 0,
        WIFI_ERROR,
        WIFI_TIMEOUT,
        WIFI_BUSY
    } WIFI_StatusTypeDef;

    /* ================= 函数声明 ================= */

    /**
     * @brief  WIFI模块初始化 (检测硬件、复位、设置Station模式)
     * @return WIFI_OK / WIFI_ERROR
     */
    WIFI_StatusTypeDef UART_WIFI_Init(void);

    /**
     * @brief  发送AT指令并等待预期响应
     * @param  cmd: AT指令字符串 (如 "AT\r\n")
     * @param  expected_resp: 期望收到的响应子串 (如 "OK")，NULL则不检查
     * @param  timeout: 超时时间(ms)
     * @return WIFI_OK: 收到预期响应; WIFI_ERROR: 收到ERROR; WIFI_TIMEOUT: 超时
     */
    WIFI_StatusTypeDef WIFI_SendCmd(const char *cmd, const char *expected_resp, uint32_t timeout);

    /**
     * @brief  连接 Wi-Fi 热点
     * @param  ssid: 热点名称
     * @param  pwd:  密码
     * @return WIFI_StatusTypeDef
     */
    WIFI_StatusTypeDef WIFI_ConnectAP(const char *ssid, const char *pwd);

    /**
     * @brief  建立 TCP 连接
     * @param  ip: 目标IP地址或域名
     * @param  port: 目标端口
     * @return WIFI_StatusTypeDef
     */
    WIFI_StatusTypeDef WIFI_ConnectTCP(const char *ip, uint16_t port);

    /**
     * @brief  建立 TCP 连接 (多连接模式)
     * @param  id: 连接ID (0-4)
     * @param  ip: 目标IP地址或域名
     * @param  port: 目标端口
     * @return WIFI_StatusTypeDef
     */
    WIFI_StatusTypeDef WIFI_ConnectTCP_Connection(uint8_t id, const char *ip, uint16_t port);

    /**
     * @brief  发送数据 (透传或非透传模式下发送指定长度数据)
     * @param  data: 数据指针
     * @param  len:  数据长度
     * @return WIFI_StatusTypeDef
     */
    WIFI_StatusTypeDef WIFI_SendData(uint8_t *data, uint16_t len);

    /**
     * @brief  多连接模式下发送数据 (用于HTTP Server回复)
     * @param  id: 连接ID
     * @param  data: 数据指针
     * @param  len:  数据长度
     * @return WIFI_StatusTypeDef
     */
    WIFI_StatusTypeDef WIFI_SendData_Connection(uint8_t id, uint8_t *data, uint16_t len);

    /**
     * @brief  获取本机 IP 地址
     * @param  ip_buf: 存储IP字符串的缓冲区
     */
    uint8_t WIFI_GetIP(char *ip_buf);

    /* ================= 环形缓冲区操作函数 ================= */

    /**
     * @brief  清空环形缓冲区指针
     */
    void WIFI_RingBuf_Clear(void);

    /**
     * @brief  获取环形缓冲区中待读取的字节数
     */
    uint16_t WIFI_RingBuf_Available(void);

    /**
     * @brief  从环形缓冲区读取一个字节
     * @param  data: 存放读取数据的指针
     * @return 1:读取成功, 0:缓冲区空
     */
    uint8_t WIFI_RingBuf_ReadByte(uint8_t *data);

    /**
     * @brief  从环形缓冲区读取指定长度数据 (非阻塞，读多少算多少)
     * @param  buf: 目标缓冲区
     * @param  len: 期望读取长度
     * @return 实际读取长度
     */
    uint16_t WIFI_RingBuf_Read(uint8_t *buf, uint16_t len);

#endif

#ifdef __cplusplus
}
#endif

#endif /* __UART_WIFI_H__ */
