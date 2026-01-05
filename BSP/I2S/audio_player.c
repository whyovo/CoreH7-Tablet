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

/* DMA 内部小缓冲区大小 (采样点数) */
/* 4096 * 2字节 = 8KB，适合放在 SRAM 中 */
#define AUDIO_DMA_BUFFER_SIZE 4096

/* 全局播放器实例 */
static AudioPlayer_t *gp_player = NULL;

/* DMA 专用小缓冲区 (必须 32 字节对齐以适配 Cache 操作) */
/* 此变量通常会被链接器放在 AXI SRAM 或 SRAM1/2 中，DMA 可访问 。别放itcm和DTCM */
static int16_t s_dma_buffer[AUDIO_DMA_BUFFER_SIZE] __attribute__((aligned(32)));

/**
 * @brief 内部函数：向环形缓冲区填充数据
 * @param player 播放器实例
 * @param min_threshold 最小填充阈值 (预填充时设为0，运行时设为4096)
 */
static void AudioPlayer_RefillRingBuffer(AudioPlayer_t *player, uint32_t min_threshold)
{
  /* 读取 volatile 变量前进入临界区，防止 ISR 修改导致计算错误 */
  __disable_irq();
  uint32_t available = player->available_samples;
  __enable_irq();

  uint32_t space = player->buffer_size - available;

  /* 如果空间小于阈值，则不进行读取，避免频繁操作 SD 卡 */
  /* 但在预填充阶段 (min_threshold=0)，只要有空间就填，防止 while 循环空转 */
  if (space < min_threshold)
    return;

  /* 如果空间真的满了（space=0），直接返回 */
  if (space == 0)
    return;

  /* 计算本次最大可写入量 */
  uint32_t samples_to_write = space;
  /* 限制单次读取量，防止占用 CPU 太久 (例如限制为 8KB) */
  if (samples_to_write > 4096)
    samples_to_write = 4096;

  /* 检查是否需要回绕写入 */
  if (player->write_pos + samples_to_write > player->buffer_size)
  {
    samples_to_write = player->buffer_size - player->write_pos;
  }

  /* === 执行文件读取逻辑 === */
  uint16_t channels = player->fmt.num_channels;
  UINT bytes_read = 0;
  uint32_t bytes_to_read_needed;
  FRESULT res;

  /* 临时 buffer 用于单声道转换 */
  int16_t chunk_buffer[512];

  if (channels == 2)
  {
    bytes_to_read_needed = samples_to_write * sizeof(int16_t);
    res = f_read(&player->file, (uint8_t *)&player->audio_buffer[player->write_pos], bytes_to_read_needed, &bytes_read);
  }
  else
  {
    /* 单声道处理 */
    uint32_t mono_samples = samples_to_write / 2;
    if (mono_samples > sizeof(chunk_buffer) / sizeof(int16_t))
      mono_samples = sizeof(chunk_buffer) / sizeof(int16_t);

    samples_to_write = mono_samples * 2;
    bytes_to_read_needed = mono_samples * sizeof(int16_t);

    res = f_read(&player->file, (uint8_t *)chunk_buffer, bytes_to_read_needed, &bytes_read);
  }

  /* 检查读取错误 */
  if (res != FR_OK)
  {

    DEBUG_ERROR("f_read Error: %d", res);
    player->is_file_ended = 1;
    return;
  }

  player->current_pos += bytes_read;

  /* 单声道扩展 */
  if (channels == 1)
  {
    uint32_t mono_read = bytes_read / sizeof(int16_t);
    for (uint32_t i = 0; i < mono_read; i++)
    {
      player->audio_buffer[player->write_pos + 2 * i] = chunk_buffer[i];
      player->audio_buffer[player->write_pos + 2 * i + 1] = chunk_buffer[i];
    }
    if (bytes_read < bytes_to_read_needed)
      samples_to_write = mono_read * 2;
  }
  else
  {
    /* 根据实际读取的字节数更新写入样本数 */
    samples_to_write = bytes_read / sizeof(int16_t);
  }

  /* 处理文件结束/循环 */
  if (bytes_read < bytes_to_read_needed)
  {
    if (player->loop_enable)
    {
      if (player->loop_count == 0 || player->current_loop < player->loop_count - 1)
      {
        if (player->loop_count != 0)
          player->current_loop++;

        // DEBUG_INFO("Looping...");
        f_lseek(&player->file, player->data_start_pos);
        player->current_pos = player->data_start_pos;
        /* 下次循环继续填充 */
      }
      else
      {
        /* 循环结束 */
        player->is_file_ended = 1;
        // DEBUG_INFO("File Ended (Loop finished)");
      }
    }
    else
    {
      /* 不循环，文件结束 */
      player->is_file_ended = 1;
      // DEBUG_INFO("File Ended");
    }
  }

  /* 更新 Cache (SDRAM -> Cache) */
  if (samples_to_write > 0)
  {
    SCB_CleanDCache_by_Addr((uint32_t *)&player->audio_buffer[player->write_pos], samples_to_write * sizeof(int16_t));
  }

  /* 更新指针 */
  player->write_pos += samples_to_write;
  if (player->write_pos >= player->buffer_size)
    player->write_pos = 0;

  /* 原子更新有效数据量 */
  __disable_irq();
  player->available_samples += samples_to_write;
  __enable_irq();
}

