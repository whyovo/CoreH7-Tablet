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

/* DMA 双缓冲区（循环模式） */
static int16_t s_dma_buffer[DSPEAKER_BUFFER_SIZE] __attribute__((aligned(4)));

/* 缓冲区管理 */
static uint32_t s_buffer_write_pos = 0; /* 写指针 */
static uint32_t s_buffer_read_pos = 0;  /* 读指针（由DMA更新） */
static uint32_t s_buffer_size = DSPEAKER_BUFFER_SIZE;

/* 状态变量 */
static DSPEAKER_State s_state = DSPEAKER_STATE_RESET;
static DSPEAKER_DataRequiredCallback_t s_callback = NULL;
static uint8_t s_volume = 50; /* 默认音量50% */

/* I2S 句柄引用（在 i2s.c 中定义） */
extern I2S_HandleTypeDef DSPEAKER_I2S_HANDLE;

/*******************************************************************************
 *                              私有函数声明
 ******************************************************************************/
static void fill_buffer(uint8_t isFirstHalf);
static void update_buffer_pos(void);

/*******************************************************************************
 *                              导出函数实现
 ******************************************************************************/

/**
 * @brief  初始化数字扬声器
 * @note   假设 I2S 和 DMA 已在 CubeMX 生成的代码中配置
 */
HAL_StatusTypeDef DSPEAKER_Init(void)
{
    /* 检查状态 */
    if (s_state == DSPEAKER_STATE_PLAYING)
    {
        DEBUG_ERROR("DSPEAKER_Init: 扬声器正在播放中，请先停止");
        return HAL_ERROR;
    }

    /* 清空缓冲区 */
    memset(s_dma_buffer, 0, sizeof(s_dma_buffer));
    s_buffer_write_pos = 0;
    s_buffer_read_pos = 0;
    s_volume = 50;

    /* 设置为就绪状态 */
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
    {
        DEBUG_INFO("DSPEAKER_Start: 扬声器已在播放中");
        return HAL_OK;
    }

    if (s_state == DSPEAKER_STATE_RESET)
    {
        DEBUG_ERROR("DSPEAKER_Start: 请先调用 DSPEAKER_Init() 初始化");
        return HAL_ERROR;
    }

    HAL_Delay(10); /* 等待芯片稳定 */

    /* 清空缓冲区并预填充数据 */
    DSPEAKER_ClearBuffer();
    fill_buffer(1); /* 填充前半缓冲 */
    fill_buffer(0); /* 填充后半缓冲 */

    /* 启动 I2S DMA 发送（循环模式） */
    status = HAL_I2S_Transmit_DMA(&DSPEAKER_I2S_HANDLE, (uint16_t *)s_dma_buffer, DSPEAKER_BUFFER_SIZE);
    if (status != HAL_OK)
    {
        DEBUG_ERROR("DSPEAKER_Start: I2S DMA 启动失败");
        s_state = DSPEAKER_STATE_ERROR;
        return status;
    }

    s_state = DSPEAKER_STATE_PLAYING;
    DEBUG_INFO("开始播放");
    return HAL_OK;
}

/**
 * @brief  停止扬声器播放
 */
HAL_StatusTypeDef DSPEAKER_Stop(void)
{
    HAL_StatusTypeDef status;

    if (s_state != DSPEAKER_STATE_PLAYING)
    {
        DEBUG_INFO("DSPEAKER_Stop: 扬声器未在播放中");
        return HAL_OK;
    }

    /* 停止 I2S DMA 发送 */
    status = HAL_I2S_DMAStop(&DSPEAKER_I2S_HANDLE);
    if (status != HAL_OK)
    {
        DEBUG_ERROR("DSPEAKER_Stop: I2S DMA 停止失败");
        s_state = DSPEAKER_STATE_ERROR;
        return status;
    }

    /* 清空缓冲区 */
    DSPEAKER_ClearBuffer();

    s_state = DSPEAKER_STATE_READY;
    DEBUG_INFO("停止播放");
    return HAL_OK;
}

/**
 * @brief  获取当前状态
 */
DSPEAKER_State DSPEAKER_GetState(void)
{
    return s_state;
}



/**
 * @brief  设置音量（0-100）
 */
void DSPEAKER_SetVolume(uint8_t volume)
{
    if (volume > 100)
        volume = 100;
    s_volume = volume;

    char buf[64];
    snprintf(buf, sizeof(buf), "音量设置为 %d%%", volume);
    DEBUG_INFO(buf);
}

/**
 * @brief  获取当前音量
 */
uint8_t DSPEAKER_GetVolume(void)
{
    return s_volume;
}

/**
 * @brief  向播放缓冲区填充数据
 */
