/**
 ******************************************************************************
 * @file    digital_output_sensor.c
 * @author  菜菜why（B站：菜菜whyy）
 * @brief   数字输出传感器驱动实现文件
 ******************************************************************************
 * @attention
 *
 * 本文件实现：
 * - 基于X-macro自动生成digital_sensors[]数组
 * - 软件消抖（可配置消抖时间）
 * - 事件检测：触发/释放边沿检测
 * - 弱符号事件处理函数，支持HAL风格回调
 *
 * 技术要点：
 * - 状态机驱动：在DIGITAL_SENSOR_Task中处理去抖和事件识别
 * - 时间戳管理：记录电平变化时间，实现消抖算法
 * - 边沿检测：检测触发和释放边沿，触发相应事件
 *
 ******************************************************************************
 */

#include "digital_output_sensor.h"

#ifdef DIGITAL_SENSOR_ENABLE

/*******************************************************************************
 *                              私有配置参数
 ******************************************************************************/
/**
 * @brief 可根据实际传感器特性调整以下参数
 */
#define SENSOR_DEBOUNCE_MS \
  50 /*!< 消抖时间（毫秒），传感器抖动较大，建议30~100ms */

/*******************************************************************************
 *                              私有状态数组
 ******************************************************************************/

/**
 * @brief  传感器数组（根据DIGITAL_SENSOR_LIST自动生成）
 */
#define X(name, port, pin, direct) {port, pin, direct},
DIGITAL_SENSOR digital_sensors[] = {DIGITAL_SENSOR_LIST};
#undef X

/**
 * @brief  获取传感器结构体指针
 * @param  id: 传感器ID
 * @retval 传感器指针
 */
DIGITAL_SENSOR *DIGITAL_SENSOR_GetPtr(DIGITAL_SENSOR_ID id)
{
  if (id >= DIGITAL_SENSOR_COUNT)
  {
    return NULL;
  }
  return &digital_sensors[id];
}

static uint8_t
    stable_state[DIGITAL_SENSOR_COUNT]; /*!< 去抖后的稳定状态（0/1） */
static uint8_t
    last_raw[DIGITAL_SENSOR_COUNT]; /*!< 上一次原始电平（用于检测变化） */
static uint32_t
    last_change_ts[DIGITAL_SENSOR_COUNT]; /*!< 上一次电平变化时间戳 */

static DIGITAL_SENSOR_Callback callbacks[DIGITAL_SENSOR_COUNT] = {
    0}; /*!< 回调函数数组 */

/*******************************************************************************
 *                              私有函数声明
 ******************************************************************************/
static void emit_event(DIGITAL_SENSOR_ID id, DIGITAL_SENSOR_Event ev);

/**
 * @brief  读取传感器原始GPIO电平（指针版本）
 * @param  sensor: 传感器指针
 * @retval GPIO_PIN_RESET(0) 或 GPIO_PIN_SET(1)
 */
uint8_t DIGITAL_SENSOR_ReadRaw_Ptr(DIGITAL_SENSOR *sensor)
{
  if (sensor == NULL)
  {
    return 0;
  }
  return (uint8_t)HAL_GPIO_ReadPin(sensor->port, sensor->pin);
}

/**
 * @brief  读取传感器原始GPIO电平（ID版本）
 * @param  id: 传感器ID
 * @retval 0=低电平，1=高电平
 */
uint8_t DIGITAL_SENSOR_ReadRaw(DIGITAL_SENSOR_ID id)
{
  return DIGITAL_SENSOR_ReadRaw_Ptr(DIGITAL_SENSOR_GetPtr(id));
}

/**
 * @brief  判断传感器是否处于触发状态（指针版本，考虑极性）
 * @param  sensor: 传感器指针
 * @retval 0=未触发，1=触发
 */
uint8_t DIGITAL_SENSOR_IsTriggered_Ptr(DIGITAL_SENSOR *sensor)
{
  if (sensor == NULL)
  {
    return 0;
  }
  uint8_t raw = DIGITAL_SENSOR_ReadRaw_Ptr(sensor);
  return sensor->direct
             ? (raw == GPIO_PIN_SET ? 1 : 0)    /* direct=1: 高电平触发 */
             : (raw == GPIO_PIN_RESET ? 1 : 0); /* direct=0: 低电平触发 */
}

/**
 * @brief  判断传感器是否处于触发状态（ID版本，考虑极性）
 * @param  id: 传感器ID
 * @retval 0=未触发，1=触发
 */
uint8_t DIGITAL_SENSOR_IsTriggered(DIGITAL_SENSOR_ID id)
{
  return DIGITAL_SENSOR_IsTriggered_Ptr(DIGITAL_SENSOR_GetPtr(id));
}

/**
 * @brief  弱符号事件处理函数（默认空实现）
 * @param  id: 传感器ID
 * @param  ev: 事件类型
 * @note   用户在任意C文件中重新实现此函数以接管传感器事件
 * @note   不同编译器的弱符号语法略有差异，此处做兼容处理
 */
