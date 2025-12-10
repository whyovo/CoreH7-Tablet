/**
 ******************************************************************************
 * @file    audio_player.c
 * @author  菜菜why(B站:菜菜whyy)
 * @brief   SD 卡音频文件播放驱动实现
 ******************************************************************************
 */

#include "audio_player.h"
#include <stdio.h>
#include <string.h>

#if (defined(DSPEAKER_ENABLE) && defined(WAV_PLAY_ENABLE))
/* 全局播放器实例 */
static AudioPlayer_t *gp_player = NULL;
/* 临时缓冲区用于双声道转换，大小需容纳双倍数据 */
static int16_t s_temp_buffer[DSPEAKER_BUFFER_SIZE];



/**
 * @brief WAV 文件数据读取回调（DMA 中断中调用）
 */
static uint32_t audio_data_callback(int16_t *pBuffer, uint32_t size,
                                    uint8_t isFirstHalf) {
  UINT bytes_read = 0;
  uint32_t i;

  if (gp_player == NULL)
    return size;

  uint16_t channels = gp_player->fmt.num_channels;
  /* 计算本次需要从文件读取的字节数 */
  /* DMA 缓冲区大小为 size (int16_t) */
  /* I2S 为立体声模式，需要 L, R 数据交替 */
  /* 因此 size 个 int16_t 对应 size/2 个立体声帧 */

  uint32_t bytes_to_read_needed;

  if (channels == 2) {
    /* 双声道源：直接读取，无需转换 */
    /* 需要读取 size 个 int16_t */
    bytes_to_read_needed = size * sizeof(int16_t);
  } else {
    /* 单声道源：需要读取 size/2 个 int16_t，然后扩展 */
    bytes_to_read_needed = (size / 2) * sizeof(int16_t);
  }

  /* 检查是否到达文件末尾 */
  if (gp_player->current_pos + bytes_to_read_needed >
      gp_player->data_start_pos + gp_player->data_size) {
    /* 检查循环次数 */
    if (gp_player->loop_enable) {
      if (gp_player->loop_count == 0) {
        /* 无限循环 */
        gp_player->current_pos = gp_player->data_start_pos;
        f_lseek(&gp_player->file, gp_player->current_pos);
        gp_player->current_loop++;
      } else if (gp_player->current_loop < gp_player->loop_count - 1) {
        /* 继续循环 */
        gp_player->current_pos = gp_player->data_start_pos;
        f_lseek(&gp_player->file, gp_player->current_pos);
        gp_player->current_loop++;
      } else {
        /* 不循环，播放完成 */
        memset(pBuffer, 0, size * sizeof(int16_t));
        gp_player->is_playing = 0;
        return size;
      }
    } else {
      /* 不循环，播放完成 */
      memset(pBuffer, 0, size * sizeof(int16_t));
      gp_player->is_playing = 0;
      return size;
    }
  }

  if (channels == 1) {
    /* 单声道读取到临时缓冲并扩展为立体声 (S -> L, R) */
    if (bytes_to_read_needed > sizeof(s_temp_buffer)) {
      bytes_to_read_needed = sizeof(s_temp_buffer);
    }

    f_read(&gp_player->file, s_temp_buffer, bytes_to_read_needed, &bytes_read);
    gp_player->current_pos += bytes_read;

    uint32_t samples_read = bytes_read / sizeof(int16_t);

    for (i = 0; i < samples_read; i++) {
      pBuffer[2 * i] = s_temp_buffer[i];     /* Left */
      pBuffer[2 * i + 1] = s_temp_buffer[i]; /* Right */
    }

    if (samples_read * 2 < size) {
      /* 文件末尾，补 0 */
      memset(pBuffer + samples_read * 2, 0,
             (size - samples_read * 2) * sizeof(int16_t));
    }
  } else if (channels == 2) {
    /* 双声道直接读取到 DMA 缓冲区 */
    /* I2S 会自动按 L, R 发送 */
    f_read(&gp_player->file, pBuffer, bytes_to_read_needed, &bytes_read);
    gp_player->current_pos += bytes_read;

    if (bytes_read < bytes_to_read_needed) {
    /* 文件末尾，补 0 */
    memset((uint8_t *)pBuffer + bytes_read, 0,
           bytes_to_read_needed - bytes_read);
    }
  }

  return size;
}

/**
 * @brief 打开并解析 WAV 文件
 */
