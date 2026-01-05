/**
 ******************************************************************************
 * @file    rtc_app.c
 * @author  菜菜why（B站：菜菜whyy）
 * @brief   RTC应用层实现 - 时间日期管理与闹钟设置
 ******************************************************************************
 */

#include "rtc_app.h"
#include <stdio.h>

#ifdef RTC_ENABLE

extern RTC_HandleTypeDef hrtc;

/* 静态变量保存闹钟配置，供中断回调使用 */
static RTC_AlarmMode_t g_AlarmMode = RTC_ALARM_MODE_DAILY;
static uint8_t g_AlarmVal = 0;

/* 静态变量保存闹钟B配置 */
static RTC_AlarmMode_t g_AlarmBMode = RTC_ALARM_MODE_DAILY;
static uint8_t g_AlarmBVal = 0;

/*******************************************************************************
 *                              初始化函数
 ******************************************************************************/

/**
 * @brief  RTC初始化
 * @note   配置LSE作为时钟源 (32.768kHz)
 * @retval 0: 成功, -1: 失败
 */
int RTC_App_Init(void)
{
    /* 1. 开启电源时钟和备份域访问权限 (必须先开启才能读写备份寄存器) */
    // __HAL_RCC_PWR_CLK_ENABLE();
#if defined(STM32F4)
    __HAL_RCC_PWR_CLK_ENABLE(); // F4 必须开启时钟
#endif
    HAL_PWR_EnableBkUpAccess();

    /* 2. 开启闹钟 A 的 NVIC 中断 */
    HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 0x01, 0);
    HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);

    /* 4. 检查是否是第一次配置 (利用备份寄存器实现断电计数) */
    // 如果读取值不是魔数 0x32F2，说明是首次上电或电池没电了
    if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0) != 0x32F2)
    {
        DEBUG_INFO("RTC首次上电或备份电源失效，设置默认时间...");

        // 设置默认的初始时间 (2026-01-01 12:00:00)
        RTC_SetTime(12, 0, 0);
        RTC_SetDate(26, 1, 1, 4);

        // 写入魔数标记，下次重启将跳过重置逻辑
        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, 0x32F2);
    }
    else
    {
        DEBUG_INFO("检测到备份域数据，RTC 继续走时中");
    }

    /* 5. 清除闹钟标志位，确保环境干净 */
    __HAL_RTC_ALARM_CLEAR_FLAG(&hrtc, RTC_FLAG_ALRAF);
    __HAL_RTC_ALARM_CLEAR_FLAG(&hrtc, RTC_FLAG_ALRBF);
    return 0;
}

/*******************************************************************************
 *                              功能函数
 ******************************************************************************/

/**
 * @brief  设置时间
 * @param  hour: 时 (0-23)
 * @param  min:  分 (0-59)
 * @param  sec:  秒 (0-59)
 * @retval 0: 成功, -1: 失败
 */
int RTC_SetTime(uint8_t hour, uint8_t min, uint8_t sec)
{
    RTC_TimeTypeDef sTime = {0};

    sTime.Hours = hour;
    sTime.Minutes = min;
    sTime.Seconds = sec;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;

    if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
    {
        DEBUG_ERROR("RTC设置时间失败");
        return -1;
    }
    return 0;
}

/**
 * @brief  设置日期
 * @param  year:  年 (0-99, 代表2000-2099)
 * @param  month: 月 (1-12)
 * @param  date:  日 (1-31)
 * @param  week:  星期 (1-7, 1=周一, 7=周日)
 * @retval 0: 成功, -1: 失败
 */
int RTC_SetDate(uint8_t year, uint8_t month, uint8_t date, uint8_t week)
{
    RTC_DateTypeDef sDate = {0};

    sDate.WeekDay = week;
    sDate.Month = month;
    sDate.Date = date;
    sDate.Year = year;

    if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
    {
        DEBUG_ERROR("RTC设置日期失败");
        return -1;
    }
    return 0;
}

