/**
 ******************************************************************************
 * @file    key.h
 * @author  菜菜why(B站:菜菜whyy)
 * @brief   按键驱动头文件
 ******************************************************************************
 * @attention
 *
 * 使用方法:
 * 1. 修改 KEY_LIST 配置按键列表(端口、引脚)
 *    - name: 枚举名称(如 KEY1)
 *    - port: GPIO端口(如 GPIOA)
 *    - pin: GPIO引脚(如 GPIO_PIN_9)
 * 2. 初始化时自动检测按键极性(默认释放状态)
 * 3. 在主循环或者定时器中断中周期调用 KEY_Task()
 * 4. 实现 KEY_EventHandler() 或使用回调注册
 *
 * 示例配置:
 * #define KEY_LIST \
 *     X(KEY1, GPIOA, GPIO_PIN_9) \
 *     X(KEY2, GPIOB, GPIO_PIN_8)
 *
 * 事件处理示例：
 * void KEY_EventHandler(KEY_ID id, KEY_Event ev) {
 *     if (id == KEY1) {
 *         switch(ev) {
 *             case KEY_EV_CLICK: LED_Toggle(&leds[0]); break;
 *             case KEY_EV_LONG_PRESS: LED_On_All(); break;
 *         }
 *     }
 * }
 *
 ******************************************************************************
 */

#ifndef KEY_H
#define KEY_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "init.h"

/*******************************************************************************
 *                              按键配置列表
 *******************************************************************************/
/**
 * @brief 在此处定义所有按键(X-macro形式)(name, port, pin)
 */
#define KEY_LIST \
    X(KEY1, GPIOA, GPIO_PIN_9)

    /*******************************************************************************
     *                              按键导出类型
     ******************************************************************************/

    /**
     * @brief  按键结构体定义
     */
    typedef struct
    {
        GPIO_TypeDef *port; /*!< GPIO端口基地址 */
        uint16_t pin;       /*!< GPIO引脚编号 */
        uint8_t idle_level; /*!< 空闲电平(初始化时自动检测): 0=低电平, 1=高电平 */
    } KEY;

/**
 * @brief  按键枚举ID(自动生成)
 */
#define X(name, port, pin) name,
    typedef enum
    {
        KEY_LIST
            KEY_COUNT /*!< 按键总数 */
    } KEY_ID;
#undef X

    /**
     * @brief  按键事件类型
     */
    typedef enum
    {
        KEY_EV_PRESS = 0,    /*!< 按下事件 */
        KEY_EV_RELEASE,      /*!< 释放事件 */
        KEY_EV_CLICK,        /*!< 单击事件 */
        KEY_EV_DOUBLE_CLICK, /*!< 双击事件 */
        KEY_EV_LONG_PRESS    /*!< 长按事件 */
    } KEY_Event;

    /**
     * @brief  按键回调函数原型
     * @param  id: 按键ID（KEY_ID枚举值）
     * @param  ev: 事件类型（KEY_Event枚举值）
     */
    typedef void (*KEY_Callback)(KEY_ID id, KEY_Event ev);

    /*******************************************************************************
     *                              按键数组
     ******************************************************************************/
    extern KEY keys[]; /*!< 按键数组，在key.c中根据KEY_LIST自动生成 */

    /*******************************************************************************
     *                              按键导出函数
     ******************************************************************************/

    /**
     * @brief  按键事件处理函数(弱定义,用户可重定义)
     * @param  id: 按键ID
     * @param  ev: 事件类型
     * @retval None
     */
    void KEY_EventHandler(KEY_ID id, KEY_Event ev);

    /*******************************************************************************
     *                              核心功能函数
     ******************************************************************************/

    /**
     * @brief  初始化所有按键
     * @note   清空状态数组，读取初始电平
     * @retval None
     */
    void KEY_Init(void);

    /**
     * @brief  按键扫描任务（非阻塞）
     * @note   必须在主循环或定时器中周期调用，建议间隔5~20ms
     * @note   内部实现消抖、边沿检测、事件识别
     * @retval None
     */
    void KEY_Task(void);

    /*******************************************************************************
     *                              辅助功能函数
     ******************************************************************************/

    /**
     * @brief  读取按键原始GPIO电平
     * @param  key: 按键指针
     * @retval 0=低电平，1=高电平
     */
    uint8_t KEY_ReadRaw(KEY *key);

    /**
     * @brief  判断按键是否按下（考虑极性）
     * @param  key: 按键指针
     * @retval 0=未按下，1=按下
     */
    uint8_t KEY_IsPressed(KEY *key);

    /*******************************************************************************
     *                              回调注册函数
     ******************************************************************************/

    /**
     * @brief  为指定按键注册回调函数
     * @param  id: 按键ID
     * @param  cb: 回调函数指针
     * @note   注册后该按键的事件将调用回调而非KEY_EventHandler
     * @retval None
     */
    void KEY_RegisterCallback(KEY_ID id, KEY_Callback cb);

    /**
     * @brief  取消指定按键的回调注册
     * @param  id: 按键ID
     * @note   取消后该按键的事件将回到调用KEY_EventHandler
     * @retval None
     */
    void KEY_UnregisterCallback(KEY_ID id);

#ifdef __cplusplus
}
#endif

#endif // !KEY_H
