/**
 ******************************************************************************
 * @file    encoder(exti).c
 * @author  菜菜why(B站:菜菜whyy)
 * @brief   编码器驱动实现文件
 ******************************************************************************
 * @attention
 *
 * 本文件实现:
 * - 基于外部中断的正反转计数
 * - 速度测量(RPM)
 * - 参数调节辅助函数
 *
 * 注意事项:
 * - 需在CubeMX中配置A相引脚为上升沿+下降沿触发的外部中断
 * - 需在stm32xxxx_it.c中调用 ENCODER_EXTI_Handler()
 *
 ******************************************************************************
 */

#include "encoder(exti).h"

#ifdef ENCODER_ENABLE

/*******************************************************************************
 *                              编码器数组定义
 ******************************************************************************/

/**
 * @brief  编码器数组(根据ENCODER_LIST自动生成)
 */
#define X(name, a_port, a_pin, b_port, b_pin)                                  \
  {a_port, a_pin, b_port, b_pin, 0, 0, 0.0f, 0},
ENCODER encoders[] = {ENCODER_LIST};
#undef X

/*******************************************************************************
 *                              私有函数
 ******************************************************************************/

static uint8_t Read_B_Phase(ENCODER *encoder) {
  return HAL_GPIO_ReadPin(encoder->b_port, encoder->b_pin) == GPIO_PIN_SET ? 1
                                                                           : 0;
}

void ENCODER_Init(void) {
  for (int i = 0; i < ENCODER_COUNT; ++i) {
    encoders[i].count = 0;
    encoders[i].last_count = 0;
    encoders[i].rpm = 0.0f;
    encoders[i].last_tick = HAL_GetTick();
  }
}

void ENCODER_Update(void) {
  uint32_t current_tick = HAL_GetTick();

  for (int i = 0; i < ENCODER_COUNT; ++i) {
    ENCODER *enc = &encoders[i];

    uint32_t dt = current_tick - enc->last_tick;
    if (dt < ENCODER_UPDATE_PERIOD_MS)
      continue;

    int32_t delta = enc->count - enc->last_count;

    /* 计算相对速度(脉冲/分钟) = delta * (60000 / dt) */
    enc->rpm = (float)delta * 60000.0f / dt;

    /* 保存状态 */
    enc->last_count = enc->count;
    enc->last_tick = current_tick;
  }
}

void ENCODER_EXTI_Handler(uint16_t GPIO_Pin) {
  for (int i = 0; i < ENCODER_COUNT; ++i) {
    ENCODER *enc = &encoders[i];

    if (GPIO_Pin == enc->a_pin) {
      uint8_t a_state =
          HAL_GPIO_ReadPin(enc->a_port, enc->a_pin) == GPIO_PIN_SET ? 1 : 0;
      uint8_t b_state = Read_B_Phase(enc);

      if (a_state == b_state)
        enc->count--; /* 反向脉冲 */
      else
        enc->count++; /* 正向脉冲 */

      break;
    }
  }
}

int32_t ENCODER_GetCount(ENCODER *encoder) {
  if (encoder == NULL) {
    DEBUG_ERROR("ENCODER_GetCount: 编码器指针为空");
    return 0;
  }
  return encoder->count;
}

void ENCODER_ResetCount(ENCODER *encoder) {
  if (encoder == NULL) {
    DEBUG_ERROR("ENCODER_ResetCount: 编码器指针为空");
    return;
  }
  encoder->count = 0;
  encoder->last_count = 0;
}

float ENCODER_GetRPM(ENCODER *encoder) {
  if (encoder == NULL) {
    DEBUG_ERROR("ENCODER_GetRPM: 编码器指针为空");
    return 0.0f;
  }
  return encoder->rpm;
}

/*******************************************************************************
 *                              参数调节函数
 ******************************************************************************/

void ENCODER_AdjustInt(ENCODER *encoder, int32_t *value, int32_t step,
                       int32_t min, int32_t max) {
  if (encoder == NULL) {
    DEBUG_ERROR("ENCODER_AdjustInt: 编码器指针为空");
    return;
  }
  if (value == NULL) {
    DEBUG_ERROR("ENCODER_AdjustInt: 数值指针为空");
    return;
  }

  int32_t delta = encoder->count - encoder->last_count;
  if (delta == 0)
    return;

  *value += delta * step;

  if (*value > max)
    *value = max;
  if (*value < min)
    *value = min;

  encoder->last_count = encoder->count;
}

void ENCODER_AdjustFloat(ENCODER *encoder, float *value, float step, float min,
                         float max) {
  if (encoder == NULL) {
    DEBUG_ERROR("ENCODER_AdjustFloat: 编码器指针为空");
    return;
  }
  if (value == NULL) {
    DEBUG_ERROR("ENCODER_AdjustFloat: 数值指针为空");
    return;
  }

  int32_t delta = encoder->count - encoder->last_count;
  if (delta == 0)
    return;

  *value += delta * step;

  if (*value > max)
    *value = max;
  if (*value < min)
    *value = min;

  encoder->last_count = encoder->count;
}

#endif // ENCODER_ENABLE