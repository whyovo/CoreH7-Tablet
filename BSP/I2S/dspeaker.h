/**
 ******************************************************************************
 * @file    dspeaker.h
 * @author  菜菜why(B站:菜菜whyy)
 * @brief   MAX98357A 数字扬声器 I2S DMA 驱动头文件
 ******************************************************************************
 * @attention
 *
 * 使用方法:
 * - 使用 I2S 主机发送模式 + DMA 循环模式实现连续音频播放
 * - 调用 DSPEAKER_Init() 初始化并准备播放
 * - 调用 DSPEAKER_Start() 启动播放
 * - 通过 DSPEAKER_FeedData() 或回调函数向缓冲区填充数据
 *
 * 使用示例:
    DSPEAKER_Init();
    DSPEAKER_Start();

    // 填充数据到缓冲区
    DSPEAKER_FeedData(pAudioData, dataSize);

    // 播放期间监控
    while (DSPEAKER_GetState() == DSPEAKER_STATE_PLAYING) {
        // 持续填充数据
        HAL_Delay(10);
    }

    DSPEAKER_Stop();
 *
 * MAX98357A 引脚连接:
 * - SCK  -> I2S_CK   (时钟)
 * - WS   -> I2S_WS   (字选择/左右声道)
 * - SD   -> I2S_SDO  (数据输出)
 * - GAIN -> GND/3.3V (增益控制，GND=-4dB, 3.3V=+8dB)
 * - SHDN -> GPIO     (关闭引脚，高电平激活，低电平关闭)
 * - VDD  -> 3.3V
 * - GND  -> GND
 *
 ******************************************************************************
 */

#ifndef DSPEAKER_H
#define DSPEAKER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "config.h"
#include <stdint.h>
#ifdef DSPEAKER_ENABLE

#define MP3_PLAY_ENABLE      //定义了就开启mp3功能，不开启就不编译mp3相关代码，减小体积
#define WAV_PLAY_ENABLE // 定义了就开启wav功能，不开启就不编译wav相关代码，减小体积
/*******************************************************************************
 *                              I2S 外设配置
 ******************************************************************************/
#define DSPEAKER_I2S_HANDLE hi2s2        /*!< I2S句柄 */
#define DSPEAKER_I2S_INSTANCE SPI2       /*!< I2S实例 */
#define DSPEAKER_DMA_HANDLE hdma_spi2_tx /*!< DMA句柄（在i2s.c中定义） */
/*******************************************************************************
 *                              音频参数配置
 ******************************************************************************/
#define DSPEAKER_BUFFER_SIZE \
  2048 /*!< DMA缓冲区大小（采样点数，必须是2的幂次） */

  /*******************************************************************************
   *                              导出类型
   ******************************************************************************/

  /**
   * @brief  扬声器状态枚举
   */
  typedef enum
  {
    DSPEAKER_STATE_RESET = 0, /*!< 未初始化状态 */
    DSPEAKER_STATE_READY,     /*!< 就绪状态（已初始化但未播放） */
    DSPEAKER_STATE_PLAYING,   /*!< 播放中（DMA运行中） */
    DSPEAKER_STATE_ERROR      /*!< 错误状态 */
  } DSPEAKER_State;

  /**
   * @brief  音频事件回调函数原型
   * @param  event: 0=前半部分传输完成(需要填充前半部), 1=全部传输完成(需要填充后半部)
   */
  typedef void (*DSPEAKER_EventCallback_t)(uint8_t event);

  /*******************************************************************************
   *                              导出函数
   ******************************************************************************/

  /**
   * @brief  初始化数字扬声器
   * @param  pBuffer: 外部传入的音频缓冲区指针 (需保证 4字节对齐)
   * @param  size: 缓冲区总大小 (采样点数，必须是偶数)
   */
  HAL_StatusTypeDef DSPEAKER_Init(int16_t *pBuffer, uint32_t size);

  /**
   * @brief  启动扬声器播放（启动DMA发送）
   * @retval HAL_StatusTypeDef: HAL_OK=成功, 其他=失败
   */
  HAL_StatusTypeDef DSPEAKER_Start(void);

  /**
   * @brief  停止扬声器播放（停止DMA发送）
   * @retval HAL_StatusTypeDef: HAL_OK=成功, 其他=失败
   */
  HAL_StatusTypeDef DSPEAKER_Stop(void);

  /**
   * @brief  获取当前状态
   * @retval DSPEAKER_State: 当前状态
   */
  DSPEAKER_State DSPEAKER_GetState(void);

  /**
   * @brief  设置音量（0-100）
   * @param  volume: 音量级别 (0-100)
   * @note   软件音量，需在填充数据时处理
   */
  void DSPEAKER_SetVolume(uint8_t volume);

  /**
   * @brief  获取当前音量
   */
  uint8_t DSPEAKER_GetVolume(void);

  /**
   * @brief  注册事件回调（通知应用层填充数据）
   * @param  callback: 回调函数指针
   * @retval None
   */
  void DSPEAKER_RegisterEventCallback(DSPEAKER_EventCallback_t callback);

  /**
   * @brief  DMA传输完成回调（内部使用）
   */
  void DSPEAKER_DMA_TransferComplete_Callback(void);

  /**
   * @brief  DMA半传输完成回调（内部使用）
   * @note   用户不应直接调用此函数
   * @retval None
   */
  void DSPEAKER_DMA_HalfTransfer_Callback(void);

#endif
#ifdef __cplusplus
}
#endif

#endif // DSPEAKER_H
