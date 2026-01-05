/**
 ******************************************************************************
 * @file    rtc_app.h
 * @author  菜菜why（B站：菜菜whyy）
 * @brief   RTC应用层头文件 - 时间日期管理与闹钟设置
 ******************************************************************************
 */

#ifndef __RTC_APP_H
#define __RTC_APP_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "config.h"

#ifdef RTC_ENABLE

    extern RTC_HandleTypeDef hrtc;

    /**
     * @brief 闹钟触发模式枚举
     */
    typedef enum
    {
        RTC_ALARM_MODE_ONCE = 0,    /*!< 一次性 (响完自动关闭) */
        RTC_ALARM_MODE_DAILY,       /*!< 每天响 */
        RTC_ALARM_MODE_WEEKLY_MASK, /*!< 自定义周几 (支持多选，如周三+周六) */
        RTC_ALARM_MODE_WORKDAY,     /*!< 工作日 (周一至周五) */
        RTC_ALARM_MODE_WEEKEND,     /*!< 周末 (周六、周日) */
        RTC_ALARM_MODE_MONTHLY      /*!< 每月某日 */
    } RTC_AlarmMode_t;

/* 星期掩码定义 (用于 RTC_ALARM_MODE_WEEKLY_MASK) */
#define RTC_WEEK_MONDAY (1 << 1)
#define RTC_WEEK_TUESDAY (1 << 2)
#define RTC_WEEK_WEDNESDAY (1 << 3)
#define RTC_WEEK_THURSDAY (1 << 4)
#define RTC_WEEK_FRIDAY (1 << 5)
#define RTC_WEEK_SATURDAY (1 << 6)
#define RTC_WEEK_SUNDAY (1 << 7)

    /**
     * @brief  RTC初始化
     * @retval 0: 成功, -1: 失败
     */
    int RTC_App_Init(void);

    /**
     * @brief  设置时间
     * @param  hour: 时 (0-23)
     * @param  min:  分 (0-59)
     * @param  sec:  秒 (0-59)
     * @retval 0: 成功, -1: 失败
     * @note   示例: RTC_SetTime(12, 30, 0);
     */
    int RTC_SetTime(uint8_t hour, uint8_t min, uint8_t sec);

    /**
     * @brief  设置日期
     * @param  year:  年 (0-99, 代表2000-2099)
     * @param  month: 月 (1-12)
     * @param  date:  日 (1-31)
     * @param  week:  星期 (1-7, 1=周一, 7=周日)
     * @retval 0: 成功, -1: 失败
     * @note   示例: RTC_SetDate(25, 12, 31, 3); // 2025年12月31日 星期三
     */
    int RTC_SetDate(uint8_t year, uint8_t month, uint8_t date, uint8_t week);

    /**
     * @brief  获取当前时间
     * @param  hour: 指针，返回时
     * @param  min:  指针，返回分
     * @param  sec:  指针，返回秒
     * @retval None
     */
    void RTC_GetTime(uint8_t *hour, uint8_t *min, uint8_t *sec);

    /**
     * @brief  获取当前日期
     * @param  year:  指针，返回年
     * @param  month: 指针，返回月
     * @param  date:  指针，返回日
     * @param  week:  指针，返回星期
     * @retval None
     */
    void RTC_GetDate(uint8_t *year, uint8_t *month, uint8_t *date, uint8_t *week);

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
    int RTC_SetAlarmA(RTC_AlarmMode_t mode, uint8_t val, uint8_t hour, uint8_t min, uint8_t sec);

    /**
     * @brief  设置闹钟 B (支持复杂场景)
     * @param  mode: 触发模式 (见 RTC_AlarmMode_t)
     * @param  val:  参数值，取决于 mode
     * @param  hour: 时
     * @param  min:  分
     * @param  sec:  秒
     * @retval 0: 成功, -1: 失败
     */
    int RTC_SetAlarmB(RTC_AlarmMode_t mode, uint8_t val, uint8_t hour, uint8_t min, uint8_t sec);

#endif /* RTC_ENABLE */

#ifdef __cplusplus
}
#endif

#endif // __RTC_APP_H
