/**
 ******************************************************************************
 * @file    key.c
 * @author  菜菜why（B站：菜菜whyy）
 * @brief   按键驱动实现文件
 ******************************************************************************
 * @attention
 *
 * 本文件实现：
 * - 软件消抖（可配置消抖时间）
 * - 事件检测：按下/释放/单击/双击/长按
 * - 弱符号事件处理函数，支持HAL风格回调
 ******************************************************************************
 */

#include "key.h"

#ifdef KEY_ENABLE

/*******************************************************************************
 *                              私有配置参数
 ******************************************************************************/
/**
 * @brief 可根据实际硬件特性调整以下参数
 */
#define KEY_DEBOUNCE_MS 20 /*!< 消抖时间（毫秒），典型10~50ms */
#define KEY_LONG_MS 600    /*!< 长按触发时间（毫秒） */
#define KEY_DBL_MS 200     /*!< 双击最大间隔时间（毫秒） */

/*******************************************************************************
 *                              私有状态数组
 ******************************************************************************/

/**
 * @brief  按键数组(根据KEY_LIST自动生成)
 */
#define X(name, port, pin) {port, pin, 0},
KEY keys[] = {
    KEY_LIST};
#undef X

static uint8_t stable_state[KEY_COUNT];    /*!< 去抖后的稳定状态（0/1） */
static uint8_t last_raw[KEY_COUNT];        /*!< 上一次原始电平（用于检测变化） */
static uint32_t last_change_ts[KEY_COUNT]; /*!< 上一次电平变化时间戳 */
static uint32_t press_ts[KEY_COUNT];       /*!< 按下时间戳 */
static uint32_t release_ts[KEY_COUNT];     /*!< 释放时间戳 */
static uint8_t click_pending[KEY_COUNT];   /*!< 单击待处理标志（等待双击窗口） */
static uint8_t long_reported[KEY_COUNT];   /*!< 长按已报告标志 */

static KEY_Callback callbacks[KEY_COUNT] = {0}; /*!< 回调函数数组 */

/*******************************************************************************
 *                              私有函数声明
 ******************************************************************************/
static void emit_event(KEY_ID id, KEY_Event ev);

/**
 * @brief  读取按键原始GPIO电平
 * @param  key: 按键指针
 * @retval GPIO_PIN_RESET(0) 或 GPIO_PIN_SET(1)
 */
uint8_t KEY_ReadRaw(KEY *key)
{
    return (uint8_t)GPIO_ReadPin(key->port, key->pin);
}

/**
 * @brief  判断按键是否处于按下状态（考虑极性）
 * @param  key: 按键指针
 * @retval 0=未按下，1=按下
 */
uint8_t KEY_IsPressed(KEY *key)
{
    uint8_t raw = KEY_ReadRaw(key);
    /* 按下时电平与空闲电平相反 */
    return (raw != key->idle_level) ? 1 : 0;
}

/**
 * @brief  弱符号事件处理函数（默认空实现）
 * @param  id: 按键ID
 * @param  ev: 事件类型
 * @note   用户在任意C文件中重新实现此函数以接管按键事件
 * @note   不同编译器的弱符号语法略有差异，此处做兼容处理
 */
#if defined(__GNUC__) || defined(__clang__)
__attribute__((weak)) void KEY_EventHandler(KEY_ID id, KEY_Event ev)
{
    (void)id;
    (void)ev;
}
#elif defined(__ICCARM__)
/* IAR不支持同名弱符号覆盖，用户可使用回调注册或链接脚本 */
void KEY_EventHandler(KEY_ID id, KEY_Event ev)
{
    (void)id;
    (void)ev;
}
#else
__weak void KEY_EventHandler(KEY_ID id, KEY_Event ev)
{
    (void)id;
    (void)ev;
}
#endif

/**
 * @brief  事件分发内部函数
 * @param  id: 按键ID
 * @param  ev: 事件类型
 * @note   优先调用已注册回调，否则调用弱处理函数
 */
static void emit_event(KEY_ID id, KEY_Event ev)
{
    if (id >= KEY_COUNT)
        return;

    if (callbacks[id])
        callbacks[id](id, ev); /* 调用注册的回调 */
    else
        KEY_EventHandler(id, ev); /* 调用弱处理函数（用户可重定义） */
}

/**
 * @brief  初始化所有按键
 * @note   清空状态数组并读取初始电平
 * @retval None
 */
void KEY_Init(void)
{
    uint32_t now = GetTick();
    for (int i = 0; i < KEY_COUNT; ++i)
    {
        /* 读取初始电平作为空闲电平(假设按键未按下) */
        keys[i].idle_level = KEY_ReadRaw(&keys[i]);

        last_raw[i] = keys[i].idle_level;
        stable_state[i] = 0; /* 初始化为未按下 */
        last_change_ts[i] = now;
        press_ts[i] = 0;
        release_ts[i] = 0;
        click_pending[i] = 0;
        long_reported[i] = 0;
        callbacks[i] = NULL;
    }
}

