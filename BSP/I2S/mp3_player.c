/**
 ******************************************************************************
 * @file    mp3_player.c
 * @author  菜菜why(B站:菜菜whyy)
 * @brief   MP3 文件播放驱动实现
 ******************************************************************************
 */


#define MINIMP3_IMPLEMENTATION

#include "mp3_player.h"
#include <stdio.h>
#include <string.h>

#if (defined(DSPEAKER_ENABLE) && defined(MP3_PLAY_ENABLE))

#include "minimp3/minimp3_ex.h"

/* MP3 解码缓冲区大小（字节） */
#define MP3_DECODE_BUF_SIZE (16 * 1024)

/* PCM 环形缓冲区大小（采样点数） */
#ifndef MP3_PCM_BUFFER_SIZE
#define MP3_PCM_BUFFER_SIZE (44100 * 2) /* 立体声，1秒钟 */
#endif

/* 全局播放器实例指针 */
static MP3Player_t *gp_mp3_player = NULL;

/**
 * @brief MP3 数据读取回调（DMA 中断中调用）
 */
static uint32_t mp3_data_callback(int16_t *pBuffer, uint32_t size,
                                  uint8_t isFirstHalf)
{
    if (gp_mp3_player == NULL || !gp_mp3_player->is_playing)
        return size;

    uint32_t samples_to_copy = 0;
    // uint32_t i; /* 移除未使用的变量 */

    /* 检查 PCM 缓冲中有多少数据可用 */
    if (gp_mp3_player->pcm_data_len == 0)
    {
        /* PCM 缓冲为空，需要解码更多数据 */
        /* 这在高优先级中断中不好做，所以这里只返回静音 */
        memset(pBuffer, 0, size * sizeof(int16_t));
        return size;
    }

    /* 从 PCM 缓冲中复制数据到 DMA 缓冲 */
    samples_to_copy = (gp_mp3_player->pcm_data_len > size)
                          ? size
                          : gp_mp3_player->pcm_data_len;

    /* 处理环形缓冲区读取 */
    if (gp_mp3_player->pcm_read_pos + samples_to_copy <=
        gp_mp3_player->pcm_buffer_size)
    {
        /* 不跨越边界 */
        memcpy(pBuffer, &gp_mp3_player->pcm_buffer[gp_mp3_player->pcm_read_pos],
               samples_to_copy * sizeof(int16_t));
        gp_mp3_player->pcm_read_pos =
            (gp_mp3_player->pcm_read_pos + samples_to_copy) %
            gp_mp3_player->pcm_buffer_size;
    }
    else
    {
        /* 跨越边界 */
        uint32_t first_part = gp_mp3_player->pcm_buffer_size -
                              gp_mp3_player->pcm_read_pos;
        memcpy(pBuffer, &gp_mp3_player->pcm_buffer[gp_mp3_player->pcm_read_pos],
               first_part * sizeof(int16_t));
        memcpy(&pBuffer[first_part], gp_mp3_player->pcm_buffer,
               (samples_to_copy - first_part) * sizeof(int16_t));
        gp_mp3_player->pcm_read_pos =
            (gp_mp3_player->pcm_read_pos + samples_to_copy) %
            gp_mp3_player->pcm_buffer_size;
    }

    gp_mp3_player->pcm_data_len -= samples_to_copy;
    gp_mp3_player->current_sample += samples_to_copy;

    /* 填充剩余部分为 0 */
    if (samples_to_copy < size)
    {
        memset(&pBuffer[samples_to_copy], 0,
               (size - samples_to_copy) * sizeof(int16_t));
    }

    return size;
}

/**
 * @brief 打开并解析 MP3 文件
 */
