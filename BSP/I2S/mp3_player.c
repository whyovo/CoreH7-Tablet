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

/* DMA 内部小缓冲区大小 (采样点数)  */
#define AUDIO_DMA_BUFFER_SIZE 4096

/* 全局播放器实例指针 */
static MP3Player_t *gp_mp3_player = NULL;

/* DMA 专用小缓冲区 (必须 32 字节对齐以适配 Cache 操作) */
static int16_t s_mp3_dma_buffer[AUDIO_DMA_BUFFER_SIZE] __attribute__((aligned(32)));

/**
 * @brief 内部函数：从文件读取数据并解码填充到 PCM 环形缓冲区
 * @param player 播放器实例
 * @param min_threshold 最小填充阈值
 */
static void MP3Player_RefillRingBuffer(MP3Player_t *player)
{
    /* 1. 检查 PCM 缓冲区空间 */
    __disable_irq();
    uint32_t available = player->available_samples;
    __enable_irq();

    uint32_t space = player->pcm_buffer_size - available;

    /* 如果空间不足以存放一帧解码数据，则不解码 */
    if (space < MINIMP3_MAX_SAMPLES_PER_FRAME * 2)
        return;

    /* 2. 确保读取缓冲区有足够数据 */
    /* 设定一个阈值，比如 4KB。当缓冲区剩余数据小于此值时，搬运数据并读取新数据 */
    /* 这样对于大缓冲区 (如 128KB)，只有在用完时才进行一次 memmove 和 f_read */
    if (player->bytes_in_read_buf < MP3_READ_CHUNK_SIZE && !player->is_file_ended)
    {
        /* 将剩余数据移到缓冲区头部 */
        if (player->bytes_in_read_buf > 0)
        {
            /* 使用 read_offset 定位当前有效数据的起始位置 */
            memmove(player->read_buffer,
                    &player->read_buffer[player->read_offset],
                    player->bytes_in_read_buf);
        }

        /* 重置偏移量 */
        player->read_offset = 0;

        /* 计算需要读取的字节数 (填满整个缓冲区) */
        UINT bytes_to_read = player->read_buffer_size - player->bytes_in_read_buf;
        UINT bytes_read = 0;

        /* 读取文件 */
        f_read(&player->file, &player->read_buffer[player->bytes_in_read_buf], bytes_to_read, &bytes_read);

        player->bytes_in_read_buf += bytes_read;
        player->current_file_pos += bytes_read;

        if (bytes_read < bytes_to_read)
        {
            /* 文件读完了 */
            if (player->loop_enable)
            {
                if (player->loop_count == 0 || player->current_loop < player->loop_count - 1)
                {
                    /* 准备循环：重置文件指针 */
                    if (player->loop_count != 0)
                        player->current_loop++;

                    /* 跳过 ID3 标签 */
                    f_lseek(&player->file, player->data_start_offset);
                    player->current_file_pos = player->data_start_offset;
                    /* 下次循环会继续读取 */
                }
                else
                {
                    player->is_file_ended = 1;
                }
            }
            else
            {
                player->is_file_ended = 1;
            }
        }
    }

    /* 如果没有数据可解了，直接返回 */
    if (player->bytes_in_read_buf == 0)
        return;

    /* 3. 解码一帧 */
    int16_t pcm_frame[MINIMP3_MAX_SAMPLES_PER_FRAME];

    /* 传入指针加上偏移量 */
    int samples = mp3dec_decode_frame(&player->mp3d,
                                      &player->read_buffer[player->read_offset],
                                      player->bytes_in_read_buf,
                                      pcm_frame,
                                      &player->frame_info);

    /* 更新读取缓冲区状态 */
    if (samples > 0)
    {
        /* 只更新偏移量和剩余计数，不移动内存，提高效率 */
        player->read_offset += player->frame_info.frame_bytes;
        player->bytes_in_read_buf -= player->frame_info.frame_bytes;

        /* 4. 写入 PCM 环形缓冲区 (保持原有逻辑) */
        uint32_t total_samples = samples * player->frame_info.channels;

        /* 单声道转立体声处理 */
        if (player->frame_info.channels == 1)
        {
            /* 扩展为双声道 */
            int16_t stereo_frame[MINIMP3_MAX_SAMPLES_PER_FRAME * 2];
            for (int i = 0; i < samples; i++)
            {
                stereo_frame[i * 2] = pcm_frame[i];
                stereo_frame[i * 2 + 1] = pcm_frame[i];
            }
            total_samples *= 2;

            /* 写入逻辑 */
            if (player->write_pos + total_samples <= player->pcm_buffer_size)
            {
                memcpy(&player->pcm_buffer[player->write_pos], stereo_frame, total_samples * sizeof(int16_t));

                SCB_CleanDCache_by_Addr((uint32_t *)&player->pcm_buffer[player->write_pos], total_samples * sizeof(int16_t));
                player->write_pos = (player->write_pos + total_samples) % player->pcm_buffer_size;
            }
            else
            {
                uint32_t first_part = player->pcm_buffer_size - player->write_pos;
                memcpy(&player->pcm_buffer[player->write_pos], stereo_frame, first_part * sizeof(int16_t));

                SCB_CleanDCache_by_Addr((uint32_t *)&player->pcm_buffer[player->write_pos], first_part * sizeof(int16_t));

                memcpy(player->pcm_buffer, &stereo_frame[first_part], (total_samples - first_part) * sizeof(int16_t));

                SCB_CleanDCache_by_Addr((uint32_t *)player->pcm_buffer, (total_samples - first_part) * sizeof(int16_t));

                player->write_pos = (player->write_pos + total_samples) % player->pcm_buffer_size;
            }
        }
        else
        {
            /* 已经是立体声，直接写入 */
            if (player->write_pos + total_samples <= player->pcm_buffer_size)
            {
                memcpy(&player->pcm_buffer[player->write_pos], pcm_frame, total_samples * sizeof(int16_t));

                SCB_CleanDCache_by_Addr((uint32_t *)&player->pcm_buffer[player->write_pos], total_samples * sizeof(int16_t));
                player->write_pos = (player->write_pos + total_samples) % player->pcm_buffer_size;
            }
            else
            {
                uint32_t first_part = player->pcm_buffer_size - player->write_pos;
                memcpy(&player->pcm_buffer[player->write_pos], pcm_frame, first_part * sizeof(int16_t));

                SCB_CleanDCache_by_Addr((uint32_t *)&player->pcm_buffer[player->write_pos], first_part * sizeof(int16_t));

                memcpy(player->pcm_buffer, &pcm_frame[first_part], (total_samples - first_part) * sizeof(int16_t));

                SCB_CleanDCache_by_Addr((uint32_t *)player->pcm_buffer, (total_samples - first_part) * sizeof(int16_t));

                player->write_pos = (player->write_pos + total_samples) % player->pcm_buffer_size;
            }
        }

        __disable_irq();
        player->available_samples += total_samples;
        __enable_irq();

        player->current_sample += samples;
    }
    else if (player->frame_info.frame_bytes > 0)
    {
        /* 有帧头但没样本，跳过 */
        player->read_offset += player->frame_info.frame_bytes;
        player->bytes_in_read_buf -= player->frame_info.frame_bytes;
    }
    else
    {
        /* 数据不足以解析一帧 */
        if (player->is_file_ended && player->bytes_in_read_buf > 0)
        {
            player->bytes_in_read_buf = 0;
        }
    }
}