#if defined(__GNUC__) || defined(__clang__)
__attribute__((weak)) void
DIGITAL_SENSOR_EventHandler(DIGITAL_SENSOR_ID id, DIGITAL_SENSOR_Event ev)
{
  (void)id;
  (void)ev;
}
#elif defined(__ICCARM__)
/* IAR不支持同名弱符号覆盖，用户可使用回调注册或链接脚本 */
void DIGITAL_SENSOR_EventHandler(DIGITAL_SENSOR_ID id,
                                 DIGITAL_SENSOR_Event ev)
{
  (void)id;
  (void)ev;
}
#else
__weak void DIGITAL_SENSOR_EventHandler(DIGITAL_SENSOR_ID id,
                                        DIGITAL_SENSOR_Event ev)
{
  (void)id;
  (void)ev;
}
#endif

/**
 * @brief  事件分发内部函数
 * @param  id: 传感器ID
 * @param  ev: 事件类型
 * @note   优先调用已注册回调，否则调用弱处理函数
 */
static void emit_event(DIGITAL_SENSOR_ID id, DIGITAL_SENSOR_Event ev)
{
  if (id >= DIGITAL_SENSOR_COUNT)
    return;

  if (callbacks[id])
    callbacks[id](id, ev); /* 调用注册的回调 */
  else
    DIGITAL_SENSOR_EventHandler(id, ev); /* 调用弱处理函数（用户可重定义） */
}

/**
 * @brief  初始化所有传感器
 * @note   清空状态数组并读取初始电平
 * @retval None
 */
void DIGITAL_SENSOR_Init(void)
{
  uint32_t now = HAL_GetTick();
  for (int i = 0; i < DIGITAL_SENSOR_COUNT; ++i)
  {
    last_raw[i] = DIGITAL_SENSOR_ReadRaw_Ptr(&digital_sensors[i]);
    stable_state[i] = DIGITAL_SENSOR_IsTriggered_Ptr(&digital_sensors[i]);
    last_change_ts[i] = now;
    callbacks[i] = NULL;
  }
}

/**
 * @brief  注册传感器回调函数
 * @param  id: 传感器ID
 * @param  cb: 回调函数指针
 * @note   注册后该传感器的事件将调用回调而非DIGITAL_SENSOR_EventHandler
 * @retval None
 */
void DIGITAL_SENSOR_RegisterCallback(DIGITAL_SENSOR_ID id,
                                     DIGITAL_SENSOR_Callback cb)
{
  if (id >= DIGITAL_SENSOR_COUNT)
  {
    DEBUG_ERROR("DIGITAL_SENSOR_RegisterCallback: 无效的传感器ID");
    return;
  }
  callbacks[id] = cb;
}

/**
 * @brief  取消传感器回调注册
 * @param  id: 传感器ID
 * @note   取消后该传感器的事件将回到调用DIGITAL_SENSOR_EventHandler
 * @retval None
 */
void DIGITAL_SENSOR_UnregisterCallback(DIGITAL_SENSOR_ID id)
{
  if (id >= DIGITAL_SENSOR_COUNT)
  {
    DEBUG_ERROR("DIGITAL_SENSOR_UnregisterCallback: 无效的传感器ID");
    return;
  }
  callbacks[id] = NULL;
}

/**
 * @brief  传感器扫描任务（非阻塞，需周期调用）
 * @note   建议在主循环或定时器中每5~50ms调用一次
 * @note   内部处理：消抖 -> 边沿检测 -> 事件识别 -> 回调触发
 * @retval None
 *
 * @par    状态机逻辑：
 *         1. 检测原始电平变化 -> 重置消抖计时器
 *         2. 原始电平稳定超过消抖时间 -> 确认为有效电平
 *         3. 有效电平与上次稳定状态不同 -> 触发边沿事件（触发/释放）
 */
void DIGITAL_SENSOR_Task(void)
{
  uint32_t now = HAL_GetTick();

  for (int i = 0; i < DIGITAL_SENSOR_COUNT; ++i)
  {
    uint8_t raw = DIGITAL_SENSOR_ReadRaw_Ptr(&digital_sensors[i]);
    uint8_t triggered = digital_sensors[i].direct ? (raw == GPIO_PIN_SET)
                                                  : (raw == GPIO_PIN_RESET);

    /* 阶段1：检测原始电平变化 */
    if (raw != last_raw[i])
    {
      last_change_ts[i] = now; /* 重置消抖计时器 */
      last_raw[i] = raw;
    }
    else
    {
      /* 阶段2：原始电平稳定，检查是否超过消抖时间 */
      if (((int32_t)(now - last_change_ts[i])) >= SENSOR_DEBOUNCE_MS)
      {
        /* 阶段3：比较去抖后状态，检测边沿 */
        if (triggered != stable_state[i])
        {
          stable_state[i] = triggered;

          if (triggered)
          {
            /* 触发边沿 */
            emit_event((DIGITAL_SENSOR_ID)i, SENSOR_EV_TRIGGERED);
          }
          else
          {
            /* 释放边沿 */
            emit_event((DIGITAL_SENSOR_ID)i, SENSOR_EV_RELEASED);
          }
        }
      }
    }
  }
}

#endif // DIGITAL_SENSOR_ENABLE
