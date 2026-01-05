/**
 ******************************************************************************
 * @file    ui_encoder.c
 * @author  菜菜why(B站:菜菜whyy)
 * @brief   UI 旋转编码器（梅花柄）轮询驱动实现文件
 *
 * @attention
 *
 * 使用说明：
 * 1. 本模块为纯轮询实现，调用 UI_ENCODER_Poll() 即可完成旋转与按键事件检测，
 *    建议在 1ms 定时器中调用。
 * 2. 初始化时会读取按键初始电平作为空闲电平（假定初始化时按键处于释放状态）。
 * 3. 旋转检测：在 A 相发生边沿时读取 B 相判定方向（a == b -> LEFT，else ->
 *RIGHT）。
 * 4. 按键检测：基于时间窗口的去抖/单击/双击/长按检测（使用 HAL_GetTick()
 *毫秒级时间）。
 * 5. 事件分发：优先调用已注册的回调，否则调用弱符号 UI_ENCODER_EventHandler。
 *
 * 功能：
 * - 旋转事件：UI_ENCODER_EV_ROTATE_LEFT / UI_ENCODER_EV_ROTATE_RIGHT
 * - 按键事件：按下/释放/单击/双击/长按
 *
 ******************************************************************************
 */

#include "ui_encoder.h"

#ifdef UI_ENCODER_ENABLE

/*******************************************************************************
 *                              私有变量
 ******************************************************************************/
static UI_ENCODER_Callback s_callback = NULL; /* 事件回调指针（优先级高） */

/* 旋转检测状态 */
static uint8_t s_prev_a = 0; /* 上次读取的 A 相电平 */

/* 按键检测状态 */
static uint8_t s_key_idle_level = 0; /* 按键空闲电平（Init 时采样） */
static uint8_t s_key_last_raw = 0;   /* 上一次原始电平 */
static uint32_t s_key_last_change_ts = 0; /* 上一次原始电平变化时间戳(ms) */
static uint8_t s_key_stable = 0; /* 去抖后稳定状态：0=松开，1=按下 */
static uint32_t s_key_press_ts = 0;     /* 按下时间戳(ms) */
static uint32_t s_key_release_ts = 0;   /* 释放时间戳(ms) */
static uint8_t s_key_click_pending = 0; /* 单击待确认标志 */
static uint8_t s_key_long_reported = 0; /* 长按已报告标志 */

/*******************************************************************************
 *                              私有函数声明
 ******************************************************************************/
static void emit_event(UI_ENCODER_Event ev);
static uint8_t read_a_phase(void);
static uint8_t read_b_phase(void);
static uint8_t read_key_raw(void);

/**
 * @brief  弱符号事件处理函数（默认空实现）
 * @note   用户可在任意 C 文件中重定义以接管事件处理
 */
#if defined(__GNUC__) || defined(__clang__)
__attribute__((weak)) void UI_ENCODER_EventHandler(UI_ENCODER_Event ev) {
  (void)ev;
}
#elif defined(__ICCARM__)
void UI_ENCODER_EventHandler(UI_ENCODER_Event ev) { (void)ev; }
#else
__weak void UI_ENCODER_EventHandler(UI_ENCODER_Event ev) { (void)ev; }
#endif

/**
 * @brief  内部事件分发（优先回调，否则弱函数）
 */
static void emit_event(UI_ENCODER_Event ev) {
  if (s_callback)
    s_callback(ev);
  else
    UI_ENCODER_EventHandler(ev);
}

/**
 * @brief  读取 A 相电平（返回 0 或 1）
 */
static uint8_t read_a_phase(void) {
  return HAL_GPIO_ReadPin(UI_ENCODER_A_PORT, UI_ENCODER_A_PIN) == GPIO_PIN_SET
             ? 1
             : 0;
}

/**
 * @brief  读取 B 相电平（返回 0 或 1）
 */
static uint8_t read_b_phase(void) {
  return HAL_GPIO_ReadPin(UI_ENCODER_B_PORT, UI_ENCODER_B_PIN) == GPIO_PIN_SET
             ? 1
             : 0;
}

/**
 * @brief  读取按键原始 GPIO 电平（返回 HAL 的 HAL_GPIO_ReadPin 结果 0/1）
 */
static uint8_t read_key_raw(void) {
  return (uint8_t)HAL_GPIO_ReadPin(UI_ENCODER_KEY_PORT, UI_ENCODER_KEY_PIN);
}

/**
 * @brief  初始化 UI 编码器模块
 * @note   读取按键当前电平作为空闲电平（假定初始化时按键未按下）
 */
void UI_ENCODER_Init(void) {
  /* 旋转部分 */
  s_prev_a = read_a_phase();

  /* 按键部分 */
  s_key_idle_level = read_key_raw();
  s_key_last_raw = s_key_idle_level;
  s_key_last_change_ts = HAL_GetTick();
  s_key_stable = 0; /* 假定松开状态 */
  s_key_press_ts = 0;
  s_key_release_ts = 0;
  s_key_click_pending = 0;
  s_key_long_reported = 0;

  s_callback = NULL;
}

