/**
 ******************************************************************************
 * @file    mp3_player.h
 * @author  菜菜why(B站:菜菜whyy)
 * @brief   MP3 文件播放驱动 (流式读取版。目前有问题，MP3 读取缓冲区得大于整个文件才不会出错)
 *          支持 MP3 格式（通过 minimp3 库解码）
 ******************************************************************************
 */

#ifndef MP3_PLAYER_H
#define MP3_PLAYER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "config.h"
#include "dspeaker.h"
#include "fatfs.h"
#include <stdint.h>

/* ================= RTOS 适配宏定义 (仿照 audio_player.h) ================= */
#if 1 // <--- 如果使用 RTOS，请改为 0
#define MP3_PLAYER_WAIT_EVENT() ((void)0)
#define MP3_PLAYER_NOTIFY_EVENT() ((void)0)
#else
#include "FreeRTOS.h"
#include "task.h"
extern TaskHandle_t audio_task_handle; // 需在外部定义
#define MP3_PLAYER_WAIT_EVENT() ulTaskNotifyTake(pdTRUE, portMAX_DELAY)
#define MP3_PLAYER_NOTIFY_EVENT()                                                 \
    do                                                                            \
    {                                                                             \
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;                            \
        if (audio_task_handle != NULL)                                            \
        {                                                                         \
            vTaskNotifyGiveFromISR(audio_task_handle, &xHigherPriorityTaskWoken); \
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);                         \
        }                                                                         \
    } while (0)
#endif
    /* ======================================================================= */

#if (defined(DSPEAKER_ENABLE) && defined(MP3_PLAY_ENABLE))
/* 引入 minimp3 头文件以获取 mp3dec_t 定义 */
#include "minimp3/minimp3.h"
#define MP3_READ_CHUNK_SIZE (4 * 1024)

    /* MP3 播放器状态 */
    typedef enum
    {
        MP3_STATE_IDLE = 0,
        MP3_STATE_PLAYING,
        MP3_STATE_PAUSED,
        MP3_STATE_ERROR
    } MP3_State;

    /* MP3 播放器结构体 */
    typedef struct
    {
        FIL file;

        /* PCM 环形缓冲区 (用户提供, 存放解码后的 PCM 数据) */
        int16_t *pcm_buffer;
        uint32_t pcm_buffer_size; /* 采样点数 */

        /* 环形缓冲控制指针 */
        volatile uint32_t play_pos;          /* 播放指针 (ISR读取) */
        volatile uint32_t write_pos;         /* 写入指针 (解码写入) */
        volatile uint32_t available_samples; /* 有效数据量 */

        /* MP3 文件流式读取控制 */
        uint8_t *read_buffer;       /* 指向外部大缓冲区 (存放压缩的 MP3 数据) */
        uint32_t read_buffer_size;  /* 读取缓冲区大小 (字节) */
        uint32_t read_offset;       /* 当前解码在 read_buffer 中的偏移量 */
        uint32_t bytes_in_read_buf; /* read_buffer 中剩余有效字节数 */

        uint32_t file_size;
        uint32_t current_file_pos;  /* 当前文件读取位置 */
        uint32_t data_start_offset; /* 音频数据起始偏移 (跳过 ID3) */
        uint8_t is_file_ended;      /* 文件是否读完 */

        /* 音频信息 */
        int channels;
        int sample_rate;
        uint32_t total_samples;
        uint32_t current_sample;

        /* 播放控制 */
        uint8_t is_playing;
        uint8_t loop_enable;
        uint32_t loop_count;
        uint32_t current_loop;

        MP3_State state;

        /* 解码器状态 */
        mp3dec_t mp3d;
        mp3dec_frame_info_t frame_info;
    } MP3Player_t;

    /* ===== 导出函数 ===== */

    /**
     * @brief 初始化播放器并绑定缓冲区
     * @param player 播放器结构体指针
     * @param pcm_buffer PCM数据缓冲区 (建议放在 SDRAM)
     * @param pcm_size PCM缓冲区大小 (采样点数)
     * @param read_buffer MP3文件读取缓冲区 (建议放在 SDRAM)
     * @param read_size 读取缓冲区大小 (字节)
     */
    void MP3Player_Init(MP3Player_t *player, int16_t *pcm_buffer, uint32_t pcm_size, uint8_t *read_buffer, uint32_t read_size);

    /**
     * @brief 打开并解析 MP3 文件 (仅打开，不读取全部内容)
     */
    HAL_StatusTypeDef MP3Player_OpenFile(MP3Player_t *player, const char *filename);

    /**
     * @brief 关闭 MP3 文件
     */
    void MP3Player_CloseFile(MP3Player_t *player);

    /**
     * @brief 开始播放（支持循环）
     */
    HAL_StatusTypeDef MP3Player_PlayWithLoop(MP3Player_t *player, uint32_t loop_count);

    /**
     * @brief 开始播放（单次）
     */
    HAL_StatusTypeDef MP3Player_Play(MP3Player_t *player);

    /**
     * @brief 停止播放
     */
    void MP3Player_Stop(MP3Player_t *player);

    /**
     * @brief 暂停播放
     */
    void MP3Player_Pause(MP3Player_t *player);

    /**
     * @brief 恢复播放
     */
    void MP3Player_Resume(MP3Player_t *player);

    /**
     * @brief 获取播放状态
     */
    uint8_t MP3Player_IsPlaying(MP3Player_t *player);

    /**
     * @brief 获取当前循环次数
     */
    uint32_t MP3Player_GetLoopCount(MP3Player_t *player);

    /**
     * @brief MP3 处理主任务 (需在 while(1) 或 RTOS 任务中不断调用)
     *        负责读取文件、解码并填充 PCM 缓冲区
     */
    void MP3Player_Process(MP3Player_t *player);

#endif /* DSPEAKER_ENABLE */

#ifdef __cplusplus
}
#endif

#endif // MP3_PLAYER_H
