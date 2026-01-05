/**
 ******************************************************************************
 * @file    digital_output_sensor.h
 * @author  菜菜why（B站：菜菜whyy）
 * @brief   数字输出传感器驱动头文件，基于X-macro自动生成传感器数组与枚举
 *          支持红外、人体感应、超声波等数字输出传感器
 ******************************************************************************
 * @attention
 *
 * 使用方法：
 * 1. 修改 DIGITAL_SENSOR_LIST 宏定义以配置传感器列表（端口、引脚、极性）
 *    - name: 枚举名称（如 IR_SENSOR1）
 *    - port: GPIO端口（如 GPIOA）
 *    - pin: GPIO引脚（如 GPIO_PIN_0）
 *    - direct: 触发极性（1=高电平触发，0=低电平触发）
 * 2. 编译后自动生成 digital_sensors[] 数组和 DIGITAL_SENSOR_ID 枚举
 * 3. 基本读取：DIGITAL_SENSOR_Read / DIGITAL_SENSOR_IsTriggered
 * 4. 事件检测：在主循环调用 DIGITAL_SENSOR_Task() + 实现回调处理边沿事件
 *
 * 示例配置：
 * #define DIGITAL_SENSOR_LIST \
 *     X(IR_SENSOR1, GPIOA, GPIO_PIN_0, 1)  \  // 红外传感器，高电平触发
 *     X(PIR_SENSOR, GPIOA, GPIO_PIN_1, 1)  \  // 人体感应，高电平触发
 *     X(ULTRASONIC_ECHO, GPIOA, GPIO_PIN_2, 1) // 超声波回响，高电平有效
 *
 * 事件处理示例：
 * void DIGITAL_SENSOR_EventHandler(DIGITAL_SENSOR_ID id, DIGITAL_SENSOR_Event
 *ev) { if (id == IR_SENSOR1) { switch(ev) { case SENSOR_EV_TRIGGERED:
 *LED_On(&leds[0]); break; case SENSOR_EV_RELEASED: LED_Off(&leds[0]); break;
 *         }
 *     }
 * }
 *
 ******************************************************************************
 */

#ifndef DIGITAL_OUTPUT_SENSOR_H
#define DIGITAL_OUTPUT_SENSOR_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "config.h"

/*******************************************************************************
 *                              传感器配置列表
 ******************************************************************************/
/**
 * @brief 在此处定义所有数字输出传感器（X-macro形式）：X（name, port, pin,
 * direct）
 */
#define DIGITAL_SENSOR_LIST X(IR_SENSOR1, GPIOA, GPIO_PIN_0, 1)

  /*******************************************************************************
   *                              传感器导出类型
   ******************************************************************************/

  /**
   * @brief  数字传感器结构体定义
   */
  typedef struct
  {
    GPIO_TypeDef *port; /*!< GPIO端口基地址 */
    uint16_t pin;       /*!< GPIO引脚编号 */
    uint8_t direct;     /*!< 触发极性：1=高电平触发，0=低电平触发 */
  } DIGITAL_SENSOR;

/**
 * @brief  数字传感器枚举ID（自动生成）
 */
#define X(name, port, pin, direct) name,
  typedef enum
  {
    DIGITAL_SENSOR_LIST DIGITAL_SENSOR_COUNT /*!< 传感器总数（用于遍历） */
  } DIGITAL_SENSOR_ID;
