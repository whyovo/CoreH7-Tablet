/**
 ******************************************************************************
 * @file    led.c
 * @author  菜菜why（B站：菜菜whyy）
 * @brief   LED驱动实现文件
 ******************************************************************************
 * @attention
 *
 * 本文件实现：
 * - LED基本控制（开/关/切换,单个/全部）
 * - LED动画效果（闪烁/呼吸/流水，阻塞实现）
 *
 * 注意事项：
 * - 动画函数使用HAL_Delay阻塞，适用于简单应用
 * - 若需非阻塞动画，建议使用状态机+定时器驱动
 *
 ******************************************************************************
 */

#include "led.h"

#ifdef LED_ENABLE

/* Private defines -----------------------------------------------------------*/
#define BREATHE_STEPS 100 /*!< 呼吸效果的PWM分段数 */

/* Private variables ---------------------------------------------------------*/
/** @defgroup LED_Private_Variables LED数组定义
 * @{
 */

/**
 * @brief  LED数组（根据LED_LIST自动生成）
 */
#define X(name, port, pin, direct) {port, pin, direct},
LED leds[] = {
    LED_LIST};
#undef X

/**
 * @}
 */

/**
 * @brief  初始化所有LED（关闭状态）
 * @retval None
 */
void LED_Init(void)
{
    for (int i = 0; i < LED_COUNT; ++i)
    {
        LED_Off(&leds[i]);
    }
}

/**
 * @brief  点亮指定LED
 * @param  led: LED指针
 * @retval None
 */
void LED_On(LED *led)
{
    if (led == NULL)
    {
        DEBUG_ERROR("LED_On: LED指针为空");
        return;
    }
    if (led->direct == 1)
        GPIO_WritePin(led->port, led->pin, GPIO_PIN_SET);
    else
        GPIO_WritePin(led->port, led->pin, GPIO_PIN_RESET);
}

/**
 * @brief  熄灭指定LED
 * @param  led: LED指针
 * @retval None
 */
void LED_Off(LED *led)
{
    if (led == NULL)
    {
        DEBUG_ERROR("LED_Off: LED指针为空");
        return;
    }
    if (led->direct == 1)
        GPIO_WritePin(led->port, led->pin, GPIO_PIN_RESET);
    else
        GPIO_WritePin(led->port, led->pin, GPIO_PIN_SET);
}

/**
 * @brief  切换指定LED状态
 * @param  led: LED指针
 * @retval None
 */
void LED_Toggle(LED *led)
{
    if (led == NULL)
    {
        DEBUG_ERROR("LED_Toggle: LED指针为空");
        return;
    }
    GPIO_TogglePin(led->port, led->pin);
}

/**
 * @brief  点亮所有LED
 * @retval None
 */
void LED_On_All(void)
{
    for (int i = 0; i < LED_COUNT; ++i)
    {
        LED_On(&leds[i]);
    }
}

/**
 * @brief  熄灭所有LED
 * @retval None
 */
void LED_Off_All(void)
{
    for (int i = 0; i < LED_COUNT; ++i)
    {
        LED_Off(&leds[i]);
    }
}

/**
 * @brief  切换所有LED状态
 * @retval None
 */
void LED_Toggle_All(void)
{
    for (int i = 0; i < LED_COUNT; ++i)
    {
        LED_Toggle(&leds[i]);
    }
}

/* Animation functions (blocking) --------------------------------------------*/

/**
 * @brief  单个LED闪烁一个周期（阻塞）
 * @param  led: LED指针
 * @param  period_ms: 闪烁周期（毫秒）
 * @note   内部切换两次（on->off），总耗时=period_ms
 * @retval None
 */
void LED_Blink(LED *led, uint32_t period_ms)
{
    if (led == NULL)
    {
        DEBUG_ERROR("LED_Blink: LED指针为空");
        return;
    }
    if (period_ms == 0)
    {
        DEBUG_ERROR("LED_Blink: 周期为0");
        return;
    }
    LED_Toggle(led);
    Delay_ms(period_ms / 2);
    LED_Toggle(led);
    Delay_ms(period_ms / 2);
}

/**
 * @brief  所有LED同步闪烁一个周期（阻塞）
 * @param  period_ms: 闪烁周期（毫秒）
 * @retval None
 */