uint32_t DSPEAKER_FeedData(const int16_t *pData, uint32_t size)
{
    uint32_t available;
    uint32_t to_write;
    uint32_t written = 0;

    if (s_state != DSPEAKER_STATE_PLAYING || pData == NULL || size == 0)
        return 0;

    available = DSPEAKER_GetAvailableSpace();

    if (available == 0)
    {
        DEBUG_ERROR("DSPEAKER_FeedData: 缓冲区已满");
        return 0;
    }

    /* 最多写入可用空间大小的数据 */
    to_write = (size > available) ? available : size;

    /* 处理循环缓冲区写入 */
    if (s_buffer_write_pos + to_write <= s_buffer_size)
    {
        /* 不跨越边界 */
        memcpy(&s_dma_buffer[s_buffer_write_pos], pData, to_write * sizeof(int16_t));
        s_buffer_write_pos = (s_buffer_write_pos + to_write) % s_buffer_size;
        written = to_write;
    }
    else
    {
        /* 跨越边界 */
        uint32_t first_part = s_buffer_size - s_buffer_write_pos;
        memcpy(&s_dma_buffer[s_buffer_write_pos], pData, first_part * sizeof(int16_t));
        memcpy(&s_dma_buffer[0], &pData[first_part], (to_write - first_part) * sizeof(int16_t));
        s_buffer_write_pos = (s_buffer_write_pos + to_write) % s_buffer_size;
        written = to_write;
    }

    return written;
}

/**
 * @brief  获取缓冲区可用空间大小
 */
uint32_t DSPEAKER_GetAvailableSpace(void)
{
    update_buffer_pos();

    if (s_buffer_write_pos >= s_buffer_read_pos)
    {
        return s_buffer_read_pos + (s_buffer_size - s_buffer_write_pos) - 1;
    }
    else
    {
        return s_buffer_read_pos - s_buffer_write_pos - 1;
    }
}

/**
 * @brief  注册缓冲区需要数据的回调函数
 */
void DSPEAKER_RegisterCallback(DSPEAKER_DataRequiredCallback_t callback)
{
    if (callback == NULL)
    {
        DEBUG_ERROR("DSPEAKER_RegisterCallback: 回调指针为空");
        return;
    }
    s_callback = callback;
}

/**
 * @brief  取消回调注册
 */
void DSPEAKER_UnregisterCallback(void)
{
    s_callback = NULL;
}

/**
 * @brief  清空播放缓冲区（填充零）
 */
void DSPEAKER_ClearBuffer(void)
{
    memset(s_dma_buffer, 0, sizeof(s_dma_buffer));
    s_buffer_write_pos = 0;
}

/**
 * @brief  等待缓冲区可用
 */
HAL_StatusTypeDef DSPEAKER_WaitForAvailable(uint32_t timeout_ms)
{
    uint32_t start_tick = HAL_GetTick();

    while (DSPEAKER_GetAvailableSpace() == 0)
    {
        if (timeout_ms > 0)
        {
            if (HAL_GetTick() - start_tick > timeout_ms)
            {
                DEBUG_ERROR("DSPEAKER_WaitForAvailable: 等待超时");
                return HAL_TIMEOUT;
            }
        }
        HAL_Delay(1);
    }

    return HAL_OK;
}

/*******************************************************************************
 *                              DMA 中断回调
 ******************************************************************************/

/**
 * @brief  DMA 半传输完成回调（后半缓冲区已发送）
 */
void DSPEAKER_DMA_HalfTransfer_Callback(void)
{
    if (s_state == DSPEAKER_STATE_PLAYING)
    {
        fill_buffer(1); /* 填充前半缓冲 */
    }
}

/**
 * @brief  DMA 传输完成回调（前半缓冲区已发送）
 */
void DSPEAKER_DMA_TransferComplete_Callback(void)
{
    if (s_state == DSPEAKER_STATE_PLAYING)
    {
        fill_buffer(0); /* 填充后半缓冲 */
    }
}

/*******************************************************************************
 *                              私有函数实现
 ******************************************************************************/

/**
 * @brief  填充缓冲区的一半
 * @param  isFirstHalf: 1=填充前半部分, 0=填充后半部分
 */
static void fill_buffer(uint8_t isFirstHalf)
{
    uint32_t offset = isFirstHalf ? 0 : (DSPEAKER_BUFFER_SIZE / 2);
    uint32_t size = DSPEAKER_BUFFER_SIZE / 2;

    /* 如果注册了回调，调用回调函数获取数据 */
    if (s_callback != NULL)
    {
        s_callback(&s_dma_buffer[offset], size, isFirstHalf);
    }
    else
    {
        /* 无数据时填充0（静音） */
        memset(&s_dma_buffer[offset], 0, size * sizeof(int16_t));
    }
    /* === 新增：STM32H7 D-Cache 一致性维护 === */
    /* 将更新后的数据从 Cache 刷入物理内存，确保 DMA 能读到正确的数据 */
    /* SCB_CleanDCache_by_Addr((uint32_t*)地址, 长度字节数) */
    SCB_CleanDCache_by_Addr((uint32_t *)&s_dma_buffer[offset],
                            size * sizeof(int16_t));
}

/**
 * @brief  更新缓冲区读指针位置
 * @note   根据DMA当前传输位置推断读指针
 */
static void update_buffer_pos(void) {
  uint32_t dma_pos =
      DSPEAKER_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(DSPEAKER_I2S_HANDLE.hdmatx);

  /* 确保在有效范围内 */
  if (dma_pos >= DSPEAKER_BUFFER_SIZE)
    dma_pos = 0;

  s_buffer_read_pos = dma_pos;
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

#endif /* DSPEAKER_ENABLE */