#undef X

  /**
   * @brief  传感器事件类型
   */
  typedef enum
  {
    SENSOR_EV_TRIGGERED = 0, /*!< 触发事件（上升沿或下降沿，取决于极性） */
    SENSOR_EV_RELEASED       /*!< 释放事件（对应的反向边沿） */
  } DIGITAL_SENSOR_Event;

  /**
   * @brief  传感器回调函数原型
   * @param  id: 传感器ID
   * @param  ev: 事件类型
   */
  typedef void (*DIGITAL_SENSOR_Callback)(DIGITAL_SENSOR_ID id,
                                          DIGITAL_SENSOR_Event ev);

  /*******************************************************************************
   *                              传感器数组
   ******************************************************************************/
  extern DIGITAL_SENSOR digital_sensors[]; /*!<
                                              传感器数组，在digital_output_sensor.c中根据DIGITAL_SENSOR_LIST自动生成
                                            */

  /**
   * @brief  获取传感器结构体指针
   * @param  id: 传感器ID
   * @retval 传感器指针，如果ID无效返回NULL
   */
  DIGITAL_SENSOR *DIGITAL_SENSOR_GetPtr(DIGITAL_SENSOR_ID id);

  /*******************************************************************************
   *                              传感器导出函数
   ******************************************************************************/

  /**
   * @brief  传感器事件处理函数（弱定义，用户可重定义）
   * @param  id: 传感器ID
   * @param  ev: 事件类型
   * @note   用户在任意C文件中重新实现此函数即可接管所有传感器事件
   * @note   若使用回调注册，已注册的传感器不会调用此函数
   * @retval None
   */
  void DIGITAL_SENSOR_EventHandler(DIGITAL_SENSOR_ID id, DIGITAL_SENSOR_Event ev);

  /*******************************************************************************
   *                              核心功能函数
   ******************************************************************************/

  /**
   * @brief  初始化所有传感器
   * @note   清空状态数组，读取初始电平
   * @retval None
   */
  void DIGITAL_SENSOR_Init(void);

  /**
   * @brief  传感器扫描任务（非阻塞）
   * @note   必须在主循环或定时器中周期调用，建议间隔5~50ms
   * @note   内部实现消抖、边沿检测、事件识别
   * @retval None
   */
  void DIGITAL_SENSOR_Task(void);

  /*******************************************************************************
   *                              辅助功能函数
   ******************************************************************************/

  /* ID 访问接口（推荐） */

  /**
   * @brief  读取传感器原始GPIO电平（ID版本）
   * @param  id: 传感器ID
   * @retval 0=低电平，1=高电平
   */
  uint8_t DIGITAL_SENSOR_ReadRaw(DIGITAL_SENSOR_ID id);

  /**
   * @brief  判断传感器是否触发（ID版本，考虑极性）
   * @param  id: 传感器ID
   * @retval 0=未触发，1=触发
   */
  uint8_t DIGITAL_SENSOR_IsTriggered(DIGITAL_SENSOR_ID id);

  /* 指针访问接口（底层/高级用法） */

  /**
   * @brief  读取传感器原始GPIO电平（指针版本）
   * @param  sensor: 传感器指针
   * @retval 0=低电平，1=高电平
   */
  uint8_t DIGITAL_SENSOR_ReadRaw_Ptr(DIGITAL_SENSOR *sensor);

  /**
   * @brief  判断传感器是否触发（指针版本，考虑极性）
   * @param  sensor: 传感器指针
   * @retval 0=未触发，1=触发
   */
  uint8_t DIGITAL_SENSOR_IsTriggered_Ptr(DIGITAL_SENSOR *sensor);

  /*******************************************************************************
   *                              回调注册函数
   ******************************************************************************/

  /**
   * @brief  为指定传感器注册回调函数
   * @param  id: 传感器ID
   * @param  cb: 回调函数指针
   * @note   注册后该传感器的事件将调用回调而非DIGITAL_SENSOR_EventHandler
   * @retval None
   */
  void DIGITAL_SENSOR_RegisterCallback(DIGITAL_SENSOR_ID id,
                                       DIGITAL_SENSOR_Callback cb);

  /**
   * @brief  取消指定传感器的回调注册
   * @param  id: 传感器ID
   * @note   取消后该传感器的事件将回到调用DIGITAL_SENSOR_EventHandler
   * @retval None
   */
  void DIGITAL_SENSOR_UnregisterCallback(DIGITAL_SENSOR_ID id);

#ifdef __cplusplus
}
#endif

#endif // !DIGITAL_OUTPUT_SENSOR_H
