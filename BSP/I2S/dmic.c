/**
 ******************************************************************************
 * @file    dmic.c
 * @author  菜菜why(B站:菜菜whyy)
 * @brief   INMP441 数字麦克风 I2S DMA 驱动实现文件
 ******************************************************************************
 * @attention
 *
 * 实现说明：
 * 1. 使用 I2S 主机接收模式 + DMA 循环模式实现双缓冲
 * 2. DMA 配置为循环模式，缓冲区分为前半/后半两部分
 * 3. 半传输中断处理前半缓冲，全传输中断处理后半缓冲
 * 4. 数据格式：I2S接收24位数据
 * 5. 声道选择：通过 INMP441 的 L/R 引脚选择（GND=左，VCC=右）
 * 6. 支持WAV文件录制到SD卡
 *
 * 数据流程：
 * INMP441 -> I2S外设 -> DMA -> 缓冲区 -> 回调函数 -> 用户处理/SD卡保存
 *
 ******************************************************************************
 */

#include "dmic.h"
#include <string.h>
#include <stdio.h>
#include <time.h>

#ifdef DMIC_ENABLE

/*******************************************************************************
 *                          WAV 文件头结构定义
 ******************************************************************************/

/**
 * @brief WAV文件头结构（RIFF格式）
 */
typedef struct __attribute__((packed))
{
    /* RIFF chunk */
    char riff[4];       /*!< "RIFF" */
    uint32_t riff_size; /*!< 文件大小 - 8（需要在录制结束时更新） */
    char wave[4];       /*!< "WAVE" */

    /* fmt chunk */
    char fmt[4];              /*!< "fmt " */
    uint32_t fmt_size;        /*!< fmt子块大小，PCM为16 */
    uint16_t audio_format;    /*!< 音频格式（1=PCM） */
    uint16_t channels;        /*!< 声道数 */
    uint32_t sample_rate;     /*!< 采样率 */
    uint32_t byte_rate;       /*!< 字节率（采样率 * 声道数 * 位深/8） */
    uint16_t block_align;     /*!< 数据块对齐（声道数 * 位深/8） */
    uint16_t bits_per_sample; /*!< 位深 */

    /* data chunk */
    char data[4];       /*!< "data" */
    uint32_t data_size; /*!< 音频数据大小（需要在录制结束时更新） */
} WAV_Header_t;

/*******************************************************************************
 *                              私有变量
 ******************************************************************************/

/* DMA 双缓冲区（循环模式） */
static int32_t s_dma_buffer[DMIC_BUFFER_SIZE] __attribute__((aligned(4)));

/* 状态变量 */
static DMIC_State s_state = DMIC_STATE_RESET;
static DMIC_DataReadyCallback_t s_callback = NULL;

/* WAV录制相关变量 */
static DMIC_RecordState s_record_state = DMIC_RECORD_IDLE;
static FIL s_record_file;
static char s_record_filename[DMIC_MAX_FILENAME_LEN];
static uint32_t s_record_data_size = 0;
static uint8_t s_record_enabled = 0;

/* I2S 句柄引用（在 i2s.c 中定义） */
extern I2S_HandleTypeDef DMIC_I2S_HANDLE;

/*******************************************************************************
 *                              私有函数声明
 ******************************************************************************/
static void process_buffer(uint8_t isHalfBuffer);
static HAL_StatusTypeDef DMIC_WriteWAVHeader(void);
static HAL_StatusTypeDef DMIC_UpdateWAVHeader(void);
static void DMIC_GenerateTimestampFilename(char *filename, size_t len);
static HAL_StatusTypeDef DMIC_FindAvailableFilename(char *filename, size_t max_len);

/*******************************************************************************
 *                              导出函数实现
 ******************************************************************************/

/* 提供弱默认回调实现，用户在任意 C 文件中实现同名函数即可覆盖 */
#if defined(__GNUC__) || defined(__clang__)
__attribute__((weak)) void DMIC_DataReadyCallback(int32_t *pData, uint32_t size, uint8_t isHalfBuffer)
{
    (void)pData;
    (void)size;
    (void)isHalfBuffer;
}
#elif defined(__ICCARM__)
void DMIC_DataReadyCallback(int32_t *pData, uint32_t size, uint8_t isHalfBuffer)
{
    (void)pData;
    (void)size;
    (void)isHalfBuffer;
}
#else
__weak void DMIC_DataReadyCallback(int32_t *pData, uint32_t size, uint8_t isHalfBuffer)
{
    (void)pData;
    (void)size;
    (void)isHalfBuffer;
}
#endif

