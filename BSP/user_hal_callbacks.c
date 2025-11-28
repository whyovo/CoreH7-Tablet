/**
 ******************************************************************************
 * @file    user_hal_callbacks.c
 * @author  菜菜why（B站：菜菜whyy）
 * @brief   用户HAL回调函数实现文件
 ******************************************************************************
 * @attention
 *
 * 本文件用于实现回调函数。
 * 这些函数会覆盖库中的弱符号默认实现。
 ******************************************************************************
 */

#include "user_hal_callbacks.h"

/**
 * @brief  按键事件处理函数（用户实现）
 * @param  id: 按键ID（KEY_ID枚举值）
 * @param  ev: 事件类型（KEY_Event枚举值）
 * @note   此函数覆盖key.c中的弱符号默认实现
 * @note   使用if判断按键ID，switch判断事件类型
 * @retval None
 */
#ifdef KEY_ENABLE
void KEY_EventHandler(KEY_ID id, KEY_Event ev)
{
    /* 根据按键ID分别处理 */
    if (id == KEY1)
    {
        switch (ev)
        {
        case KEY_EV_PRESS:
            DEBUG_INFO("KEY1 按下");
            /* 按下 */
            break;

        case KEY_EV_RELEASE:
            DEBUG_INFO("KEY1 释放");
            /* 释放 */
            break;

        case KEY_EV_CLICK:
            DEBUG_INFO("KEY1 单击");
            /* 单击事件 */
            break;

        case KEY_EV_DOUBLE_CLICK:
            DEBUG_INFO("KEY1 双击");
            /* 双击事件 */
            break;

        case KEY_EV_LONG_PRESS:
            DEBUG_INFO("KEY1 长按");
            /* 长按事件 */
            
            break;
        }
    }
    /* 若有更多按键，在此添加 else if (id == KEY2) { ... } */
}
#endif // KEY_ENABLE

/**
 * @brief  麦克风数据就绪回调函数（用户实现）
 * @param  pData: 指向就绪数据的指针
 * @param  size: 数据长度（采样点数，通常为512）
 * @param  isHalfBuffer: 1=前半缓冲就绪, 0=后半缓冲就绪
 * @note   此函数在DMA中断中调用，应快速处理
 */
#ifdef DMIC_ENABLE
void DMIC_DataReadyCallback(int32_t *pData, uint32_t size, uint8_t isHalfBuffer)
{
    /* 示例1: 计算音频音量（RMS值） */
    int64_t sum = 0;

    for (uint32_t i = 0; i < size; i++)
    {
        sum += (int64_t)pData[i] * (int64_t)pData[i];
    }


}
#endif /* DMIC_ENABLE */
