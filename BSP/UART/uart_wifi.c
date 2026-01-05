/**
 ******************************************************************************
 * @file    uart_wifi.c
 * @author  菜菜why（B站：菜菜whyy）
 * @brief   ESP8266/ESP32 AT指令驱动实现
 ******************************************************************************
 */

#include "uart_wifi.h"

#ifdef UART_WIFI_ENABLE

/* 接收缓冲区 (DMA 循环写入) */
/* 建议将此缓冲区放置在 D2 域 (SRAM1/2) 以获得最佳 DMA 性能 */
uint8_t g_wifi_rx_buf[WIFI_RX_BUF_SIZE];

/* 环形缓冲区读指针 (写指针由 DMA CNDTR 寄存器决定) */
static volatile uint16_t g_rx_read_idx = 0;

/**
 * @brief  获取 DMA 当前写入位置 (即缓冲区末尾的索引)
 * @note   CNDTR 寄存器是递减的，初始值为 BufferSize
 */
static uint16_t WIFI_GetDMARxHead(void)
{
    return WIFI_RX_BUF_SIZE - __HAL_DMA_GET_COUNTER(WIFI_UART_HANDLE.hdmarx);
}

/**
 * @brief  清空环形缓冲区指针 (丢弃所有未读数据)
 */
void WIFI_RingBuf_Clear(void)
{
    // 只需要重置读指针，让它追上 DMA 的写指针
    g_rx_read_idx = WIFI_GetDMARxHead();
}

/**
 * @brief  获取环形缓冲区中待读取的字节数
 */
uint16_t WIFI_RingBuf_Available(void)
{
    uint16_t head = WIFI_GetDMARxHead();
    if (head >= g_rx_read_idx)
    {
        return head - g_rx_read_idx;
    }
    else
    {
        return WIFI_RX_BUF_SIZE - (g_rx_read_idx - head);
    }
}

/**
 * @brief  从环形缓冲区读取一个字节
 * @param  data: 存放读取数据的指针
 * @return 1:读取成功, 0:缓冲区空
 */
uint8_t WIFI_RingBuf_ReadByte(uint8_t *data)
{
    if (WIFI_RingBuf_Available() == 0)
        return 0;

    *data = g_wifi_rx_buf[g_rx_read_idx];
    g_rx_read_idx++;
    if (g_rx_read_idx >= WIFI_RX_BUF_SIZE)
    {
        g_rx_read_idx = 0;
    }
    return 1;
}

/**
 * @brief  从环形缓冲区读取指定长度数据
 */
uint16_t WIFI_RingBuf_Read(uint8_t *buf, uint16_t len)
{
    uint16_t count = 0;
    while (count < len && WIFI_RingBuf_Available())
    {
        WIFI_RingBuf_ReadByte(&buf[count++]);
    }
    return count;
}

/**
 * @brief  开启 DMA 循环接收
 */
static void WIFI_StartRx_Circular(void)
{
    // 强制修改 DMA 模式为 Circular (防止 CubeMX 配置遗漏)
    WIFI_UART_HANDLE.hdmarx->Init.Mode = DMA_CIRCULAR;
    HAL_DMA_Init(WIFI_UART_HANDLE.hdmarx);

    // 启动 DMA 接收
    HAL_UART_Receive_DMA(&WIFI_UART_HANDLE, g_wifi_rx_buf, WIFI_RX_BUF_SIZE);
}

/**
 * @brief  UART WIFI模块初始化
 */
WIFI_StatusTypeDef UART_WIFI_Init(void)
{
    DEBUG_INFO("WIFI Init Start...");

    // 1. 启动循环接收
    WIFI_StartRx_Circular();
    WIFI_RingBuf_Clear();

    // 2. 测试 AT 响应
    WIFI_StatusTypeDef status = WIFI_ERROR;
    uint8_t retry = 3;
    while (retry--)
    {
        if (WIFI_SendCmd("AT\r\n", "OK", 1000) == WIFI_OK)
        {
            status = WIFI_OK;
            break;
        }
        HAL_Delay(500);
    }

    if (status != WIFI_OK)
    {
        DEBUG_ERROR("WIFI AT Check Failed");
        return status;
    }

    WIFI_SendCmd("ATE0\r\n", "OK", 500);
    WIFI_SendCmd("AT+CWMODE=3\r\n", "OK", 1000);

    DEBUG_INFO("WIFI Init Success");
    return WIFI_OK;
}

/**
 * @brief  发送AT指令并等待预期响应 (适配环形缓冲区)
 */