/**
 * @brief  初始化数字麦克风
 * @note   假设 I2S 和 DMA 已在 CubeMX 生成的代码中配置
 */
HAL_StatusTypeDef DMIC_Init(void)
{
    /* 检查状态 */
    if (s_state == DMIC_STATE_RECORDING)
    {
        DEBUG_ERROR("DMIC_Init: 麦克风正在录音中，请先停止");
        return HAL_ERROR;
    }

    /* 清空缓冲区 */
    memset(s_dma_buffer, 0, sizeof(s_dma_buffer));
    memset(s_record_filename, 0, sizeof(s_record_filename));

    /* 初始化录制相关变量 */
    s_record_state = DMIC_RECORD_IDLE;
    s_record_data_size = 0;
    s_record_enabled = 0;

    /* 设置为就绪状态（未启动录音） */
    s_state = DMIC_STATE_READY;

    /* 注册默认弱回调（用户可不调用注册而直接实现同名弱函数覆盖） */
    DMIC_RegisterCallback(DMIC_DataReadyCallback);

    return HAL_OK;
}

/**
 * @brief  启动麦克风录音
 */
HAL_StatusTypeDef DMIC_Start(void)
{
    HAL_StatusTypeDef status;

    if (s_state == DMIC_STATE_RECORDING)
    {
        DEBUG_INFO("DMIC_Start: 麦克风已在录音中");
        return HAL_OK;
    }

    if (s_state == DMIC_STATE_RESET)
    {
        DEBUG_ERROR("DMIC_Start: 请先调用 DMIC_Init() 初始化");
        return HAL_ERROR;
    }

    /* 启动 I2S DMA 接收（循环模式） */
    status = HAL_I2S_Receive_DMA(&DMIC_I2S_HANDLE, (uint16_t *)s_dma_buffer, DMIC_BUFFER_SIZE);
    if (status != HAL_OK)
    {
        DEBUG_ERROR("DMIC_Start: I2S DMA 启动失败");
        s_state = DMIC_STATE_ERROR;
        return status;
    }

    s_state = DMIC_STATE_RECORDING;
    DEBUG_INFO("开始录音");
    return HAL_OK;
}

/**
 * @brief  停止麦克风录音
 */
HAL_StatusTypeDef DMIC_Stop(void)
{
    HAL_StatusTypeDef status;

    if (s_state != DMIC_STATE_RECORDING)
    {
        DEBUG_INFO("DMIC_Stop: 麦克风未在录音中");
        return HAL_OK;
    }

    /* 停止 I2S DMA 接收 */
    status = HAL_I2S_DMAStop(&DMIC_I2S_HANDLE);
    if (status != HAL_OK)
    {
        DEBUG_ERROR("DMIC_Stop: I2S DMA 停止失败");
        s_state = DMIC_STATE_ERROR;
        return status;
    }

    s_state = DMIC_STATE_READY;
    DEBUG_INFO("停止录音");
    return HAL_OK;
}

/**
 * @brief  获取当前状态
 */
DMIC_State DMIC_GetState(void)
{
    return s_state;
}

/**
 * @brief  注册数据就绪回调函数
 */
void DMIC_RegisterCallback(DMIC_DataReadyCallback_t callback)
{
    if (callback == NULL)
    {
        DEBUG_ERROR("DMIC_RegisterCallback: 回调指针为空");
        return;
    }
    s_callback = callback;
}

/**
 * @brief  取消回调注册
 */
void DMIC_UnregisterCallback(void)
{
    s_callback = NULL;
}

/*******************************************************************************
 *                      WAV 文件录制函数实现
 ******************************************************************************/

/**
 * @brief  启动WAV文件录制
 */