HAL_StatusTypeDef MP3Player_OpenFile(MP3Player_t *player,
                                     const char *filename)
{
    mp3dec_t mp3d;
    mp3dec_frame_info_t frame_info;
    UINT bytes_read;
    // uint32_t total_samples = 0; /* 移除未使用的局部变量 */
    int samples;
    char buf[256];

    if (player == NULL || filename == NULL)
        return HAL_ERROR;

    /* 打开文件 */
    FRESULT res = f_open(&player->file, filename, FA_READ);
    if (res != FR_OK)
    {
        snprintf(buf, sizeof(buf), "打开 MP3 文件失败: %s", filename);
        DEBUG_ERROR(buf);
        return HAL_ERROR;
    }

    /* 获取文件大小 */
    player->file_size = f_size(&player->file);
    if (player->file_size == 0)
    {
        DEBUG_ERROR("MP3 文件大小为 0");
        f_close(&player->file);
        return HAL_ERROR;
    }

    /* 检查文件大小是否超过 SDRAM 缓冲区 */
    if (player->file_size > MP3_FILE_BUFFER_SIZE)
    {
        snprintf(buf, sizeof(buf), "MP3 文件过大: %u KB > %u KB",
                 player->file_size / 1024, MP3_FILE_BUFFER_SIZE / 1024);
        DEBUG_ERROR(buf);
        f_close(&player->file);
        return HAL_ERROR;
    }

    /* 使用 SDRAM 中预分配的缓冲区（不使用 malloc） */
    player->file_buffer = (uint8_t *)MP3_FILE_BUFFER_ADDR;

    /* 读取整个文件到 SDRAM */
    res = f_read(&player->file, player->file_buffer, player->file_size,
                 &bytes_read);
    if (res != FR_OK || bytes_read != player->file_size)
    {
        DEBUG_ERROR("读取 MP3 文件失败");
        f_close(&player->file);
        return HAL_ERROR;
    }

    /* 关闭文件（已读入内存）*/
    f_close(&player->file);

    snprintf(buf, sizeof(buf), "MP3 文件读入 SDRAM: %u KB", player->file_size / 1024);
    DEBUG_INFO(buf);

    /* 初始化 MP3 解码器 */
    mp3dec_init(&mp3d);

    /* 解析：读取第一帧获取采样率和声道 */
    int frame_size = 0;
    int free_format_bytes = 0;
    const uint8_t *frame_ptr = player->file_buffer;
    int sample_rate = 0;
    int channels = 0;

    for (int i = 0; i < player->file_size - 4; i++)
    {
        frame_size = 0;
        int j = mp3d_find_frame(&frame_ptr[i], player->file_size - i,
                                &free_format_bytes, &frame_size);
        if (frame_size > 0)
        {
            frame_ptr = &frame_ptr[i + j];
            samples = mp3dec_decode_frame(&mp3d, frame_ptr, frame_size, NULL, &frame_info);
            if (samples > 0)
            {
                sample_rate = frame_info.hz;
                channels = frame_info.channels;
                player->channels = channels;
                player->sample_rate = sample_rate;
                snprintf(buf, sizeof(buf), "MP3: %d Hz, %d 声道", sample_rate, channels);
                DEBUG_INFO(buf);
                break;
            }
        }
    }

    if (sample_rate == 0)
    {
        DEBUG_ERROR("无法读取 MP3 信息");
        return HAL_ERROR;
    }

    /* 计算实际需要的 PCM 缓冲大小 */
    /* 使用固定的 512KB SDRAM 缓冲 */
    uint32_t pcm_buffer_size = MP3_PCM_BUFFER_SIZE / sizeof(int16_t); /* 转换为采样点数 */

    player->pcm_buffer_size = pcm_buffer_size;
    player->pcm_buffer = (int16_t *)MP3_PCM_BUFFER_ADDR;

    snprintf(buf, sizeof(buf), "PCM 缓冲区: %u 采样点 (%u KB)",
             player->pcm_buffer_size,
             player->pcm_buffer_size * sizeof(int16_t) / 1024);
    DEBUG_INFO(buf);

    /* 计算总采样数（粗略估计） */
    player->total_samples =
        (uint64_t)player->file_size * player->sample_rate * 8 / (128 * 1024);

    player->pcm_read_pos = 0;
    player->pcm_write_pos = 0;
    player->pcm_data_len = 0;
    player->current_sample = 0;
    player->is_playing = 0;
    player->loop_enable = 0;
    player->loop_count = 1;
    player->current_loop = 0;
    player->state = MP3_STATE_IDLE;

    return HAL_OK;
}

/**
 * @brief 关闭 MP3 文件
 */
void MP3Player_CloseFile(MP3Player_t *player)
{
    if (player == NULL)
        return;

    if (player->is_playing)
    {
        MP3Player_Stop(player);
    }

    /* 注意：不释放 SDRAM 缓冲区（它是静态的） */
    f_close(&player->file);
    gp_mp3_player = NULL;
}

/**
 * @brief 开始播放（支持循环）
 */
HAL_StatusTypeDef MP3Player_PlayWithLoop(MP3Player_t *player,
                                         uint32_t loop_count)
{
    HAL_StatusTypeDef status;

    if (player == NULL || player->file_buffer == NULL)
        return HAL_ERROR;

    /* 重置播放位置 */
    player->pcm_read_pos = 0;
    player->pcm_write_pos = 0;
    player->pcm_data_len = 0;
    player->current_sample = 0;

    /* 设置循环参数 */
    if (loop_count == 0)
    {
        player->loop_enable = 1;
        player->loop_count = 0; /* 无限循环 */
    }
    else if (loop_count == 1)
    {
        player->loop_enable = 0; /* 单次播放 */
    }
    else
    {
        player->loop_enable = 1;
        player->loop_count = loop_count;
    }

    player->current_loop = 0;
    player->state = MP3_STATE_PLAYING;

    /* 注册全局指针和回调 */
    gp_mp3_player = player;
    DSPEAKER_RegisterCallback(mp3_data_callback);

    /* 启动扬声器 */
    status = DSPEAKER_Start();
    if (status == HAL_OK)
    {
        player->is_playing = 1;
    }
    else
    {
        DEBUG_ERROR("扬声器启动失败");
        player->state = MP3_STATE_ERROR;
    }

    return status;
}

/**
 * @brief 开始播放（单次）
 */
HAL_StatusTypeDef MP3Player_Play(MP3Player_t *player)
{
    return MP3Player_PlayWithLoop(player, 1);
}