WIFI_StatusTypeDef WIFI_SendCmd(const char *cmd, const char *expected_resp, uint32_t timeout)
{
    // 1. 清空之前的残留数据
    WIFI_RingBuf_Clear();

    // 2. 发送指令
    HAL_UART_Transmit(&WIFI_UART_HANDLE, (uint8_t *)cmd, strlen(cmd), 100);

    if (expected_resp == NULL)
        return WIFI_OK;

    // 3. 循环读取并匹配
    uint32_t tick_start = HAL_GetTick();

    // 使用一个小的临时缓冲区来做匹配
    char match_buf[128] = {0};
    uint16_t match_idx = 0;

    while ((HAL_GetTick() - tick_start) < timeout)
    {
        uint8_t ch;
        while (WIFI_RingBuf_ReadByte(&ch))
        {
            // 存入临时匹配缓冲区
            if (match_idx < sizeof(match_buf) - 1)
            {
                match_buf[match_idx++] = ch;
                match_buf[match_idx] = '\0';
            }
            else
            {
                // 缓冲区满了，移动数据 (简单的滑动窗口)
                memmove(match_buf, match_buf + 1, sizeof(match_buf) - 2);
                match_buf[sizeof(match_buf) - 2] = ch;
                match_buf[sizeof(match_buf) - 1] = '\0';
            }

            // 检查匹配
            if (strstr(match_buf, expected_resp) != NULL)
            {
                return WIFI_OK;
            }
            if (strstr(match_buf, "ERROR") != NULL)
            {
                return WIFI_ERROR;
            }
        }
        // 稍微延时，避免死循环占用总线
        // HAL_Delay(1);
    }

    DEBUG_INFO("WIFI CMD Timeout: %s", cmd);
    return WIFI_TIMEOUT;
}

/**
 * @brief  连接 Wi-Fi 热点
 */