/**
 * @brief DMA 回调：从 PCM 环形缓冲搬运到 DMA 双缓冲
 */
static void mp3_player_event_callback(uint8_t event)
{
    if (gp_mp3_player == NULL || !gp_mp3_player->is_playing)
        return;

    uint32_t dma_offset = (event == 0) ? 0 : (AUDIO_DMA_BUFFER_SIZE / 2);
    uint32_t len = AUDIO_DMA_BUFFER_SIZE / 2;

    /* 计算实际可用数据 */
    uint32_t valid_samples = gp_mp3_player->available_samples;
    if (valid_samples > len)
        valid_samples = len;

    if (valid_samples > 0)
    {
        uint32_t samples_to_end = gp_mp3_player->pcm_buffer_size - gp_mp3_player->play_pos;

        if (samples_to_end >= valid_samples)
        {
            memcpy(&s_mp3_dma_buffer[dma_offset], &gp_mp3_player->pcm_buffer[gp_mp3_player->play_pos], valid_samples * sizeof(int16_t));
            gp_mp3_player->play_pos += valid_samples;
        }
        else
        {
            memcpy(&s_mp3_dma_buffer[dma_offset], &gp_mp3_player->pcm_buffer[gp_mp3_player->play_pos], samples_to_end * sizeof(int16_t));
            memcpy(&s_mp3_dma_buffer[dma_offset + samples_to_end], gp_mp3_player->pcm_buffer, (valid_samples - samples_to_end) * sizeof(int16_t));
            gp_mp3_player->play_pos = valid_samples - samples_to_end;
        }

        if (gp_mp3_player->play_pos >= gp_mp3_player->pcm_buffer_size)
            gp_mp3_player->play_pos = 0;

        __disable_irq();
        gp_mp3_player->available_samples -= valid_samples;
        __enable_irq();
    }

    /* 补零 */
    if (valid_samples < len)
    {
        memset(&s_mp3_dma_buffer[dma_offset + valid_samples], 0, (len - valid_samples) * sizeof(int16_t));
    }

    /* 音量处理 */
    uint8_t vol = DSPEAKER_GetVolume();
    if (vol < 100)
    {
        int16_t *target = &s_mp3_dma_buffer[dma_offset];
        for (uint32_t i = 0; i < len; i++)
        {
            target[i] = (int16_t)((target[i] * vol) / 100);
        }
    }

    /* 刷新 Cache */
    SCB_CleanDCache_by_Addr((uint32_t *)&s_mp3_dma_buffer[dma_offset], len * sizeof(int16_t));

    /* 通知主循环 */
    MP3_PLAYER_NOTIFY_EVENT();
}

