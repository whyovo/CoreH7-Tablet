/**
 ******************************************************************************
 * @file    audio_player.h
 * @author  菜菜why(B站:菜菜whyy)
 * @brief   SD 卡音频文件播放驱动
 *          支持 WAV 格式（PCM 16-bit）
 ******************************************************************************
 */

#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "dspeaker.h"
#include "fatfs.h"
#include <stdint.h>

#if (defined(DSPEAKER_ENABLE) && defined(WAV_PLAY_ENABLE))
  /* ===== WAV 文件格式定义 ===== */
  typedef struct
  {
    uint32_t chunk_id;   /* "RIFF" */
    uint32_t chunk_size; /* 文件大小-8 */
    uint32_t format;     /* "WAVE" */
  } WAV_RIFF_Header;

  typedef struct
  {
    uint32_t subchunk_id;     /* "fmt " */
    uint32_t subchunk_size;   /* 16 for PCM */
    uint16_t audio_format;    /* 1 for PCM */
    uint16_t num_channels;    /* 1=mono, 2=stereo */
    uint32_t sample_rate;     /* Hz */
    uint32_t byte_rate;       /* sample_rate * num_channels * bits_per_sample/8 */
    uint16_t block_align;     /* num_channels * bits_per_sample/8 */
    uint16_t bits_per_sample; /* 16 */
  } WAV_Format_Header;

  typedef struct
  {
    uint32_t subchunk_id;   /* "data" */
    uint32_t subchunk_size; /* 数据长度 */
  } WAV_Data_Header;

  typedef struct
  {
    FIL file;
    WAV_Format_Header fmt;
    uint32_t data_start_pos;
    uint32_t data_size;
    uint32_t current_pos;
    uint8_t is_playing;
    uint8_t loop_enable;   /* 是否启用循环播放 */
    uint32_t loop_count;   /* 循环次数（0=无限循环） */
    uint32_t current_loop; /* 当前循环次数 */
  } AudioPlayer_t;

  /* ===== 导出函数 ===== */

  /**
   * @brief 打开并解析 WAV 文件
   * @param player 播放器结构体指针
   * @param filename 文件路径（例："0:music.wav"）
   * @return HAL_OK=成功, HAL_ERROR=失败
   */
  HAL_StatusTypeDef AudioPlayer_OpenFile(AudioPlayer_t *player,
                                         const char *filename);

  /**
   * @brief 关闭音频文件
   */
  void AudioPlayer_CloseFile(AudioPlayer_t *player);

  /**
   * @brief 开始播放
   * @param player 播放器结构体指针
   * @param loop_count 循环次数（0=无限循环, 1=播放1次, 2=播放2次...）
   */
  HAL_StatusTypeDef AudioPlayer_PlayWithLoop(AudioPlayer_t *player,
                                             uint32_t loop_count);

  /**
   * @brief 开始播放（单次，不循环）
   */
  HAL_StatusTypeDef AudioPlayer_Play(AudioPlayer_t *player);

  /**
   * @brief 停止播放
   */
  void AudioPlayer_Stop(AudioPlayer_t *player);

  /**
   * @brief 获取当前播放状态
   */
  uint8_t AudioPlayer_IsPlaying(AudioPlayer_t *player);

  /**
   * @brief 获取当前循环次数
   */
  uint32_t AudioPlayer_GetLoopCount(AudioPlayer_t *player);
#endif
#ifdef __cplusplus
}
#endif

#endif // AUDIO_PLAYER_H