/**
 * @brief  获取当前时间
 * @param  hour: 指针，返回时
 * @param  min:  指针，返回分
 * @param  sec:  指针，返回秒
 */
void RTC_GetTime(uint8_t *hour, uint8_t *min, uint8_t *sec)
{
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    /* 必须同时读取时间和日期以解锁影子寄存器，保证数据一致性 */
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    if (hour)
        *hour = sTime.Hours;
    if (min)
        *min = sTime.Minutes;
    if (sec)
        *sec = sTime.Seconds;
}

/**
 * @brief  获取当前日期
 * @param  year:  指针，返回年
 * @param  month: 指针，返回月
 * @param  date:  指针，返回日
 * @param  week:  指针，返回星期
 */
void RTC_GetDate(uint8_t *year, uint8_t *month, uint8_t *date, uint8_t *week)
{
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    /* 必须同时读取时间和日期以解锁影子寄存器 */
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    if (year)
        *year = sDate.Year;
    if (month)
        *month = sDate.Month;
    if (date)
        *date = sDate.Date;
    if (week)
        *week = sDate.WeekDay;
}

/**
 * @brief  设置闹钟 A (支持复杂场景)
 * @param  mode: 触发模式 (见 RTC_AlarmMode_t)
 * @param  val:  参数值，取决于 mode:
 *               - ONCE/DAILY/WORKDAY/WEEKEND: 填 0 即可
 *               - WEEKLY_MASK: 星期掩码 (如 RTC_WEEK_MONDAY | RTC_WEEK_FRIDAY)
 *               - MONTHLY: 日期 (1-31)
 * @param  hour: 时
 * @param  min:  分
 * @param  sec:  秒
 * @retval 0: 成功, -1: 失败
 */
int RTC_SetAlarmA(RTC_AlarmMode_t mode, uint8_t val, uint8_t hour, uint8_t min, uint8_t sec)
{
    RTC_AlarmTypeDef sAlarm = {0};

    /* 保存配置供中断处理使用 */
    g_AlarmMode = mode;
    g_AlarmVal = val;

    sAlarm.AlarmTime.Hours = hour;
    sAlarm.AlarmTime.Minutes = min;
    sAlarm.AlarmTime.Seconds = sec;
    sAlarm.AlarmTime.SubSeconds = 0;
    sAlarm.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sAlarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;
    sAlarm.Alarm = RTC_ALARM_A;
    sAlarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;

    /* 硬件配置策略：
     * 1. MONTHLY 模式：使用硬件日期匹配
     * 2. 其他所有模式：配置为“每天匹配”，在中断里软件过滤
     */
    if (mode == RTC_ALARM_MODE_MONTHLY)
    {
        sAlarm.AlarmMask = RTC_ALARMMASK_NONE; // 匹配日期、时、分、秒
        sAlarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
        sAlarm.AlarmDateWeekDay = val; // 日期 1-31
        DEBUG_INFO("闹钟A设置: 每月%d日 %02d:%02d:%02d", val, hour, min, sec);
    }
    else
    {
        /* 每天触发，具体逻辑在回调中判断 */
        sAlarm.AlarmMask = RTC_ALARMMASK_DATEWEEKDAY; // 忽略日期/星期，只匹配时分秒
        sAlarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
        sAlarm.AlarmDateWeekDay = 1; // 占位

        const char *mode_str = "";
        switch (mode)
        {
        case RTC_ALARM_MODE_ONCE:
            mode_str = "一次性";
            break;
        case RTC_ALARM_MODE_DAILY:
            mode_str = "每天";
            break;
        case RTC_ALARM_MODE_WEEKLY_MASK:
            mode_str = "特定星期";
            break;
        case RTC_ALARM_MODE_WORKDAY:
            mode_str = "工作日";
            break;
        case RTC_ALARM_MODE_WEEKEND:
            mode_str = "周末";
            break;
        default:
            break;
        }
        DEBUG_INFO("闹钟A设置: [%s] %02d:%02d:%02d", mode_str, hour, min, sec);
    }

    if (HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BIN) != HAL_OK)
    {
        DEBUG_ERROR("RTC闹钟设置失败");
        return -1;
    }

    return 0;
}