/**
 * @brief 初始化播放器
 */
void MP3Player_Init(MP3Player_t *player, int16_t *pcm_buffer, uint32_t pcm_size, uint8_t *read_buffer, uint32_t read_size)
{
    if (player == NULL)
        return;

    memset(player, 0, sizeof(MP3Player_t));

    /* 绑定 PCM 缓冲区 */
    player->pcm_buffer = pcm_buffer;
    player->pcm_buffer_size = pcm_size;

    /* 绑定 读取 缓冲区 */
    player->read_buffer = read_buffer;
    player->read_buffer_size = read_size;

    /* 初始化 DSPEAKER，使用内部小缓冲区 */
    DSPEAKER_Init(s_mp3_dma_buffer, AUDIO_DMA_BUFFER_SIZE);
    DSPEAKER_RegisterEventCallback(mp3_player_event_callback);
}

/**
 * @brief 打开并解析 MP3 文件
 */
HAL_StatusTypeDef MP3Player_OpenFile(MP3Player_t *player, const char *filename)
{
    char buf[64];

    if (player == NULL || filename == NULL)
        return HAL_ERROR;

    /* 打开文件 */
    FRESULT res = f_open(&player->file, filename, FA_READ);
    if (res != FR_OK)
    {
        snprintf(buf, sizeof(buf), "打开 MP3 失败: %d", res);
        DEBUG_ERROR(buf);
        return HAL_ERROR;
    }

    player->file_size = f_size(&player->file);
    player->current_file_pos = 0;
    player->bytes_in_read_buf = 0;
    player->read_offset = 0; // 重置偏移
    player->is_file_ended = 0;
    player->data_start_offset = 0; // 默认从头开始

    /* === ID3v2 标签检测与跳过 === */
    UINT br;
    uint8_t id3_header[10];
    f_read(&player->file, id3_header, 10, &br);

    if (br == 10 && memcmp(id3_header, "ID3", 3) == 0)
    {
        /* 计算标签大小 (Synchsafe integers: 4 bytes, 7 bits each) */
        uint32_t tag_size = ((id3_header[6] & 0x7F) << 21) |
                            ((id3_header[7] & 0x7F) << 14) |
                            ((id3_header[8] & 0x7F) << 7) |
                            (id3_header[9] & 0x7F);

        /* 加上头部的 10 字节 */
        player->data_start_offset = tag_size + 10;
        snprintf(buf, sizeof(buf), "检测到 ID3v2, 偏移: %lu", player->data_start_offset);
        DEBUG_INFO(buf);
    }

    /* 定位到音频数据开始处 */
    f_lseek(&player->file, player->data_start_offset);
    player->current_file_pos = player->data_start_offset;

    /* 初始化解码器 */
    mp3dec_init(&player->mp3d);

    /* 预读一小段来检测格式 */
    /*使用 read_buffer_size 或 4KB，取较小值，避免读太多 */
    UINT read_chunk = (player->read_buffer_size > 4096) ? 4096 : player->read_buffer_size;
    f_read(&player->file, player->read_buffer, read_chunk, &br);

    player->bytes_in_read_buf = br;
    player->read_offset = 0;
    player->current_file_pos += br;

    /* 尝试解码一帧获取信息 */
    int16_t pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
    /* 使用偏移量 */
    int samples = mp3dec_decode_frame(&player->mp3d, player->read_buffer, br, pcm, &player->frame_info);

    if (samples > 0)
    {
        player->sample_rate = player->frame_info.hz;
        player->channels = player->frame_info.channels;
        snprintf(buf, sizeof(buf), "MP3: %dHz %dch %dkbps", player->sample_rate, player->channels, player->frame_info.bitrate_kbps);
        DEBUG_INFO(buf);
    }
    else
    {
        DEBUG_ERROR("MP3 格式检测失败，尝试继续");
        /* 即使检测失败，也尝试重置状态准备播放，万一只是第一帧坏了 */
    }

    /* 重置文件指针到音频数据开头，准备正式播放 */
    f_lseek(&player->file, player->data_start_offset);
    player->current_file_pos = player->data_start_offset;
    player->bytes_in_read_buf = 0;
    player->read_offset = 0;
    mp3dec_init(&player->mp3d); // 重置解码器状态

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

    f_close(&player->file);
    gp_mp3_player = NULL;
}