/**
 * @brief  轮询函数（需周期调用，建议 1ms）
 *
 * @note   包含旋转（A 相边沿判定）和按键（消抖/单击/双击/长按）检测
 * @note   使用 HAL_GetTick() 获取当前 ms。去抖和长按阈值由 ui_encoder.h
 * 中宏控制
 *
 * @par    工作流程：
 *         【旋转检测】
 *         1. 检测 A 相边沿（任意边沿）
 *         2. 读取 B 相电平判定方向：a == b -> 左转，否则右转
 *
 *         【按键检测】
 *         1. 检测原始电平变化 -> 重置消抖计时器
 *         2. 原始电平稳定超过消抖时间 -> 确认为有效电平
 *         3. 有效电平变化 -> 触发边沿事件（按下/释放）
 *         4. 按下持续时间检测 -> 触发长按事件
 *         5. 释放后时间窗口检测 -> 判断单击/双击
 *
 * @retval None
 */
void UI_ENCODER_Poll(void) {
  uint32_t now = HAL_GetTick();

  /***************************************************************************
   *                              旋转检测（A 相任意边沿）
   **************************************************************************/
  {
    uint8_t a = read_a_phase();
    if (a != s_prev_a) {
      /* A 相发生边沿，读取 B 相判定方向 */
      uint8_t b = read_b_phase();
      if (a == b)
        emit_event(UI_ENCODER_EV_ROTATE_LEFT);
      else
        emit_event(UI_ENCODER_EV_ROTATE_RIGHT);

      s_prev_a = a;
    }
  }

  /***************************************************************************
   *                              按键去抖与事件检测
   **************************************************************************/
  {
    uint8_t raw = read_key_raw();
    uint8_t pressed = (raw != s_key_idle_level) ? 1 : 0;

    /* 检测原始电平变化，重置去抖计时器 */
    if (raw != s_key_last_raw) {
      s_key_last_change_ts = now;
      s_key_last_raw = raw;
    } else {
      /* 原始电平稳定，判断是否超过去抖时间 */
      if (((int32_t)(now - s_key_last_change_ts)) >= UI_ENCODER_DEBOUNCE_MS) {
        /* 去抖后状态变化 -> 边沿事件 */
        if (pressed != s_key_stable) {
          s_key_stable = pressed;
          if (pressed) {
            /* 按下边沿 */
            s_key_press_ts = now;
            s_key_long_reported = 0;
            emit_event(UI_ENCODER_EV_KEY_PRESS);
          } else {
            /* 释放边沿 */
            s_key_release_ts = now;
            emit_event(UI_ENCODER_EV_KEY_RELEASE);

            uint32_t held = (s_key_press_ts ? (now - s_key_press_ts) : 0);
            if (held < UI_ENCODER_LONG_MS) {
              /* 短按：检查单击/双击 */
              if (s_key_click_pending &&
                  ((now - s_key_release_ts) <= UI_ENCODER_DBL_MS)) {
                /* 双击 */
                s_key_click_pending = 0;
                emit_event(UI_ENCODER_EV_KEY_DOUBLE_CLICK);
              } else {
                /* 标记单击待确认（等待双击窗口） */
                s_key_click_pending = 1;
              }
            } else {
              /* 长按后释放，不计单击 */
              s_key_click_pending = 0;
            }
          }
        } else {
          /* 稳定状态下的持续检测：长按与单击超时 */
          if (s_key_stable) {
            /* 持续按下：检测长按 */
            if (!s_key_long_reported && s_key_press_ts &&
                ((now - s_key_press_ts) >= UI_ENCODER_LONG_MS)) {
              s_key_long_reported = 1;
              emit_event(UI_ENCODER_EV_KEY_LONG_PRESS);
            }
          } else {
            /* 持续释放：检测单击超时，超时则确认单击 */
            if (s_key_click_pending &&
                ((now - s_key_release_ts) > UI_ENCODER_DBL_MS)) {
              s_key_click_pending = 0;
              emit_event(UI_ENCODER_EV_KEY_CLICK);
            }
          }
        }
      }
    }
  }
}

/**
 * @brief  注册事件回调函数（优先于弱函数）
 */
void UI_ENCODER_RegisterCallback(UI_ENCODER_Callback cb) {
  if (cb == NULL) {
    DEBUG_ERROR("UI_ENCODER_RegisterCallback: 回调指针为空");
    return;
  }
  s_callback = cb;
}

/**
 * @brief  取消回调注册（回退到弱函数处理）
 */
void UI_ENCODER_UnregisterCallback(void) { s_callback = NULL; }

#endif /* UI_ENCODER_ENABLE */