HAL_StatusTypeDef AudioPlayer_OpenFile(AudioPlayer_t *player,
                                       const char *filename) {
  WAV_RIFF_Header riff_header;
  WAV_Format_Header fmt_header;
  WAV_Data_Header chunk_header;
  UINT bytes_read;
  char buf[256];

  if (player == NULL || filename == NULL)
    return HAL_ERROR;

  /* 打开文件 */
  FRESULT res = f_open(&player->file, filename, FA_READ);
  if (res != FR_OK) {
    snprintf(buf, sizeof(buf), "打开文件失败: %s", filename);
    DEBUG_ERROR(buf);
    return HAL_ERROR;
  }

  /* 读取 RIFF 头 */
  f_read(&player->file, (void *)&riff_header, sizeof(WAV_RIFF_Header),
         &bytes_read);
  if (riff_header.chunk_id != 0x46464952) /* "RIFF" */
  {
    DEBUG_ERROR("不是有效的 WAV 文件（RIFF 头错误）");
    f_close(&player->file);
    return HAL_ERROR;
  }

  /* 读取 fmt 块 */
  f_read(&player->file, (void *)&fmt_header, sizeof(WAV_Format_Header),
         &bytes_read);
  if (fmt_header.subchunk_id != 0x20746d66) /* "fmt " */
  {
    DEBUG_ERROR("WAV 格式错误");
    f_close(&player->file);
    return HAL_ERROR;
  }

  /* 检查音频格式 */
  if (fmt_header.audio_format != 1) /* PCM */
  {
    DEBUG_ERROR("仅支持 PCM 格式音频");
    f_close(&player->file);
    return HAL_ERROR;
  }

  if (fmt_header.bits_per_sample != 16) {
    DEBUG_ERROR("仅支持 16-bit 音频");
    f_close(&player->file);
    return HAL_ERROR;
  }

  /* 保存格式信息 */
  memcpy(&player->fmt, &fmt_header, sizeof(WAV_Format_Header));

  /* 处理 fmt 块可能的额外数据 (如果 subchunk_size > 16) */
  if (fmt_header.subchunk_size > 16) {
    f_lseek(&player->file,
            f_tell(&player->file) + (fmt_header.subchunk_size - 16));
  }

  /* 循环查找 data 块 */
  while (1) {
    /* 读取块头 (ID + Size) */
    res = f_read(&player->file, (void *)&chunk_header, sizeof(WAV_Data_Header),
                 &bytes_read);

    if (res != FR_OK || bytes_read < sizeof(WAV_Data_Header)) {
      DEBUG_ERROR("无法找到 data 块");
      f_close(&player->file);
      return HAL_ERROR;
    }

    /* 检查是否为 "data" (0x61746164) */
    if (chunk_header.subchunk_id == 0x61746164) {
      break; /* 找到了 */
    }

    /* 如果不是 data 块，跳过它 */
    /* 注意：WAV 规范要求块大小若为奇数，文件流中会有一个填充字节，但 size
     * 不包含它 */
    uint32_t skip_size = chunk_header.subchunk_size;
    if (skip_size % 2 != 0) {
      skip_size++;
    }

    /* 移动文件指针跳过当前块 */
    f_lseek(&player->file, f_tell(&player->file) + skip_size);
  }

  /* 记录 data 块信息 */
  player->data_start_pos = f_tell(&player->file);
  player->data_size = chunk_header.subchunk_size;
  player->current_pos = player->data_start_pos;
  player->is_playing = 0;
  player->loop_enable = 0;
  player->loop_count = 1;
player->current_loop = 0;

  return HAL_OK;
}

/**
 * @brief 关闭文件
 */
void AudioPlayer_CloseFile(AudioPlayer_t *player) {
  if (player == NULL)
    return;

  f_close(&player->file);
  player->is_playing = 0;
  gp_player = NULL;
}

/**
 * @brief 开始播放（支持循环）
 */
HAL_StatusTypeDef AudioPlayer_PlayWithLoop(AudioPlayer_t *player,
                                           uint32_t loop_count) {
  HAL_StatusTypeDef status;


  if (player == NULL)
    return HAL_ERROR;

  /* 重置到文件开始 */
  player->current_pos = player->data_start_pos;
  f_lseek(&player->file, player->current_pos);

  /* 设置循环参数 */
  if (loop_count == 0) {
    player->loop_enable = 1;
    player->loop_count = 0; /* 无限循环 */

  } else if (loop_count == 1) {
    player->loop_enable = 0; /* 单次播放 */

  } else {
    player->loop_enable = 1;
    player->loop_count = loop_count;

  }

  player->current_loop = 0;


  /* 注册全局指针 */
  gp_player = player;

  /* 注册数据回调 */
  DSPEAKER_RegisterCallback(audio_data_callback);

  /* 启动扬声器 */
  status = DSPEAKER_Start();
  if (status == HAL_OK) {
    player->is_playing = 1;
  } else {
    DEBUG_ERROR("扬声器启动失败");
  }

  return status;
}

/**
 * @brief 开始播放（单次，不循环）
 */
HAL_StatusTypeDef AudioPlayer_Play(AudioPlayer_t *player) {
  return AudioPlayer_PlayWithLoop(player, 1);
}

/**
 * @brief 停止播放
 */
void AudioPlayer_Stop(AudioPlayer_t *player) {
  if (player == NULL)
    return;

  DSPEAKER_Stop();
  DSPEAKER_UnregisterCallback();
  player->is_playing = 0;
  gp_player = NULL;
  DEBUG_INFO("停止播放");
}

/**
 * @brief 获取播放状态
 */
uint8_t AudioPlayer_IsPlaying(AudioPlayer_t *player) {
  if (player == NULL)
    return 0;

  return player->is_playing && (DSPEAKER_GetState() == DSPEAKER_STATE_PLAYING);
}

/**
 * @brief 获取当前循环次数
 */
uint32_t AudioPlayer_GetLoopCount(AudioPlayer_t *player) {
  if (player == NULL)
    return 0;

  return player->current_loop + 1; /* 返回当前播放的循环次数 */
}

#endif
