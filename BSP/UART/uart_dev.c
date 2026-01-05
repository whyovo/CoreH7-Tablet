/**
 ******************************************************************************
 * @file    uart_dev.c
 * @brief   通用UART设备驱动实现
 ******************************************************************************
 */

#include "uart_dev.h"

#ifdef UART_DEV_ENABLE

/* 接收缓冲区 */
uint8_t g_dev_rx_buf[UART_DEV_RX_BUF_SIZE];
/* 当前接收到的数据总长度 */
volatile uint16_t g_dev_rx_len = 0;
/* 接收事件标志位 */
volatile uint8_t g_dev_rx_event_flag = 0;

void UART_DEV_ClearRxBuffer(void)
{
    HAL_UART_DMAStop(&UART_DEV_HANDLE);
    memset(g_dev_rx_buf, 0, UART_DEV_RX_BUF_SIZE);
    g_dev_rx_len = 0;
    g_dev_rx_event_flag = 0;
}

void UART_DEV_StartRx(uint16_t offset)
{
    if (offset >= UART_DEV_RX_BUF_SIZE)
        return;

    if (HAL_UARTEx_ReceiveToIdle_DMA(&UART_DEV_HANDLE,
                                     g_dev_rx_buf + offset,
                                     UART_DEV_RX_BUF_SIZE - offset) != HAL_OK)
    {
        DEBUG_ERROR("DEV StartRx Failed");
    }
}

/**
 * @brief  实际的处理回调 (需要在统一的 HAL_UARTEx_RxEventCallback 中调用)
 */
void UART_DEV_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance == UART_DEV_HANDLE.Instance)
    {
        g_dev_rx_len += Size;

        if (g_dev_rx_len < UART_DEV_RX_BUF_SIZE)
            g_dev_rx_buf[g_dev_rx_len] = '\0';
        else
            g_dev_rx_buf[UART_DEV_RX_BUF_SIZE - 1] = '\0';

        g_dev_rx_event_flag = 1;

        // 可以在这里打印接收到的数据用于调试
        // DEBUG_INFO("DEV RX: %s", g_dev_rx_buf);
    }
}

void UART_DEV_SendData(uint8_t *data, uint16_t len)
{
    HAL_UART_Transmit(&UART_DEV_HANDLE, data, len, 1000);
}

void UART_DEV_SendString(const char *str)
{
    UART_DEV_SendData((uint8_t *)str, strlen(str));
}

DEV_StatusTypeDef UART_DEV_SendCmd(const char *cmd, const char *expected_resp, uint32_t timeout)
{
    UART_DEV_ClearRxBuffer();
    UART_DEV_StartRx(0);
    UART_DEV_SendString(cmd);

    if (expected_resp == NULL)
        return DEV_OK;

    uint32_t tick_start = HAL_GetTick();
    while ((HAL_GetTick() - tick_start) < timeout)
    {
        if (g_dev_rx_event_flag)
        {
            g_dev_rx_event_flag = 0;
            if (strstr((const char *)g_dev_rx_buf, expected_resp) != NULL)
            {
                return DEV_OK;
            }
            UART_DEV_StartRx(g_dev_rx_len);
        }
    }
    return DEV_TIMEOUT;
}

void UART_DEV_Init(void)
{
    DEBUG_INFO("UART DEV Init (UART3)...");
    UART_DEV_ClearRxBuffer();
    UART_DEV_StartRx(0);
}

#endif