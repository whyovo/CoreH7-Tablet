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
extern "C"
{
#endif

#include "config.h"
#include "dspeaker.h"
#include "fatfs.h"
#include <stdint.h>

/* ================= RTOS 适配宏定义 ================= */
/*
 * 在这里选择你的运行环境。
 * 如果是裸机，保持默认即可。
 * 如果是 FreeRTOS，请取消注释下方的 FreeRTOS 部分，并确保 audio_task_handle 已定义。
 */

/*  模式 1: 裸机 (默认)  */
#if 1 // <--- 如果使用 RTOS，请改为 0

#define AUDIO_PLAYER_WAIT_EVENT() ((void)0)   /* 不等待，直接返回，由主循环轮询 */
#define AUDIO_PLAYER_NOTIFY_EVENT() ((void)0) /* 不通知，因为主循环一直在跑 */

/*  模式 2: FreeRTOS (任务通知方式 - 最高效) */
#else

#include "FreeRTOS.h"
#include "task.h"

/* 需在 main.c 或其他地方定义音频任务的句柄 */
extern TaskHandle_t audio_task_handle;

/*
 * 等待事件 (在 AudioPlayer_Process 中调用)
 * 作用：当缓冲区满时，挂起当前任务，释放 CPU 给其他任务
 */
#define AUDIO_PLAYER_WAIT_EVENT() ulTaskNotifyTake(pdTRUE, portMAX_DELAY)

/*
 * 通知事件 (在 ISR 中调用)
 * 作用：当 ISR 消耗了数据后，唤醒音频任务起来干活
 */
#define AUDIO_PLAYER_NOTIFY_EVENT()                                         \
  do                                                                        \
  {                                                                         \
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;                          \
    if (audio_task_handle != NULL)                                          \
    {                                                                       \
      vTaskNotifyGiveFromISR(audio_task_handle, &xHigherPriorityTaskWoken); \
      portYIELD_FROM_ISR(xHigherPriorityTaskWoken);                         \
    }                                                                       \
  } while (0)

#endif
  /* =================================================== */

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

    /* 缓冲区控制 */
    volatile int16_t *audio_buffer; /* 指向大缓冲区的指针 (SDRAM) */
    uint32_t buffer_size;  /* 缓冲区总大小 */

    /* 环形缓冲控制指针 */
    volatile uint32_t play_pos;          /* 播放指针：ISR 从这里取数据拷贝到 DMA */
    volatile uint32_t write_pos;         /* 写入指针 */
    volatile uint32_t available_samples; /* 有效数据量 */

    /* 文件读取结束标志 (SD卡读完了，但缓冲区可能还有数据) */
    volatile uint8_t is_file_ended;
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

  /**
   * @brief 初始化播放器并绑定缓冲区
   * @param buffer: 外部大缓冲区 (建议放在 SDRAM)
   * @param size: 缓冲区大小
   */
  void AudioPlayer_Init(AudioPlayer_t *player, int16_t *buffer, uint32_t size);

  /**
   * @brief 音频处理主任务 (需在 while(1) 或 RTOS 任务中不断调用)
   */
  void AudioPlayer_Process(AudioPlayer_t *player);
#endif
#ifdef __cplusplus
}
#endif

#endif // AUDIO_PLAYER_H