HAL_StatusTypeDef DMIC_StartRecord(const char *filename)
{
    FRESULT fresult;
    char full_path[DMIC_MAX_FILENAME_LEN];
    char buf[256];

    /* 检查状态 */
    if (s_record_state == DMIC_RECORD_RUNNING)
    {
        DEBUG_ERROR("DMIC_StartRecord: 已有录制在进行中");
        return HAL_ERROR;
    }

    /* 需要FATFS和SD卡支持 */
#ifdef FATFS_ENABLE
    extern char SDPath[4];
#else
    DEBUG_ERROR("DMIC_StartRecord: 需要启用 FATFS_ENABLE");
    return HAL_ERROR;
#endif

    /* 生成文件名 */
    if (filename == NULL)
    {
        /* 使用时间戳命名 */
        DMIC_GenerateTimestampFilename(full_path, sizeof(full_path));
    }
    else
    {
        /* 检查文件名是否已有重复，自动添加序号 */
        strncpy(full_path, filename, sizeof(full_path) - 1);
        full_path[sizeof(full_path) - 1] = '\0';

        if (DMIC_FindAvailableFilename(full_path, sizeof(full_path)) != HAL_OK)
        {
            DEBUG_ERROR("DMIC_StartRecord: 无法生成可用的文件名");
            return HAL_ERROR;
        }
    }

    /* 保存文件名 */
    strncpy(s_record_filename, full_path, sizeof(s_record_filename) - 1);
    s_record_filename[sizeof(s_record_filename) - 1] = '\0';

    /* 打开或创建文件 */
    fresult = f_open(&s_record_file, full_path, FA_CREATE_ALWAYS | FA_WRITE);
    if (fresult != FR_OK)
    {
        snprintf(buf, sizeof(buf), "DMIC_StartRecord: 打开文件失败，错误码: %d", fresult);
        DEBUG_ERROR(buf);
        s_record_state = DMIC_RECORD_ERROR;
        return HAL_ERROR;
    }

    /* 写入WAV文件头（暂时性的，待录制结束时更新） */
    if (DMIC_WriteWAVHeader() != HAL_OK)
    {
        DEBUG_ERROR("DMIC_StartRecord: 写入WAV头失败");
        f_close(&s_record_file);
        s_record_state = DMIC_RECORD_ERROR;
        return HAL_ERROR;
    }

    /* 重置数据计数 */
    s_record_data_size = 0;
    s_record_enabled = 1;
    s_record_state = DMIC_RECORD_RUNNING;

    snprintf(buf, sizeof(buf), "开始录制WAV文件: %s", full_path);
    DEBUG_INFO(buf);
    DMIC_Start();
    return HAL_OK;
}

/**
 * @brief  停止WAV文件录制
 */
HAL_StatusTypeDef DMIC_StopRecord(void)
{
    DMIC_Stop();
    FRESULT fresult;
    char buf[256];

    if (s_record_state != DMIC_RECORD_RUNNING)
    {
        DEBUG_INFO("DMIC_StopRecord: 未在录制中");
        return HAL_OK;
    }

    /* 禁止继续写入 */
    s_record_enabled = 0;

    /* 更新WAV文件头 */
    if (DMIC_UpdateWAVHeader() != HAL_OK)
    {
        DEBUG_ERROR("DMIC_StopRecord: 更新WAV头失败");
        f_close(&s_record_file);
        s_record_state = DMIC_RECORD_ERROR;
        return HAL_ERROR;
    }

    /* 关闭文件 */
    fresult = f_close(&s_record_file);
    if (fresult != FR_OK)
    {
        snprintf(buf, sizeof(buf), "DMIC_StopRecord: 关闭文件失败，错误码: %d", fresult);
        DEBUG_ERROR(buf);
        s_record_state = DMIC_RECORD_ERROR;
        return HAL_ERROR;
    }

    snprintf(buf, sizeof(buf), "停止录制，数据大小: %u 字节", s_record_data_size);
    DEBUG_INFO(buf);

    s_record_state = DMIC_RECORD_IDLE;
    return HAL_OK;
}

/**
 * @brief  获取当前录制状态
 */
DMIC_RecordState DMIC_GetRecordState(void)
{
    return s_record_state;
}

/**
 * @brief  获取已录制的音频数据大小（字节）
 */
uint32_t DMIC_GetRecordedSize(void)
{
    return s_record_data_size;
}

/**
 * @brief  获取最后一次录制的文件名
 */
const char *DMIC_GetLastRecordFile(void)
{
    return s_record_filename;
}

/*******************************************************************************
 *                              DMA 中断回调
 ******************************************************************************/

/**
 * @brief  DMA 半传输完成回调（前半缓冲区就绪）
 */
void DMIC_DMA_HalfTransfer_Callback(void)
{
    process_buffer(1); // 前半缓冲就绪
}

/**
 * @brief  DMA 传输完成回调（后半缓冲区就绪）
 */
void DMIC_DMA_TransferComplete_Callback(void)
{
    process_buffer(0); // 后半缓冲就绪
}

/*******************************************************************************
 *                              私有函数实现
 ******************************************************************************/

/**
 * @brief  处理就绪的缓冲区数据
 * @param  isHalfBuffer: 1=前半缓冲, 0=后半缓冲
 */