void LED_Blink_All(uint32_t period_ms)
{
    if (period_ms == 0)
    {
        DEBUG_ERROR("LED_Blink_All: 周期为0");
        return;
    }
    LED_Toggle_All();
    Delay_ms(period_ms / 2);
    LED_Toggle_All();
    Delay_ms(period_ms / 2);
}

/**
 * @brief  单个LED呼吸效果一个周期（阻塞）
 * @param  led: LED指针
 * @param  period_ms: 呼吸总周期（毫秒），从暗到亮再到暗
 * @note   使用软件PWM，分100步渐变，总耗时≈period_ms
 * @retval None
 */
void LED_Breathe(LED *led, uint32_t period_ms)
{
    if (led == NULL)
    {
        DEBUG_ERROR("LED_Breathe: LED指针为空");
        return;
    }
    if (period_ms < 2)
    {
        DEBUG_ERROR("LED_Breathe: 周期过短");
        return;
    }

    uint32_t N = (BREATHE_STEPS < 2) ? 2 : BREATHE_STEPS;
    uint32_t step_ms = period_ms / (2 * N);
    if (step_ms == 0)
        step_ms = 1;

    /* 上坡 0 -> max (N 步) */
    for (uint32_t i = 0; i < N; ++i)
    {
        float duty = (float)i / (float)(N - 1);
        uint32_t on_time = (uint32_t)(duty * step_ms);
        uint32_t off_time = step_ms - on_time;

        if (on_time)
        {
            LED_On(led);
            Delay_ms(on_time);
        }
        if (off_time)
        {
            LED_Off(led);
            Delay_ms(off_time);
        }
    }

    /* 下坡 max -> 0 (N 步) */
    for (int32_t i = (int32_t)N - 1; i >= 0; --i)
    {
        float duty = (float)i / (float)(N - 1);
        uint32_t on_time = (uint32_t)(duty * step_ms);
        uint32_t off_time = step_ms - on_time;

        if (on_time)
        {
            LED_On(led);
            Delay_ms(on_time);
        }
        if (off_time)
        {
            LED_Off(led);
            Delay_ms(off_time);
        }
    }
}

/**
 * @brief  所有LED同步呼吸一个周期（阻塞）
 * @param  period_ms: 呼吸总周期（毫秒）
 * @retval None
 */
void LED_Breathe_All(uint32_t period_ms)
{
    if (period_ms < 2)
    {
        DEBUG_ERROR("LED_Breathe_All: 周期过短");
        return;
    }

    uint32_t N = (BREATHE_STEPS < 2) ? 2 : BREATHE_STEPS;
    uint32_t step_ms = period_ms / (2 * N);
    if (step_ms == 0)
        step_ms = 1;

    /* 上坡 */
    for (uint32_t i = 0; i < N; ++i)
    {
        float duty = (float)i / (float)(N - 1);
        uint32_t on_time = (uint32_t)(duty * step_ms);
        uint32_t off_time = step_ms - on_time;

        if (on_time)
        {
            LED_On_All();
            Delay_ms(on_time);
        }
        if (off_time)
        {
            LED_Off_All();
            Delay_ms(off_time);
        }
    }

    /* 下坡 */
    for (int32_t i = (int32_t)N - 1; i >= 0; --i)
    {
        float duty = (float)i / (float)(N - 1);
        uint32_t on_time = (uint32_t)(duty * step_ms);
        uint32_t off_time = step_ms - on_time;

        if (on_time)
        {
            LED_On_All();
            Delay_ms(on_time);
        }
        if (off_time)
        {
            LED_Off_All();
            Delay_ms(off_time);
        }
    }
}

/**
 * @brief  流水灯效果（阻塞）
 * @param  step_ms: 单步停留时间（毫秒）
 * @note   依次点亮leds[]中的每个LED，完成一次遍历
 * @retval None
 */
void LED_ChaseStart(uint32_t step_ms)
{
    if (LED_COUNT <= 0)
    {
        DEBUG_ERROR("LED_ChaseStart: LED数量为0");
        return;
    }
    if (step_ms == 0)
    {
        DEBUG_ERROR("LED_ChaseStart: 步进时间为0");
        return;
    }

    for (int i = 0; i < LED_COUNT; i++)
    {
        LED_On(&leds[i]);
        Delay_ms(step_ms);
        LED_Off(&leds[i]);
    }
}

#endif // LED_ENABLE