/**
 * @brief 开始播放（支持循环）
 */
HAL_StatusTypeDef MP3Player_PlayWithLoop(MP3Player_t *player, uint32_t loop_count)
{
    if (player == NULL || player->pcm_buffer == NULL)
        return HAL_ERROR;

    /* 重置播放状态 */
    player->play_pos = 0;
    player->write_pos = 0;
    player->available_samples = 0;
    player->current_sample = 0;
    player->is_file_ended = 0;

    /* 重置文件读取 */
    f_lseek(&player->file, player->data_start_offset);
    player->current_file_pos = player->data_start_offset;
    player->bytes_in_read_buf = 0;
    player->read_offset = 0;
    mp3dec_init(&player->mp3d);

    /* 设置循环 */
    player->loop_enable = (loop_count != 1);
    player->loop_count = loop_count;
    player->current_loop = 0;

    player->state = MP3_STATE_PLAYING;
    gp_mp3_player = player;

    /* 预填充 PCM 缓冲区 (解码一些数据) */
    uint32_t prefill_count = 0;
    while (player->available_samples < player->pcm_buffer_size / 2 && prefill_count < 50)
    {
        MP3Player_RefillRingBuffer(player);
        if (player->is_file_ended)
            break;
        prefill_count++;
    }

    /* 预填充 DMA 缓冲区 */
    memset(s_mp3_dma_buffer, 0, sizeof(s_mp3_dma_buffer));
    mp3_player_event_callback(0);
    mp3_player_event_callback(1);

    /* 启动硬件 */
    DSPEAKER_RegisterEventCallback(mp3_player_event_callback);
    if (DSPEAKER_Start() == HAL_OK)
    {
        player->is_playing = 1;
        return HAL_OK;
    }

    return HAL_ERROR;
}