WIFI_StatusTypeDef WIFI_ConnectAP(const char *ssid, const char *pwd)
{

    // if (WIFI_SendCmd("AT+CIPSERVER=0\r\n", "OK", 1000) != WIFI_OK)
    // {
    //     DEBUG_ERROR("Close CIPSERVER Failed");
    // }
    // if (WIFI_SendCmd("AT+CIPMUX=0\r\n", "OK", 1000) != WIFI_OK)
    // {
    //     DEBUG_ERROR("Set CIPMUX Failed");
    // }

    // 1. 先检查当前是否已经连接到该热点
    // 发送 AT+CWJAP? 查询，如果响应中包含目标 SSID，说明已连接
    if (WIFI_SendCmd("AT+CWJAP?\r\n", ssid, 1000) == WIFI_OK)
    {
        DEBUG_INFO("WIFI Already Connected");
        return WIFI_OK;
    }

    // 2. 如果未连接，则发起强制连接
    DEBUG_INFO("WIFI Connecting to SSID: %s ...", ssid);
    char cmd_buf[128];

    // 格式: AT+CWJAP="SSID","PWD"
    snprintf(cmd_buf, sizeof(cmd_buf), "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, pwd);

    WIFI_StatusTypeDef res = WIFI_SendCmd(cmd_buf, "OK", 15000);

    if (res == WIFI_OK)
    {
        DEBUG_INFO("WIFI Connected!");
    }
    else
    {
        DEBUG_ERROR("WIFI Connect Failed: %d", res);
    }
    return res;
}

/**
 * @brief  建立 TCP 连接
 */
WIFI_StatusTypeDef WIFI_ConnectTCP(const char *ip, uint16_t port)
{
    char cmd_buf[128];

    // 格式: AT+CIPSTART="TCP","IP",PORT
    snprintf(cmd_buf, sizeof(cmd_buf), "AT+CIPSTART=\"TCP\",\"%s\",%d\r\n", ip, port);
    return WIFI_SendCmd(cmd_buf, "OK", 5000);
}

/**
 * @brief  建立 TCP 连接 (多连接模式)
 */
WIFI_StatusTypeDef WIFI_ConnectTCP_Connection(uint8_t id, const char *ip, uint16_t port)
{
    char cmd_buf[128];
    // 格式: AT+CIPSTART=<id>,"TCP","IP",PORT
    snprintf(cmd_buf, sizeof(cmd_buf), "AT+CIPSTART=%d,\"TCP\",\"%s\",%d\r\n", id, ip, port);
    return WIFI_SendCmd(cmd_buf, "OK", 5000);
}

/**
 * @brief  发送数据
 */
WIFI_StatusTypeDef WIFI_SendData(uint8_t *data, uint16_t len)
{
    char cmd_buf[32];
    snprintf(cmd_buf, sizeof(cmd_buf), "AT+CIPSEND=%d\r\n", len);
    if (WIFI_SendCmd(cmd_buf, ">", 2000) != WIFI_OK)
        return WIFI_ERROR;

    WIFI_RingBuf_Clear();
    HAL_UART_Transmit(&WIFI_UART_HANDLE, data, len, 1000);

    // 等待 SEND OK
    uint32_t tick_start = HAL_GetTick();
    char match_buf[32] = {0};
    uint16_t match_idx = 0;

    while ((HAL_GetTick() - tick_start) < 5000)
    {
        uint8_t ch;
        while (WIFI_RingBuf_ReadByte(&ch))
        {
            if (match_idx < sizeof(match_buf) - 1)
            {
                match_buf[match_idx++] = ch;
                match_buf[match_idx] = '\0';
            }
            else
            {
                memmove(match_buf, match_buf + 1, sizeof(match_buf) - 2);
                match_buf[sizeof(match_buf) - 2] = ch;
            }
            if (strstr(match_buf, "SEND OK"))
                return WIFI_OK;
            if (strstr(match_buf, "ERROR"))
                return WIFI_ERROR;
        }
    }
    return WIFI_TIMEOUT;
}

/**
 * @brief  多连接模式下发送数据
 */
WIFI_StatusTypeDef WIFI_SendData_Connection(uint8_t id, uint8_t *data, uint16_t len)
{
    char cmd_buf[32];
    // 多连接模式指令: AT+CIPSEND=<id>,<len>
    snprintf(cmd_buf, sizeof(cmd_buf), "AT+CIPSEND=%d,%d\r\n", id, len);

    if (WIFI_SendCmd(cmd_buf, ">", 2000) != WIFI_OK)
        return WIFI_ERROR;

    WIFI_RingBuf_Clear();
    HAL_UART_Transmit(&WIFI_UART_HANDLE, data, len, 1000);

    // 等待 SEND OK
    uint32_t tick_start = HAL_GetTick();
    char match_buf[32] = {0};
    uint16_t match_idx = 0;

    while ((HAL_GetTick() - tick_start) < 5000)
    {
        uint8_t ch;
        while (WIFI_RingBuf_ReadByte(&ch))
        {
            if (match_idx < sizeof(match_buf) - 1)
            {
                match_buf[match_idx++] = ch;
                match_buf[match_idx] = '\0';
            }
            else
            {
                memmove(match_buf, match_buf + 1, sizeof(match_buf) - 2);
                match_buf[sizeof(match_buf) - 2] = ch;
            }
            if (strstr(match_buf, "SEND OK"))
                return WIFI_OK;
            if (strstr(match_buf, "ERROR"))
                return WIFI_ERROR;
        }
    }
    return WIFI_TIMEOUT;
}

/**
 * @brief  获取本机 IP 地址
 */
uint8_t WIFI_GetIP(char *ip_buf)
{
    if (ip_buf == NULL)
        return 0;

    // 1. 清空接收缓冲区，准备接收 IP 响应
    WIFI_RingBuf_Clear();

    // 2. 发送指令
    HAL_UART_Transmit(&WIFI_UART_HANDLE, (uint8_t *)"AT+CIFSR\r\n", 10, 100);

    uint32_t tick_start = HAL_GetTick();
    char temp_buf[256] = {0}; // 足够存放 CIFSR 的所有返回内容
    uint16_t idx = 0;

    // 3. 等待数据并捕获
    // AT+CIFSR 的返回通常包含多行 (APIP, STAIP)，以 OK 结尾
    while ((HAL_GetTick() - tick_start) < 2000)
    {
        uint8_t ch;
        // 在 H7 上，如果是 DMA 模式，读取前强制失效 Cache
        SCB_InvalidateDCache_by_Addr((uint32_t *)g_wifi_rx_buf, WIFI_RX_BUF_SIZE);

        while (WIFI_RingBuf_ReadByte(&ch))
        {
            if (idx < sizeof(temp_buf) - 1)
                temp_buf[idx++] = ch;
        }

        // 检查是否收到了 OK 结束标志
        if (strstr(temp_buf, "OK"))
            break;
    }

    // 4. 解析 STAIP (Station 模式的 IP)
    // 响应格式通常为: +CIFSR:STAIP,"192.168.x.x"
    char *p_sta = strstr(temp_buf, "STAIP,\"");
    if (p_sta)
    {
        p_sta += 7;                       // 跳过 STAIP,"
        char *p_end = strchr(p_sta, '"'); // 找到结束引号
        if (p_end)
        {
            uint16_t ip_len = p_end - p_sta;
            if (ip_len < 16)
            {
                strncpy(ip_buf, p_sta, ip_len);
                ip_buf[ip_len] = '\0';
                DEBUG_INFO("Local IP: %s", ip_buf);
                return 1;
            }
        }
    }

    DEBUG_ERROR("Get IP Failed or Not Connected");
    return 0;
}
#endif