/**
 * @brief 停止播放
 */
void MP3Player_Stop(MP3Player_t *player)
{
    if (player == NULL)
        return;

    DSPEAKER_Stop();
    DSPEAKER_UnregisterCallback();
    player->is_playing = 0;
    player->state = MP3_STATE_IDLE;
    gp_mp3_player = NULL;
}

/**
 * @brief 暂停播放
 */
void MP3Player_Pause(MP3Player_t *player)
{
    if (player == NULL || !player->is_playing)
        return;

    DSPEAKER_Stop();
    player->is_playing = 0;
    player->state = MP3_STATE_PAUSED;
}

/**
 * @brief 恢复播放
 */
void MP3Player_Resume(MP3Player_t *player)
{
    if (player == NULL || player->state != MP3_STATE_PAUSED)
        return;

    gp_mp3_player = player;
    DSPEAKER_RegisterCallback(mp3_data_callback);
    DSPEAKER_Start();
    player->is_playing = 1;
    player->state = MP3_STATE_PLAYING;
}

/**
 * @brief 获取播放状态
 */
uint8_t MP3Player_IsPlaying(MP3Player_t *player)
{
    if (player == NULL)
        return 0;

    return player->is_playing && (DSPEAKER_GetState() == DSPEAKER_STATE_PLAYING);
}

/**
 * @brief 获取当前循环次数
 */
uint32_t MP3Player_GetLoopCount(MP3Player_t *player)
{
    if (player == NULL)
        return 0;

    return player->current_loop + 1;
}

/**
 * @brief 获取播放进度（采样点）
 */
uint32_t MP3Player_GetCurrentSample(MP3Player_t *player)
{
    if (player == NULL)
        return 0;

    return player->current_sample;
}

/**
 * @brief 获取总采样点数
 */
uint32_t MP3Player_GetTotalSamples(MP3Player_t *player)
{
    if (player == NULL)
        return 0;

    return player->total_samples;
}

/**
 * @brief MP3 解码后台任务（需要在主循环中调用）
 * @note 此函数解码 MP3 文件并填充 PCM 缓冲区
 */
void MP3Player_DecodeTask(MP3Player_t *player)
{
    static mp3dec_t mp3d_static = {0};
    static uint32_t file_offset = 0;
    static int is_initialized = 0;

    if (player == NULL || !player->is_playing || player->file_buffer == NULL)
        return;

    /* 初始化解码器 */
    if (!is_initialized)
    {
        mp3dec_init(&mp3d_static);
        file_offset = 0;
        is_initialized = 1;
    }

    /* 如果 PCM 缓冲还有足够数据，不需要解码 */
    if (player->pcm_data_len > player->pcm_buffer_size / 2)
        return;

    /* 解码一帧 */
    mp3dec_frame_info_t frame_info;
    int16_t pcm_buf[MINIMP3_MAX_SAMPLES_PER_FRAME];

    int samples = mp3dec_decode_frame(&mp3d_static, &player->file_buffer[file_offset],
                                      player->file_size - file_offset, pcm_buf,
                                      &frame_info);

    if (samples > 0)
    {
        file_offset += frame_info.frame_bytes;

        /* 写入 PCM 缓冲 */
        uint32_t write_samples = samples * frame_info.channels;

        if (player->pcm_write_pos + write_samples <= player->pcm_buffer_size)
        {
            /* 不跨越边界 */
            memcpy(&player->pcm_buffer[player->pcm_write_pos], pcm_buf,
                   write_samples * sizeof(int16_t));
            player->pcm_write_pos =
                (player->pcm_write_pos + write_samples) % player->pcm_buffer_size;
        }
        else
        {
            /* 跨越边界 */
            uint32_t first_part =
                player->pcm_buffer_size - player->pcm_write_pos;
            memcpy(&player->pcm_buffer[player->pcm_write_pos], pcm_buf,
                   first_part * sizeof(int16_t));
            memcpy(player->pcm_buffer, &pcm_buf[first_part],
                   (write_samples - first_part) * sizeof(int16_t));
            player->pcm_write_pos =
                (player->pcm_write_pos + write_samples) % player->pcm_buffer_size;
        }

        player->pcm_data_len += write_samples;
    }
    else
    {
        /* 文件结束或解码出错 */
        if (player->loop_enable)
        {
            if (player->loop_count == 0)
            {
                /* 无限循环 */
                file_offset = 0;
                mp3dec_init(&mp3d_static);
                player->current_loop++;
            }
            else if (player->current_loop < player->loop_count - 1)
            {
                /* 继续循环 */
                file_offset = 0;
                mp3dec_init(&mp3d_static);
                player->current_loop++;
            }
            else
            {
                /* 播放完成 */
                player->is_playing = 0;
                player->state = MP3_STATE_IDLE;
                is_initialized = 0;
            }
        }
        else
        {
            player->is_playing = 0;
            player->state = MP3_STATE_IDLE;
            is_initialized = 0;
        }
    }
}

#endif /* DSPEAKER_ENABLE */