static void process_buffer(uint8_t isHalfBuffer)
{
    if (s_state != DMIC_STATE_RECORDING)
        return;

    uint32_t offset = isHalfBuffer ? 0 : (DMIC_BUFFER_SIZE / 2);
    uint32_t size = DMIC_BUFFER_SIZE / 2;

    /* ---- For STM32H7: invalidate D-Cache for the DMA buffer region before CPU read ---- */
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
    {
        void *buf_addr = (void *)&s_dma_buffer[offset];
        int32_t bytes = (int32_t)(size * sizeof(s_dma_buffer[0]));

        uintptr_t addr_u = (uintptr_t)buf_addr;
        uintptr_t aligned_start = addr_u & ~((uintptr_t)0x1F);
        int32_t aligned_len = (int32_t)(((bytes + (int32_t)(addr_u - aligned_start) + 31) & ~31));

        SCB_InvalidateDCache_by_Addr((void *)aligned_start, aligned_len);
    }
#endif

    /* 如果启用WAV录制，将数据写入SD卡 */
    if (s_record_enabled && s_record_state == DMIC_RECORD_RUNNING)
    {
        DMIC_RecordAudioData(&s_dma_buffer[offset], size);
    }

    /* 调用用户回调 */
    if (s_callback != NULL)
    {
        s_callback(&s_dma_buffer[offset], size, isHalfBuffer);
    }
    else
    {
        DMIC_DataReadyCallback(&s_dma_buffer[offset], size, isHalfBuffer);
    }
}

/**
 * @brief  将音频数据写入WAV文件
 * @param  pData: 音频数据指针
 * @param  size: 数据大小（采样点数）
 * @note   内部函数，在中断中调用，需要高效处理
 */
static HAL_StatusTypeDef DMIC_RecordAudioData(int32_t *pData, uint32_t size)
{
    FRESULT fresult;
    UINT bytes_written;
    uint16_t i;
    int16_t pcm_sample;

    /* 将32位I2S数据转换为16位PCM并写入文件 */
    for (i = 0; i < size; i++)
    {
        /* 将32位数据转换为16位（取高16位） */
        pcm_sample = (int16_t)(pData[i] >> 8);

        /* 写入文件 */
        fresult = f_write(&s_record_file, &pcm_sample, sizeof(int16_t), &bytes_written);
        if (fresult != FR_OK)
        {
            DEBUG_ERROR("DMIC_RecordAudioData: 写入文件失败");
            return HAL_ERROR;
        }

        if (bytes_written != sizeof(int16_t))
        {
            DEBUG_ERROR("DMIC_RecordAudioData: 写入字节数不完整");
            return HAL_ERROR;
        }

        s_record_data_size += bytes_written;
    }

    return HAL_OK;
}

/**
 * @brief  写入WAV文件头
 */
static HAL_StatusTypeDef DMIC_WriteWAVHeader(void)
{
    FRESULT fresult;
    UINT bytes_written;
    WAV_Header_t wav_header;

    /* 初始化WAV文件头 */
    memcpy(wav_header.riff, "RIFF", 4);
    memcpy(wav_header.wave, "WAVE", 4);
    memcpy(wav_header.fmt, "fmt ", 4);
    memcpy(wav_header.data, "data", 4);

    wav_header.fmt_size = 16;    /* PCM格式 */
    wav_header.audio_format = 1; /* 1 = PCM */
    wav_header.channels = DMIC_CHANNELS;
    wav_header.sample_rate = DMIC_SAMPLE_RATE;
    wav_header.bits_per_sample = DMIC_BITS_PER_SAMPLE;
    wav_header.block_align = DMIC_CHANNELS * (DMIC_BITS_PER_SAMPLE / 8);
    wav_header.byte_rate = DMIC_SAMPLE_RATE * wav_header.block_align;

    /* 暂时设为0，录制结束时更新 */
    wav_header.data_size = 0;
    wav_header.riff_size = 36 + wav_header.data_size;

    /* 写入文件头 */
    fresult = f_write(&s_record_file, &wav_header, sizeof(wav_header), &bytes_written);
    if (fresult != FR_OK || bytes_written != sizeof(wav_header))
    {
        DEBUG_ERROR("DMIC_WriteWAVHeader: 写入WAV头失败");
        return HAL_ERROR;
    }

    return HAL_OK;
}

/**
 * @brief  更新WAV文件头（补齐文件大小等信息）
 */