/**
 * @brief  设置闹钟 B (支持复杂场景)
 * @param  mode: 触发模式 (见 RTC_AlarmMode_t)
 * @param  val:  参数值，取决于 mode:
 *               - ONCE/DAILY/WORKDAY/WEEKEND: 填 0 即可
 *               - WEEKLY_MASK: 星期掩码 (如 RTC_WEEK_MONDAY | RTC_WEEK_FRIDAY)
 *               - MONTHLY: 日期 (1-31)
 * @param  hour: 时
 * @param  min:  分
 * @param  sec:  秒
 * @retval 0: 成功, -1: 失败
 */
int RTC_SetAlarmB(RTC_AlarmMode_t mode, uint8_t val, uint8_t hour, uint8_t min, uint8_t sec)
{
    RTC_AlarmTypeDef sAlarm = {0};

    /* 保存配置供中断处理使用 */
    g_AlarmBMode = mode;
    g_AlarmBVal = val;

    sAlarm.AlarmTime.Hours = hour;
    sAlarm.AlarmTime.Minutes = min;
    sAlarm.AlarmTime.Seconds = sec;
    sAlarm.AlarmTime.SubSeconds = 0;
    sAlarm.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sAlarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;
    sAlarm.Alarm = RTC_ALARM_B;
    sAlarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;

    /* 硬件配置策略：
     * 1. MONTHLY 模式：使用硬件日期匹配
     * 2. 其他所有模式：配置为“每天匹配”，在中断里软件过滤
     */
    if (mode == RTC_ALARM_MODE_MONTHLY)
    {
        sAlarm.AlarmMask = RTC_ALARMMASK_NONE; // 匹配日期、时、分、秒
        sAlarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
        sAlarm.AlarmDateWeekDay = val; // 日期 1-31
        DEBUG_INFO("闹钟B设置: 每月%d日 %02d:%02d:%02d", val, hour, min, sec);
    }
    else
    {
        /* 每天触发，具体逻辑在回调中判断 */
        sAlarm.AlarmMask = RTC_ALARMMASK_DATEWEEKDAY; // 忽略日期/星期，只匹配时分秒
        sAlarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
        sAlarm.AlarmDateWeekDay = 1; // 占位

        const char *mode_str = "";
        switch (mode)
        {
        case RTC_ALARM_MODE_ONCE:
            mode_str = "一次性";
            break;
        case RTC_ALARM_MODE_DAILY:
            mode_str = "每天";
            break;
        case RTC_ALARM_MODE_WEEKLY_MASK:
            mode_str = "特定星期";
            break;
        case RTC_ALARM_MODE_WORKDAY:
            mode_str = "工作日";
            break;
        case RTC_ALARM_MODE_WEEKEND:
            mode_str = "周末";
            break;
        default:
            break;
        }
        DEBUG_INFO("闹钟B设置: [%s] %02d:%02d:%02d", mode_str, hour, min, sec);
    }

    if (HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BIN) != HAL_OK)
    {
        DEBUG_ERROR("RTC闹钟B设置失败");
        return -1;
    }

    return 0;
}

