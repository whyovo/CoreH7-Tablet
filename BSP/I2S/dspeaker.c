/**
 ******************************************************************************
 * @file    dspeaker.c
 * @author  菜菜why(B站:菜菜whyy)
 * @brief   MAX98357A 数字扬声器 I2S DMA 驱动实现文件
 ******************************************************************************
 * @attention
 *
 * 实现说明：
 * 1. 使用 I2S 主机发送模式 + DMA 循环模式实现连续音频播放
 * 2. DMA 配置为循环模式，缓冲区分为前半/后半两部分
 * 3. 半传输中断处理后半缓冲，全传输中断处理前半缓冲
 * 4. 数据格式：I2S发送16位PCM数据
 * 5. MAX98357A 内置功率放大器，可直接驱动扬声器
 *
 * 数据流程：
 * 缓冲区 <- 用户数据/回调函数 <- DMA <- I2S外设 -> MAX98357A -> 扬声器
 *
 ******************************************************************************
 */

#include "dspeaker.h"
#include <string.h>

#ifdef DSPEAKER_ENABLE

/*******************************************************************************
 *                              私有变量
 ******************************************************************************/

/* 指向外部缓冲区的指针 */
static int16_t *gp_dma_buffer = NULL;
static uint32_t g_buffer_size = 0;
static DSPEAKER_EventCallback_t s_event_callback = NULL;

/* 状态变量 */
static DSPEAKER_State s_state = DSPEAKER_STATE_RESET;
static uint8_t s_volume = 1; /* 默认音量1% */

/* I2S 句柄引用（在 i2s.c 中定义） */
extern I2S_HandleTypeDef DSPEAKER_I2S_HANDLE;

/*******************************************************************************
 *                              导出函数实现
 ******************************************************************************/

/**
 * @brief  初始化数字扬声器
 */
HAL_StatusTypeDef DSPEAKER_Init(int16_t *pBuffer, uint32_t size)
{
    if (pBuffer == NULL || size == 0)
        return HAL_ERROR;

    gp_dma_buffer = pBuffer;
    g_buffer_size = size;
    s_state = DSPEAKER_STATE_READY;
    return HAL_OK;
}

/**
 * @brief  启动扬声器播放
 */
HAL_StatusTypeDef DSPEAKER_Start(void)
{
    HAL_StatusTypeDef status;

    if (s_state == DSPEAKER_STATE_PLAYING)
        return HAL_OK;
    if (s_state == DSPEAKER_STATE_RESET)
        return HAL_ERROR;

    /* 启动 I2S DMA 发送（循环模式） */
    status = HAL_I2S_Transmit_DMA(&DSPEAKER_I2S_HANDLE, (uint16_t *)gp_dma_buffer, g_buffer_size);
    if (status != HAL_OK)
    {
        DEBUG_ERROR("DSPEAKER_Start: I2S DMA 启动失败");
        s_state = DSPEAKER_STATE_ERROR;
        return status;
    }

    s_state = DSPEAKER_STATE_PLAYING;
    return HAL_OK;
}

/**
 * @brief  停止扬声器播放
 */
HAL_StatusTypeDef DSPEAKER_Stop(void)
{
    if (s_state != DSPEAKER_STATE_PLAYING)
        return HAL_OK;

    HAL_I2S_DMAStop(&DSPEAKER_I2S_HANDLE);
    s_state = DSPEAKER_STATE_READY;
    return HAL_OK;
}

DSPEAKER_State DSPEAKER_GetState(void)
{
    return s_state;
}

void DSPEAKER_SetVolume(uint8_t volume)
{
    if (volume > 100)
        volume = 100;
    s_volume = volume;
}

uint8_t DSPEAKER_GetVolume(void)
{
    return s_volume;
}

void DSPEAKER_RegisterEventCallback(DSPEAKER_EventCallback_t callback)
{
    s_event_callback = callback;
}

/*******************************************************************************
 *                              DMA 中断回调
 ******************************************************************************/

/**
 * @brief  DMA 半传输完成回调（后半缓冲区已发送，通知填充前半部）
 */
void DSPEAKER_DMA_HalfTransfer_Callback(void)
{
    if (s_event_callback)
    {
        s_event_callback(0);
    }
}

/**
 * @brief  DMA 传输完成回调（前半缓冲区已发送，通知填充后半部）
 */
void DSPEAKER_DMA_TransferComplete_Callback(void)
{
    if (s_event_callback)
    {
        s_event_callback(1);
    }
}

/*******************************************************************************
 *                              HAL 回调函数重定义
 ******************************************************************************/