/* 来自 ISR 的回调：负责从大缓冲搬运到小缓冲 */
static void player_event_callback(uint8_t event)
{
  if (gp_player == NULL || !gp_player->is_playing)
    return;

  /* 确定填充 DMA 缓冲区的哪一半 */
  uint32_t dma_offset = (event == 0) ? 0 : (AUDIO_DMA_BUFFER_SIZE / 2);
  uint32_t len = AUDIO_DMA_BUFFER_SIZE / 2;

  /* 计算实际可用的数据量，能搬多少搬多少 */
  uint32_t valid_samples = gp_player->available_samples;
  if (valid_samples > len)
    valid_samples = len;

  /* 1. 搬运有效数据 */
  if (valid_samples > 0)
  {
    uint32_t samples_to_end = gp_player->buffer_size - gp_player->play_pos;

    if (samples_to_end >= valid_samples)
    {
      /* 直接拷贝，未回绕 */
      memcpy(&s_dma_buffer[dma_offset], (void *)&gp_player->audio_buffer[gp_player->play_pos], valid_samples * sizeof(int16_t));
      gp_player->play_pos += valid_samples;
    }
    else
    {
      /* 回绕拷贝 */
      memcpy(&s_dma_buffer[dma_offset], (void *)&gp_player->audio_buffer[gp_player->play_pos], samples_to_end * sizeof(int16_t));
      memcpy(&s_dma_buffer[dma_offset + samples_to_end], (void *)&gp_player->audio_buffer[0], (valid_samples - samples_to_end) * sizeof(int16_t));
      gp_player->play_pos = valid_samples - samples_to_end;
    }

    if (gp_player->play_pos >= gp_player->buffer_size)
      gp_player->play_pos = 0;

    __disable_irq();
    gp_player->available_samples -= valid_samples;
    __enable_irq();
  }

  /*  数据不足的部分补零 (防止播放上一段的残留噪音) */
  if (valid_samples < len)
  {
    memset(&s_dma_buffer[dma_offset + valid_samples], 0, (len - valid_samples) * sizeof(int16_t));
  }

  /* H7 处理 2048 次乘法极快，不会造成中断阻塞 */
  uint8_t vol = DSPEAKER_GetVolume();
  if (vol < 100)
  {
    /* 仅处理本次填充的区域 */
    int16_t *target = &s_dma_buffer[dma_offset];
    for (uint32_t i = 0; i < len; i++)
    {
      int32_t val = target[i];
      target[i] = (int16_t)((val * vol) / 100);
    }
  }

  /* 刷新 DMA 缓冲区 Cache (CPU 写 -> DMA 读) */
  SCB_CleanDCache_by_Addr((uint32_t *)&s_dma_buffer[dma_offset], len * sizeof(int16_t));

  /* 通知主循环补充数据  */
  AUDIO_PLAYER_NOTIFY_EVENT();
}

void AudioPlayer_Init(AudioPlayer_t *player, int16_t *buffer, uint32_t size)
{
  player->audio_buffer = buffer;
  player->buffer_size = size;
  player->play_pos = 0;
  player->write_pos = 0;
  player->available_samples = 0;

  /* 初始化 DSPEAKER，使用内部小缓冲区！ */
  DSPEAKER_Init(s_dma_buffer, AUDIO_DMA_BUFFER_SIZE);
  DSPEAKER_RegisterEventCallback(player_event_callback);
}

/**
 * @brief 打开并解析 WAV 文件
 */
