/**
 ******************************************************************************
 * @file    led.h
 * @author  菜菜why（B站：菜菜whyy）
 * @brief   LED驱动头文件，基于X-macro自动生成LED数组与枚举
 ******************************************************************************
 * @attention
 *
 * 使用方法：
 * 1. 修改 LED_LIST 宏定义以配置LED列表（端口、引脚、极性）
 *    - name: 枚举名称（如 LD1）
 *    - port: GPIO端口（如 GPIOC）
 *    - pin: GPIO引脚（如 GPIO_PIN_13）
 *    - direct: 点亮极性（1=高电平亮，0=低电平亮）
 * 2. 编译后自动生成 leds[] 数组和 LED_ID 枚举
 * 3. 基本控制：LED_On/Off/Toggle(&leds[LD1])
 * 4. 动画效果（阻塞）：LED_Blink / LED_Breathe / LED_ChaseStart
 *
 * 示例配置：
 * #define LED_LIST \
 *     X(LD1, GPIOC, GPIO_PIN_13, 0) \
 *     X(LD2, GPIOB, GPIO_PIN_0, 1)
 *
 ******************************************************************************
 */

#ifndef LED_H
#define LED_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#ifdef LED_ENABLE
/*******************************************************************************
 *                              LED配置列表
 *******************************************************************************/
/**
 * @brief 在此处定义所有LED（X-macro形式）：X（name, port, pin, direct）
 */
#define LED_LIST X(LD1, GPIOC, GPIO_PIN_13, 0)

/*******************************************************************************
 *                              LED导出类型
 *******************************************************************************/

/**
 * @brief  LED结构体定义
 */
typedef struct {
  GPIO_TypeDef *port; /*!< GPIO端口基地址 */
  uint16_t pin;       /*!< GPIO引脚编号 */
  uint8_t direct;     /*!< 点亮极性：1=高电平亮，0=低电平亮 */
} LED;

/**
 * @brief  LED枚举ID（自动生成）
 */
#define X(name, port, pin, direct) name,
typedef enum {
  LED_LIST LED_COUNT /*!< LED总数（用于遍历） */
} LED_ID;
#undef X

/*******************************************************************************
 *                              LED数组
 *******************************************************************************/
extern LED leds[]; /*!< LED数组，在led.c中根据LED_LIST自动生成 */

/*******************************************************************************
 *                              基本控制函数
 *******************************************************************************/
void LED_Init(void);
void LED_On(LED *led);
void LED_Off(LED *led);
void LED_Toggle(LED *led);

void LED_On_All(void);
void LED_Off_All(void);
void LED_Toggle_All(void);

/*******************************************************************************
 *                              动画效果函数（阻塞实现）
 *******************************************************************************/

/**
 * @brief  单个LED闪烁（阻塞）
 * @param  led: LED指针
 * @param  period_ms: 闪烁周期（毫秒），灯在半周期切换
 * @note   一次调用完成一个完整周期（on -> off），需在循环中反复调用
 * @retval None
 */
void LED_Blink(LED *led, uint32_t period_ms);

/**
 * @brief  所有LED同步闪烁（阻塞）
 * @param  period_ms: 闪烁周期（毫秒）
 * @retval None
 */
void LED_Blink_All(uint32_t period_ms);

/**
 * @brief  单个LED呼吸效果（阻塞）
 * @param  led: LED指针
 * @param  period_ms: 呼吸总周期（毫秒），从暗到亮再到暗
 * @note   使用软件PWM实现渐变，分100步完成一个周期
 * @retval None
 */
void LED_Breathe(LED *led, uint32_t period_ms);

/**
 * @brief  所有LED同步呼吸（阻塞）
 * @param  period_ms: 呼吸总周期（毫秒）
 * @retval None
 */
void LED_Breathe_All(uint32_t period_ms);

/**
 * @brief  流水灯效果（阻塞）
 * @param  step_ms: 单步停留时间（毫秒）
 * @note   依次点亮leds[]数组中的每个LED，完成一次遍历后返回
 * @retval None
 */
void LED_ChaseStart(uint32_t step_ms);
#endif
#ifdef __cplusplus
}
#endif

#endif // !LED_H
