/**
 ******************************************************************************
 * @file    mp3_player.h
 * @author  菜菜why(B站:菜菜whyy)
 * @brief   MP3 文件播放驱动
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

#if (defined(DSPEAKER_ENABLE) && defined(MP3_PLAY_ENABLE))
/* 引入 minimp3 头文件以获取 mp3dec_t 定义 */
#include "minimp3/minimp3.h"

/* ===== SDRAM 缓冲区定义 ===== */
/* SDRAM 起始地址：0xC0000000，总大小 16MB .使用0xC0800000开始的，防止冲突*/
#define SDRAM_BASE_ADDR 0xC0800000

/* MP3 文件缓冲：4MB（用于存储整个 MP3 文件） */
#define MP3_FILE_BUFFER_SIZE (4 * 1024 * 1024)
#define MP3_FILE_BUFFER_ADDR (SDRAM_BASE_ADDR)

/* PCM 缓冲：512KB（环形缓冲用于 DMA 播放） */
#define MP3_PCM_BUFFER_SIZE (512 * 1024)
#define MP3_PCM_BUFFER_ADDR (MP3_FILE_BUFFER_ADDR + MP3_FILE_BUFFER_SIZE)

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
        uint8_t *file_buffer; /* 整个文件缓冲区 */
        uint32_t file_size;
        int16_t *pcm_buffer; /* PCM 解码缓冲区 */
        uint32_t pcm_buffer_size;
        uint32_t pcm_read_pos;          /* PCM 缓冲读位置 */
        uint32_t pcm_write_pos;         /* PCM 缓冲写位置 */
        volatile uint32_t pcm_data_len; /* PCM 缓冲中有效数据长度 (volatile) */

        int channels;
        int sample_rate;
        uint32_t total_samples;
        uint32_t current_sample;

        uint8_t is_playing;
        uint8_t loop_enable;
        uint32_t loop_count;
        uint32_t current_loop;

        MP3_State state;
        int last_error;

        /* 解码器状态 */
        mp3dec_t mp3d;
        uint32_t decode_offset;
    } MP3Player_t;

    /* ===== 导出函数 ===== */

    /**
     * @brief 打开并解析 MP3 文件
     * @param player 播放器结构体指针
     * @param filename 文件路径（例："0:music.mp3"）
     * @return HAL_OK=成功, HAL_ERROR=失败
     */
    HAL_StatusTypeDef MP3Player_OpenFile(MP3Player_t *player,
                                         const char *filename);

    /**
     * @brief 关闭 MP3 文件
     */
    void MP3Player_CloseFile(MP3Player_t *player);

    /**
     * @brief 开始播放（支持循环）
     * @param player 播放器结构体指针
     * @param loop_count 循环次数（0=无限循环, 1=播放1次, 2=播放2次...）
     */
    HAL_StatusTypeDef MP3Player_PlayWithLoop(MP3Player_t *player,
                                             uint32_t loop_count);

    /**
     * @brief 开始播放（单次，不循环）
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
     * @brief 获取播放进度（采样点）
     */
    uint32_t MP3Player_GetCurrentSample(MP3Player_t *player);

    /**
     * @brief 获取总采样点数
     */
    uint32_t MP3Player_GetTotalSamples(MP3Player_t *player);

    /**
     * @brief MP3 解码后台任务（需要在主循环中调用）
     * @note 此函数解码 MP3 文件并填充 PCM 缓冲区
     */
    void MP3Player_DecodeTask(MP3Player_t *player);

#endif /* DSPEAKER_ENABLE */

#ifdef __cplusplus
}
#endif

#endif // MP3_PLAYER_H