HAL_StatusTypeDef AudioPlayer_OpenFile(AudioPlayer_t *player,
                                       const char *filename)
{
  WAV_RIFF_Header riff_header;
  WAV_Format_Header fmt_header;
  WAV_Data_Header chunk_header;
  UINT bytes_read;
  char buf[256];

  if (player == NULL || filename == NULL)
    return HAL_ERROR;

  /* 打开文件 */
  FRESULT res = f_open(&player->file, filename, FA_READ);
  if (res != FR_OK)
  {
    snprintf(buf, sizeof(buf), "打开文件失败: %s (Res=%d)", filename, res);
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

  if (fmt_header.bits_per_sample != 16)
  {
    DEBUG_ERROR("仅支持 16-bit 音频");
    f_close(&player->file);
    return HAL_ERROR;
  }

  /* 保存格式信息 */
  memcpy(&player->fmt, &fmt_header, sizeof(WAV_Format_Header));

  /* 处理 fmt 块可能的额外数据 (如果 subchunk_size > 16) */
  if (fmt_header.subchunk_size > 16)
  {
    f_lseek(&player->file,
            f_tell(&player->file) + (fmt_header.subchunk_size - 16));
  }

  /* 循环查找 data 块 */
  while (1)
  {
    /* 读取块头 (ID + Size) */
    res = f_read(&player->file, (void *)&chunk_header, sizeof(WAV_Data_Header),
                 &bytes_read);

    if (res != FR_OK || bytes_read < sizeof(WAV_Data_Header))
    {
      DEBUG_ERROR("无法找到 data 块");
      f_close(&player->file);
      return HAL_ERROR;
    }

    /* 检查是否为 "data" (0x61746164) */
    if (chunk_header.subchunk_id == 0x61746164)
    {
      break; /* 找到了 */
    }

    /* 如果不是 data 块，跳过它 */
    uint32_t skip_size = chunk_header.subchunk_size;
    if (skip_size % 2 != 0)
    {
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
void AudioPlayer_CloseFile(AudioPlayer_t *player)
{
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
                                           uint32_t loop_count)
{
  if (player == NULL)
    return HAL_ERROR;

  // DEBUG_INFO("AudioPlayer_PlayWithLoop Start");

  /* 重置状态 */
  player->current_pos = player->data_start_pos;
  f_lseek(&player->file, player->current_pos);
  player->loop_enable = (loop_count != 1);
  player->loop_count = loop_count;
  player->current_loop = 0;
  player->is_file_ended = 0;

  player->play_pos = 0;
  player->write_pos = 0;
  player->available_samples = 0;

  gp_player = player;

  /* 预填充：尽可能填满大缓冲区 */
  uint32_t prefill_count = 0;
  /* 只要没满且文件没读完，就一直读 */
  /* 增加预填充次数限制，防止大文件卡死，但要保证足够多 */
  while (player->available_samples < player->buffer_size && prefill_count < 1000)
  {
    /* 预填充时传入 0，强制填充，避免空转 */
    AudioPlayer_RefillRingBuffer(player, 0);

    if (player->is_file_ended)
      break; // 如果文件很短，读完就退
    prefill_count++;
  }



  /* 预填充 DMA 小缓冲区 */
  memset(s_dma_buffer, 0, sizeof(s_dma_buffer));
  player_event_callback(0);
  player_event_callback(1);

  if (DSPEAKER_Start() == HAL_OK)
  {
    player->is_playing = 1;
    // DEBUG_INFO("Playback Started");
    return HAL_OK;
  }
  DEBUG_ERROR("DSPEAKER_Start Failed");
  return HAL_ERROR;
}

/**
 * @brief 开始播放（单次，不循环）
 */
HAL_StatusTypeDef AudioPlayer_Play(AudioPlayer_t *player)
{
  return AudioPlayer_PlayWithLoop(player, 1);
}

/**
 * @brief 停止播放
 */
void AudioPlayer_Stop(AudioPlayer_t *player)
{
  if (player == NULL)
    return;

  DSPEAKER_Stop();
  /* 修正：传入 NULL 来注销回调 */
  DSPEAKER_RegisterEventCallback(NULL);

  player->is_playing = 0;
  gp_player = NULL;
  DEBUG_INFO("停止播放");
}

/**
 * @brief 获取播放状态
 */
uint8_t AudioPlayer_IsPlaying(AudioPlayer_t *player)
{
  if (player == NULL)
    return 0;

  return player->is_playing && (DSPEAKER_GetState() == DSPEAKER_STATE_PLAYING);
}

/**
 * @brief 获取当前循环次数
 */
uint32_t AudioPlayer_GetLoopCount(AudioPlayer_t *player)
{
  if (player == NULL)
    return 0;

  return player->current_loop + 1; /* 返回当前播放的循环次数 */
}

/**
 * @brief 此函数在 main while(1) 或 RTOS 任务中运行
 */
void AudioPlayer_Process(AudioPlayer_t *player)
{
  if (!player->is_playing)
    return;

  // /* 调试：每秒打印一次状态，检查是否卡死 */
  // static uint32_t last_debug_tick = 0;
  // if (HAL_GetTick() - last_debug_tick > 1000)
  // {
  //   last_debug_tick = HAL_GetTick();
  //   char buf[128];
  //   snprintf(buf, sizeof(buf), "Audio: Avail=%lu, Wr=%lu, Pl=%lu, Ended=%d",
  //            player->available_samples, player->write_pos, player->play_pos, player->is_file_ended);
  //   DEBUG_INFO(buf);
  // }

  /* 如果文件已经读完 (is_file_ended)，且缓冲区数据已被 ISR 播完 (available_samples == 0) */
  if (player->is_file_ended && player->available_samples == 0)
  {
    AudioPlayer_Stop(player);
    return;
  }

  /* 只要大缓冲区不满，就尝试填充 */
  if (player->available_samples < player->buffer_size - 4096)
  {
    AudioPlayer_RefillRingBuffer(player, 4096);
  }
  else
  {
    /* 缓冲区满，等待事件 */
    AUDIO_PLAYER_WAIT_EVENT();
  }
}

#endif