HAL_StatusTypeDef MP3Player_Play(MP3Player_t *player)
{
    return MP3Player_PlayWithLoop(player, 1);
}

void MP3Player_Stop(MP3Player_t *player)
{
    if (player == NULL)
        return;

    DSPEAKER_Stop();
    DSPEAKER_RegisterEventCallback(NULL);
    player->is_playing = 0;
    player->state = MP3_STATE_IDLE;
    gp_mp3_player = NULL;
}

void MP3Player_Pause(MP3Player_t *player)
{
    if (player == NULL || !player->is_playing)
        return;
    DSPEAKER_Stop();
    player->is_playing = 0;
    player->state = MP3_STATE_PAUSED;
}

void MP3Player_Resume(MP3Player_t *player)
{
    if (player == NULL || player->state != MP3_STATE_PAUSED)
        return;
    gp_mp3_player = player;
    DSPEAKER_RegisterEventCallback(mp3_player_event_callback);
    DSPEAKER_Start();
    player->is_playing = 1;
    player->state = MP3_STATE_PLAYING;
}

uint8_t MP3Player_IsPlaying(MP3Player_t *player)
{
    return player && player->is_playing;
}

uint32_t MP3Player_GetLoopCount(MP3Player_t *player)
{
    return player ? player->current_loop + 1 : 0;
}

uint32_t MP3Player_GetCurrentSample(MP3Player_t *player)
{
    return player ? player->current_sample : 0;
}

uint32_t MP3Player_GetTotalSamples(MP3Player_t *player)
{
    /* 估算值：总大小(字节) * 8 / (码率(kbps) * 1000) * 采样率 */
    if (!player || player->sample_rate == 0 || player->frame_info.bitrate_kbps == 0)
        return 0;

    // 使用 bitrate_kbps，并注意单位转换
    return (uint64_t)player->file_size * 8 * player->sample_rate / (player->frame_info.bitrate_kbps * 1000);
}

/**
 * @brief MP3 处理主任务
 */
void MP3Player_Process(MP3Player_t *player)
{
    if (player == NULL || !player->is_playing)
        return;

    /* 如果文件结束且缓冲区空，停止播放 */
    if (player->is_file_ended && player->available_samples == 0)
    {
        MP3Player_Stop(player);
        return;
    }

    /* 只要 PCM 缓冲区不满，就尝试解码填充 */
    /* 限制单次循环解码次数，防止阻塞太久 */
    int decode_ops = 0;
    while (player->available_samples < player->pcm_buffer_size - MINIMP3_MAX_SAMPLES_PER_FRAME * 2)
    {
        MP3Player_RefillRingBuffer(player);

        decode_ops++;
        if (decode_ops > 5)
            break; /* 每次 Process 最多解 5 帧，避免卡顿 */

        if (player->is_file_ended)
            break;
    }

    /* 如果缓冲区满了，挂起等待 ISR 唤醒 */
    if (player->available_samples >= player->pcm_buffer_size - MINIMP3_MAX_SAMPLES_PER_FRAME * 4)
    {
        MP3_PLAYER_WAIT_EVENT();
    }
}

#endif /* DSPEAKER_ENABLE */
