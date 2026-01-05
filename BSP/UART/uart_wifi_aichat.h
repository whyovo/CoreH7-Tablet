#ifndef __UART_WIFI_AICHAT_H__
#define __UART_WIFI_AICHAT_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "config.h"

#ifdef UART_WIFI_ENABLE

    /**
     * @brief  发送问题给 AI 并流式打印回复
     * @param  question: 提问内容
     */
    void AI_Chat_Send(const char *question);

#endif

#ifdef __cplusplus
}
#endif

#endif