/**
 * @brief  闹钟中断回调函数
 */
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
    RTC_DateTypeDef sDate;
    uint8_t current_week;
    uint8_t should_trigger = 0;

    /* 获取当前日期（用于判断星期） */
    HAL_RTC_GetDate(hrtc, &sDate, RTC_FORMAT_BIN);
    current_week = sDate.WeekDay; // 1=周一 ... 7=周日

    /* 根据模式判断是否触发 */
    switch (g_AlarmMode)
    {
    case RTC_ALARM_MODE_ONCE:
        should_trigger = 1;
        /* 一次性闹钟：触发后关闭 */
        HAL_RTC_DeactivateAlarm(hrtc, RTC_ALARM_A);
        DEBUG_INFO("一次性闹钟已执行并关闭");
        break;

    case RTC_ALARM_MODE_DAILY:
    case RTC_ALARM_MODE_MONTHLY: // 硬件已过滤，能进中断肯定是对的
        should_trigger = 1;
        break;

    case RTC_ALARM_MODE_WEEKLY_MASK:
        /* 检查当前星期是否在掩码中 */
        if (g_AlarmVal & (1 << current_week))
        {
            should_trigger = 1;
        }
        break;

    case RTC_ALARM_MODE_WORKDAY:
        /* 周一(1) 到 周五(5) */
        if (current_week >= 1 && current_week <= 5)
        {
            should_trigger = 1;
        }
        break;

    case RTC_ALARM_MODE_WEEKEND:
        /* 周六(6) 或 周日(7) */
        if (current_week >= 6 && current_week <= 7)
        {
            should_trigger = 1;
        }
        break;
    }

    if (should_trigger)
    {
        DEBUG_INFO("闹钟A触发! (当前:周%d)", current_week);
        /* TODO: 在此处添加你的业务逻辑，如播放音乐、LED闪烁等 */
    }
    else
    {
        /* 调试用：虽然硬件触发了，但软件过滤掉了 */
        // DEBUG_INFO("闹钟A硬件触发，但条件不符 (当前:周%d, 模式:%d)", current_week, g_AlarmMode);
    }
}

/**
 * @brief  闹钟B中断回调函数
 * @note   当闹钟时间到达时，HAL库会自动调用此函数
 */
void HAL_RTCEx_AlarmBEventCallback(RTC_HandleTypeDef *hrtc)
{
    RTC_DateTypeDef sDate;
    uint8_t current_week;
    uint8_t should_trigger = 0;

    /* 获取当前日期（用于判断星期） */
    HAL_RTC_GetDate(hrtc, &sDate, RTC_FORMAT_BIN);
    current_week = sDate.WeekDay; // 1=周一 ... 7=周日

    /* 根据模式判断是否触发 */
    switch (g_AlarmBMode)
    {
    case RTC_ALARM_MODE_ONCE:
        should_trigger = 1;
        /* 一次性闹钟：触发后关闭 */
        HAL_RTC_DeactivateAlarm(hrtc, RTC_ALARM_B);
        DEBUG_INFO("一次性闹钟B已执行并关闭");
        break;

    case RTC_ALARM_MODE_DAILY:
    case RTC_ALARM_MODE_MONTHLY: // 硬件已过滤，能进中断肯定是对的
        should_trigger = 1;
        break;

    case RTC_ALARM_MODE_WEEKLY_MASK:
        /* 检查当前星期是否在掩码中 */
        if (g_AlarmBVal & (1 << current_week))
        {
            should_trigger = 1;
        }
        break;

    case RTC_ALARM_MODE_WORKDAY:
        /* 周一(1) 到 周五(5) */
        if (current_week >= 1 && current_week <= 5)
        {
            should_trigger = 1;
        }
        break;

    case RTC_ALARM_MODE_WEEKEND:
        /* 周六(6) 或 周日(7) */
        if (current_week >= 6 && current_week <= 7)
        {
            should_trigger = 1;
        }
        break;
    }

    if (should_trigger)
    {
        DEBUG_INFO("闹钟B触发! (当前:周%d)", current_week);
        /* TODO: 在此处添加你的业务逻辑 */
    }
}

/**
 * @brief  RTC全局中断处理函数
 * @note   请确保在 stm32h7xx_it.c 中没有重复定义，或者将此函数移动到中断文件中
 */
void RTC_Alarm_IRQHandler(void)
{
    HAL_RTC_AlarmIRQHandler(&hrtc);
}

#endif /* RTC_ENABLE */
