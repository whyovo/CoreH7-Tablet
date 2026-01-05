/**
 ******************************************************************************
 * @file    ui_encoder.h
 * @author  菜菜why(B站:菜菜whyy)
 * @brief   UI 旋转编码器（梅花柄）轮询驱动头文件
 ******************************************************************************
 * @attention
 *
 * 使用方法:
 * - 纯轮询实现，调用 UI_ENCODER_Poll() 即可完成旋转与按键事件检测。
 * - 建议在 1ms 定时器中调用 UI_ENCODER_Poll()。
 *
 * 使用示例:
    void UI_ENCODER_EventHandler(UI_ENCODER_Event ev)
    {
        switch (ev) {
            case UI_ENCODER_EV_ROTATE_LEFT:
                // 下拉UI菜单
                break;
            case UI_ENCODER_EV_ROTATE_RIGHT:
                // 上拉UI菜单
                break;
            case UI_ENCODER_EV_KEY_CLICK:
                // 确认选择
                break;
            case UI_ENCODER_EV_KEY_LONG_PRESS:
                // 退出菜单
                break;
        }
    }
 ******************************************************************************
 */

#ifndef UI_ENCODER_H
#define UI_ENCODER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "config.h"

/*******************************************************************************
 *                              引脚配置
 ******************************************************************************/
#define UI_ENCODER_A_PORT GPIOA       /*!< A相GPIO端口 */
#define UI_ENCODER_A_PIN GPIO_PIN_0   /*!< A相引脚(中断触发) */
#define UI_ENCODER_B_PORT GPIOA       /*!< B相GPIO端口 */
#define UI_ENCODER_B_PIN GPIO_PIN_1   /*!< B相引脚(方向判断) */
#define UI_ENCODER_KEY_PORT GPIOA     /*!< 按键GPIO端口 */
#define UI_ENCODER_KEY_PIN GPIO_PIN_9 /*!< 按键引脚 */

/*******************************************************************************
 *                              参数配置
 ******************************************************************************/
#define UI_ENCODER_DEBOUNCE_MS 20 /*!< 按键消抖时间(毫秒) */
#define UI_ENCODER_LONG_MS 600    /*!< 长按触发时间(毫秒) */
#define UI_ENCODER_DBL_MS 200     /*!< 双击最大间隔时间(毫秒) */

    /*******************************************************************************
     *                              导出类型
     ******************************************************************************/

    /**
     * @brief  UI编码器事件类型
     */
    typedef enum
    {
        UI_ENCODER_EV_ROTATE_LEFT = 0,  /*!< 左转事件(下拉UI) */
        UI_ENCODER_EV_ROTATE_RIGHT,     /*!< 右转事件(上拉UI) */
        UI_ENCODER_EV_KEY_PRESS,        /*!< 按键按下事件 */
        UI_ENCODER_EV_KEY_RELEASE,      /*!< 按键释放事件 */
        UI_ENCODER_EV_KEY_CLICK,        /*!< 按键单击事件 */
        UI_ENCODER_EV_KEY_DOUBLE_CLICK, /*!< 按键双击事件 */
        UI_ENCODER_EV_KEY_LONG_PRESS    /*!< 按键长按事件 */
    } UI_ENCODER_Event;

    /**
     * @brief  UI编码器回调函数原型
     * @param  ev: 事件类型
     */
    typedef void (*UI_ENCODER_Callback)(UI_ENCODER_Event ev);

    /*******************************************************************************
     *                              导出函数
     ******************************************************************************/

    /**
     * @brief  UI编码器事件处理函数(弱定义,用户可重定义)
     * @param  ev: 事件类型
     * @note   用户在任意C文件中重新实现此函数即可接管所有事件
     * @retval None
     */
    void UI_ENCODER_EventHandler(UI_ENCODER_Event ev);

    /**
     * @brief  初始化UI编码器
     * @retval None
     */
    void UI_ENCODER_Init(void);

    /**
     * @brief  轮询函数（必须周期调用，建议 1ms）
     * @note   包含：旋转检测（A 相边沿）与按键去抖/单击/双击/长按逻辑
     * @retval None
     */
    void UI_ENCODER_Poll(void);

    /**
     * @brief  注册事件回调函数
     * @param  cb: 回调函数指针
     * @note   注册后事件将调用回调而非UI_ENCODER_EventHandler
     * @retval None
     */
    void UI_ENCODER_RegisterCallback(UI_ENCODER_Callback cb);

    /**
     * @brief  取消回调注册
     * @note   取消后事件将回到调用UI_ENCODER_EventHandler
     * @retval None
     */
    void UI_ENCODER_UnregisterCallback(void);

#ifdef __cplusplus
}
#endif

#endif // UI_ENCODER_H

