/**
 ******************************************************************************
 * @file    encoder(exti).h
 * @author  菜菜why(B站:菜菜whyy)
 * @brief   正交编码器(外部中断)驱动头文件,基于外部中断实现测速、计数、旋转调参
 ******************************************************************************
 * @attention
 *
 * 使用方法:
 * 1. 修改 ENCODER_LIST 宏定义以配置编码器列表
 *    - name: 枚举名称(如 ENCODER1)
 *    - a_port/a_pin: A相引脚
 *    - b_port/b_pin: B相引脚
 *    - ppr: 每转脉冲数(Pulse Per Revolution)
 * 2. 在CubeMX中配置A相引脚为外部中断(上升沿+下降沿触发)
 * 3. 在中断回调中调用 ENCODER_EXTI_Handler()
 * 4. 周期调用 ENCODER_Update() 更新速度计算
 *
 * 功能特性:
 * - 正反转计数
 * - 速度测量(RPM/脉冲频率)
 * - 参数调节(步进/平滑调节)
 *
 ******************************************************************************
 */

#ifndef ENCODER_EXTI_H
#define ENCODER_EXTI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"

/*******************************************************************************
 *                              编码器配置列表
 ******************************************************************************/
#define ENCODER_LIST                                                           \
  X(ENCODER1, GPIOA, GPIO_PIN_0, GPIOA,                                        \
    GPIO_PIN_1) /* 去掉 ppr 参数，速度为相对速度(脉冲/分钟) */

/*******************************************************************************
 *                              编码器参数配置
 ******************************************************************************/
#define ENCODER_UPDATE_PERIOD_MS 100 /*!< 速度更新周期(毫秒) */

/*******************************************************************************
 *                              编码器导出类型
 ******************************************************************************/

/**
 * @brief  编码器结构体定义
 */
typedef struct {
  /* 硬件配置 */
  GPIO_TypeDef *a_port; /*!< A相GPIO端口 */
  uint16_t a_pin;       /*!< A相引脚编号 */
  GPIO_TypeDef *b_port; /*!< B相GPIO端口 */
  uint16_t b_pin;       /*!< B相引脚编号 */
  /* 注意：移除了 ppr 字段，速度为相对值(脉冲/分钟) */

  /* 运行数据 */
  int32_t count; /*!< 累计脉冲计数(有符号,支持正反转) - 作为"距离"的脉冲数 */
  int32_t last_count; /*!< 上次计数(用于计算速度) */
  float rpm;          /*!< 相对速度(脉冲/分钟) */
  uint32_t last_tick; /*!< 上次更新时间戳 */
} ENCODER;

/**
 * @brief  编码器枚举ID(自动生成)
 */
#define X(name, a_port, a_pin, b_port, b_pin) name,
typedef enum {
  ENCODER_LIST ENCODER_COUNT /*!< 编码器总数 */
} ENCODER_ID;
#undef X

/*******************************************************************************
 *                              编码器数组
 ******************************************************************************/
extern ENCODER encoders[]; /*!< 编码器数组 */

/*******************************************************************************
 *                              编码器导出函数
 ******************************************************************************/

void ENCODER_Init(void);
void ENCODER_Update(void);
void ENCODER_EXTI_Handler(uint16_t GPIO_Pin);

/* 获取脉冲计数（作为距离，单位：脉冲） */
int32_t ENCODER_GetCount(ENCODER *encoder);

/* 重置脉冲计数 */
void ENCODER_ResetCount(ENCODER *encoder);

/* 获取相对速度（脉冲/分钟） */
float ENCODER_GetRPM(ENCODER *encoder);

/* 参数调节接口 */
void ENCODER_AdjustInt(ENCODER *encoder, int32_t *value, int32_t step,
                       int32_t min, int32_t max);
void ENCODER_AdjustFloat(ENCODER *encoder, float *value, float step, float min,
                         float max);

#ifdef __cplusplus
}
#endif

#endif // !ENCODER_EXTI_H