static HAL_StatusTypeDef DMIC_UpdateWAVHeader(void)
{
    FRESULT fresult;
    UINT bytes_written;
    WAV_Header_t wav_header;

    /* 初始化WAV文件头 */
    memcpy(wav_header.riff, "RIFF", 4);
    memcpy(wav_header.wave, "WAVE", 4);
    memcpy(wav_header.fmt, "fmt ", 4);
    memcpy(wav_header.data, "data", 4);

    wav_header.fmt_size = 16;
    wav_header.audio_format = 1;
    wav_header.channels = DMIC_CHANNELS;
    wav_header.sample_rate = DMIC_SAMPLE_RATE;
    wav_header.bits_per_sample = DMIC_BITS_PER_SAMPLE;
    wav_header.block_align = DMIC_CHANNELS * (DMIC_BITS_PER_SAMPLE / 8);
    wav_header.byte_rate = DMIC_SAMPLE_RATE * wav_header.block_align;

    /* 设置正确的大小 */
    wav_header.data_size = s_record_data_size;
    wav_header.riff_size = 36 + s_record_data_size;

    /* 移动到文件开头 */
    fresult = f_lseek(&s_record_file, 0);
    if (fresult != FR_OK)
    {
        DEBUG_ERROR("DMIC_UpdateWAVHeader: 文件指针移动失败");
        return HAL_ERROR;
    }

    /* 重新写入文件头 */
    fresult = f_write(&s_record_file, &wav_header, sizeof(wav_header), &bytes_written);
    if (fresult != FR_OK || bytes_written != sizeof(wav_header))
    {
        DEBUG_ERROR("DMIC_UpdateWAVHeader: 更新WAV头失败");
        return HAL_ERROR;
    }

    return HAL_OK;
}

/**
 * @brief  生成基于时间戳的文件名
 * @param  filename: 文件名缓冲区
 * @param  len: 缓冲区大小
 */
static void DMIC_GenerateTimestampFilename(char *filename, size_t len)
{
    uint32_t tick = HAL_GetTick();
    snprintf(filename, len, "0:record_%08u.wav", tick);
}

/**
 * @brief  查找可用的文件名（处理同名冲突）
 * @param  filename: 输入输出文件名缓冲区
 * @param  max_len: 缓冲区最大大小
 * @retval HAL_OK=成功找到可用名, HAL_ERROR=失败
 */
static HAL_StatusTypeDef DMIC_FindAvailableFilename(char *filename, size_t max_len)
{
    char test_filename[DMIC_MAX_FILENAME_LEN];
    char *p_dot;
    char name_base[DMIC_MAX_FILENAME_LEN];
    char name_ext[20];
    uint32_t index = 1;

    /* 分离文件名和扩展名 */
    strncpy(test_filename, filename, sizeof(test_filename) - 1);
    test_filename[sizeof(test_filename) - 1] = '\0';

    p_dot = strrchr(test_filename, '.');
    if (p_dot == NULL)
    {
        /* 无扩展名 */
        strncpy(name_base, test_filename, sizeof(name_base) - 1);
        name_base[sizeof(name_base) - 1] = '\0';
        strcpy(name_ext, "");
    }
    else
    {
        *p_dot = '\0';
        strncpy(name_base, test_filename, sizeof(name_base) - 1);
        name_base[sizeof(name_base) - 1] = '\0';
        strncpy(name_ext, p_dot, sizeof(name_ext) - 1);
        name_ext[sizeof(name_ext) - 1] = '\0';
    }

    /* 首先检查原始文件名是否存在 */
    if (FatFs_FileExists(filename) == 0)
    {
        return HAL_OK; /* 文件不存在，使用原始名 */
    }

    /* 尝试添加数字后缀 */
    while (index <= 9999)
    {
        snprintf(test_filename, sizeof(test_filename), "%s_%u%s", name_base, index, name_ext);

        if (FatFs_FileExists(test_filename) == 0)
        {
            /* 找到可用的名字 */
            strncpy(filename, test_filename, max_len - 1);
            filename[max_len - 1] = '\0';
            return HAL_OK;
        }

        index++;
    }

    return HAL_ERROR; /* 无法找到可用的文件名 */
}

/*******************************************************************************
 *                              HAL 回调函数重定义
 ******************************************************************************/

/**
 * @brief  I2S DMA 接收半传输完成回调（重定义HAL弱函数）
 * @note   由 HAL 库在 DMA 中断中自动调用
 */
void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance == DMIC_I2S_INSTANCE)
    {
        DMIC_DMA_HalfTransfer_Callback();
    }
}

/**
 * @brief  I2S DMA 接收完成回调（重定义HAL弱函数）
 * @note   由 HAL 库在 DMA 中断中自动调用
 */
void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance == DMIC_I2S_INSTANCE)
    {
        DMIC_DMA_TransferComplete_Callback();
    }
}

/**
 * @brief  I2S 错误回调（重定义HAL弱函数）
 */
void HAL_I2S_ErrorCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance == DMIC_I2S_INSTANCE)
    {
        DEBUG_ERROR("DMIC: I2S DMA 错误");
        s_state = DMIC_STATE_ERROR;
    }
}

#endif /* DMIC_ENABLE */
