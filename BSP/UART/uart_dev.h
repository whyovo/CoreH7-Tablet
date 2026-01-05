/**
 ******************************************************************************
 * @file    uart_dev.h
 * @brief   通用UART设备驱动头文件 (蓝牙/有线串口等)
 ******************************************************************************
 */

#ifndef __UART_DEV_H__
#define __UART_DEV_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "config.h"

#ifdef UART_DEV_ENABLE
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

    /* ================= 配置区域 ================= */

    /*
     * 定义使用的 UART 句柄 (默认 UART3)
     * 注意：请确保在 CubeMX 中：
     * 1. 开启了 UART3 的 DMA RX 通道 (Mode: Normal)
     * 2. 开启了 UART3 的全局中断 (NVIC)
     */
    extern UART_HandleTypeDef huart3;
#define UART_DEV_HANDLE huart3

/* 接收缓冲区大小 */
#define UART_DEV_RX_BUF_SIZE 512
/* 默认超时时间 (ms)*/
#define UART_DEV_TIMEOUT 1000

    /* ================= 变量导出 ================= */
    extern uint8_t g_dev_rx_buf[UART_DEV_RX_BUF_SIZE];
    extern volatile uint16_t g_dev_rx_len;
    extern volatile uint8_t g_dev_rx_event_flag;

    /* ================= 类型定义 ================= */

    typedef enum
    {
        DEV_OK = 0,
        DEV_ERROR,
        DEV_TIMEOUT
    } DEV_StatusTypeDef;

    /* ================= 函数声明 ================= */

    /**
     * @brief  通用设备初始化 (开启接收)
     */
    void UART_DEV_Init(void);

    /**
     * @brief  发送数据
     * @param  data: 数据指针
     * @param  len:  数据长度
     */
    void UART_DEV_SendData(uint8_t *data, uint16_t len);

    /**
     * @brief  发送字符串
     */
    void UART_DEV_SendString(const char *str);

    /**
     * @brief  发送指令并等待预期响应 (适用于AT指令设备)
     * @param  cmd: 指令字符串
     * @param  expected_resp: 期望响应
     * @param  timeout: 超时时间
     */
    DEV_StatusTypeDef UART_DEV_SendCmd(const char *cmd, const char *expected_resp, uint32_t timeout);

    /**
     * @brief  清空接收缓冲区
     */
    void UART_DEV_ClearRxBuffer(void);

    /**
     * @brief  开启接收
     */
    void UART_DEV_StartRx(uint16_t offset);

    /**
     * @brief  UART 接收事件回调 (供 HAL_UARTEx_RxEventCallback 调用)
     */
    void UART_DEV_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size);

#endif

#ifdef __cplusplus
}
#endif

#endif /* __UART_DEV_H__ */