/**
 * @brief  注册按键回调函数
 * @param  id: 按键ID
 * @param  cb: 回调函数指针
 * @note   注册后该按键的事件将调用回调而非KEY_EventHandler
 * @retval None
 */
void KEY_RegisterCallback(KEY_ID id, KEY_Callback cb)
{
    if (id >= KEY_COUNT)
    {
        DEBUG_ERROR("KEY_RegisterCallback: 无效的按键ID");
        return;
    }
    callbacks[id] = cb;
    
}

/**
 * @brief  取消按键回调注册
 * @param  id: 按键ID
 * @note   取消后该按键的事件将回到调用KEY_EventHandler
 * @retval None
 */
void KEY_UnregisterCallback(KEY_ID id)
{
    if (id >= KEY_COUNT)
    {
        DEBUG_ERROR("KEY_UnregisterCallback: 无效的按键ID");
        return;
    }
    callbacks[id] = NULL;
}

/**
 * @brief  按键扫描任务（非阻塞，需周期调用）
 * @note   建议在主循环或定时器中每5~20ms调用一次
 * @note   内部处理：消抖 -> 边沿检测 -> 事件识别 -> 回调触发
 * @retval None
 *
 * @par    状态机逻辑：
 *         1. 检测原始电平变化 -> 重置消抖计时器
 *         2. 原始电平稳定超过消抖时间 -> 确认为有效电平
 *         3. 有效电平与上次稳定状态不同 -> 触发边沿事件（按下/释放）
 *         4. 按下后持续时间检测 -> 触发长按事件
 *         5. 释放后检测持续时间 -> 判断单击/双击
 */
void KEY_Task(void)
{
    uint32_t now = GetTick();

    for (int i = 0; i < KEY_COUNT; ++i)
    {
        uint8_t raw = KEY_ReadRaw(&keys[i]);
        uint8_t pressed = (raw != keys[i].idle_level) ? 1 : 0;

        /* 阶段1: 检测原始电平变化 */
        if (raw != last_raw[i])
        {
            last_change_ts[i] = now; /* 重置消抖计时器 */
            last_raw[i] = raw;
        }
        else
        {
            /* 阶段2: 原始电平稳定,检查是否超过消抖时间 */
            if (((int32_t)(now - last_change_ts[i])) >= KEY_DEBOUNCE_MS)
            {
                /* 阶段3: 比较去抖后状态,检测边沿 */
                if (pressed != stable_state[i])
                {
                    stable_state[i] = pressed;

                    if (pressed)
                    {
                        /* 按下边沿 */
                        press_ts[i] = now;
                        long_reported[i] = 0;
                        emit_event((KEY_ID)i, KEY_EV_PRESS);
                    }
                    else
                    {
                        /* 释放边沿 */
                        release_ts[i] = now;
                        emit_event((KEY_ID)i, KEY_EV_RELEASE);

                        uint32_t held = (press_ts[i] ? (now - press_ts[i]) : 0);

                        if (held < KEY_LONG_MS)
                        {
                            /* 短按: 检测单击/双击 */
                            if (click_pending[i] && ((now - release_ts[i]) <= KEY_DBL_MS))
                            {
                                /* 双击窗口内的第二次释放 -> 双击 */
                                click_pending[i] = 0;
                                emit_event((KEY_ID)i, KEY_EV_DOUBLE_CLICK);
                            }
                            else
                            {
                                /* 标记单击待处理，等待双击窗口结束 */
                                click_pending[i] = 1;
                            }
                        }
                        else
                        {
                            /* 长按后释放,不当作点击 */
                            click_pending[i] = 0;
                        }
                    }
                }
                else
                {
                    /* 阶段4: 状态稳定,检测长按和双击超时 */
                    if (stable_state[i])
                    {
                        /* 持续按下: 检测长按 */
                        if (!long_reported[i] && press_ts[i] &&
                            ((now - press_ts[i]) >= KEY_LONG_MS))
                        {
                            long_reported[i] = 1;
                            emit_event((KEY_ID)i, KEY_EV_LONG_PRESS);
                        }
                    }
                    else
                    {
                        /* 持续释放: 检测单击超时 */
                        if (click_pending[i] && ((now - release_ts[i]) > KEY_DBL_MS))
                        {
                            click_pending[i] = 0;
                            emit_event((KEY_ID)i, KEY_EV_CLICK);
                        }
                    }
                }
            }
        }
    }
}

#endif // KEY_ENABLE