/**
 * @brief  I2S DMA 发送半传输完成回调（重定义HAL弱函数）
 * @note   由 HAL 库在 DMA 中断中自动调用
 */
void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance == DSPEAKER_I2S_INSTANCE)
    {
        DSPEAKER_DMA_HalfTransfer_Callback();
    }
}

/**
 * @brief  I2S DMA 发送完成回调（重定义HAL弱函数）
 * @note   由 HAL 库在 DMA 中断中自动调用
 */
void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance == DSPEAKER_I2S_INSTANCE)
    {
        DSPEAKER_DMA_TransferComplete_Callback();
    }
}

/**
 * @brief  I2S 错误回调（重定义HAL弱函数）
 */
void HAL_I2S_ErrorCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance == DSPEAKER_I2S_INSTANCE)
    {
        DEBUG_ERROR("DSPEAKER: I2S DMA 错误");
        s_state = DSPEAKER_STATE_ERROR;
    }
}
//示例代码：
/* 全局播放器实例*/
// static AudioPlayer_t g_wav_player = {0};
// static MP3Player_t g_mp3_player = {0};
/* ================= 音频播放测试 ================= */
// #if (defined(DSPEAKER_ENABLE) && defined(WAV_PLAY_ENABLE))
//   /*
//    * 在 SDRAM (0xC0800000) 开辟 2MB 缓冲区
//    * 2MB = 2 * 1024 * 1024 字节
//    * int16_t 缓冲区大小 = 字节数 / 2
//    * 注意：请确保 MPU 配置允许访问该 SDRAM 区域
//    */
//   int16_t *p_audio_buffer = (int16_t *)0xC0800000;
//   uint32_t audio_buffer_size = ( 1024 * 1024) / sizeof(int16_t);

//   DEBUG_INFO("正在初始化音频播放器...");

//   /* 初始化播放器并绑定 SDRAM 缓冲区 */
//   AudioPlayer_Init(&g_wav_player, p_audio_buffer, audio_buffer_size);

//   /* 打开并播放文件 (路径: 0:mymusic/1.wav) */
//   if (AudioPlayer_OpenFile(&g_wav_player, "0:mymusic/1.wav") == HAL_OK)
//   {
//     DEBUG_INFO("音频文件打开成功: mymusic/1.wav");
//     /* 0 表示无限循环 */
//     AudioPlayer_PlayWithLoop(&g_wav_player, 0);
//   }
//   else
//   {
//     DEBUG_ERROR("音频文件打开失败: mymusic/1.wav");
//   }
// #endif
// /* ================= 音频播放测试 ================= */
// #if (defined(DSPEAKER_ENABLE) && defined(MP3_PLAY_ENABLE))
// /*
//  * 在 SDRAM (0xC0800000) 开辟缓冲区
//  * 假设总共使用 2MB 空间
//  */

// /* 1. MP3 读取缓冲区 (存放压缩数据) - 分配 128KB，足够减少 SD 卡读取频率 */
// #define MP3_READ_BUF_SIZE (1024 * 1024)
//   uint8_t *p_mp3_read_buffer = (uint8_t *)0xC0800000;

//   /* 2. PCM 音频缓冲区 (存放解码后数据) - 使用剩余空间 */
//   /* 起始地址偏移 128KB */
//   int16_t *p_pcm_buffer = (int16_t *)(0xC0800000 + MP3_READ_BUF_SIZE);

//   /* 计算剩余空间大小 (字节) -> 转换为 int16_t 数量 */
//   uint32_t pcm_buffer_size = (2 * 1024 * 1024 - MP3_READ_BUF_SIZE) / sizeof(int16_t);

//   DEBUG_INFO("正在初始化 MP3 播放器...");

//   /* 初始化播放器并绑定 SDRAM 缓冲区 */
//   MP3Player_Init(&g_mp3_player, p_pcm_buffer, pcm_buffer_size, p_mp3_read_buffer, MP3_READ_BUF_SIZE);

//   /* 打开并播放文件 (路径: 0:mymusic/1.mp3) */
//   if (MP3Player_OpenFile(&g_mp3_player, "0:mymusic/1.mp3") == HAL_OK)
//   {
//     DEBUG_INFO("音频文件打开成功: mymusic/1.mp3");
//     /* 0 表示无限循环 */
//     MP3Player_PlayWithLoop(&g_mp3_player, 0);
//   }
//   else
//   {
//     DEBUG_ERROR("音频文件打开失败: mymusic/1.mp3");
//   }
// #endif
//   /* ================================================ */
// 在主循环里面：
// AudioPlayer_Process(&g_wav_player);
// MP3Player_Process(&g_mp3_player);

#endif /* DSPEAKER_ENABLE */
