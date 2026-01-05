#include "ov5640_lcd_rgb.h"

#if defined(OV5640_ENABLE) && defined(LCD_RGB_ENABLE)

extern DCMI_HandleTypeDef hdcmi;
extern DMA_HandleTypeDef hdma_dcmi;
extern LTDC_HandleTypeDef hltdc;    // 引用 LTDC 句柄，用于切换显存地址
extern volatile uint8_t OV5640_FPS; // 引用外部变量

/* ==================================================================== */
/*                          SRAM 双缓冲配置                              */
/* ==================================================================== */
// 屏幕参数
#define RGB_WIDTH 800
#define RGB_HEIGHT 480

// 分块参数：将 480 行分为 5 块，每块 96 行
// 96行 * 800像素 * 2字节 = 153,600 字节 (约150KB)
// 双缓冲总大小 = 300KB。STM32H750 的 AXI SRAM (D1域) 有 512KB，完全够用。
#define BLOCK_LINES 96
#define BLOCK_SIZE_BYTES (RGB_WIDTH * BLOCK_LINES * 2)
#define BLOCK_SIZE_WORDS (BLOCK_SIZE_BYTES / 4)

// 定义双缓冲数组，强制放在 RAM_D1 (AXI SRAM) 中以获得最高速度
uint32_t RGB_DoubleBuffer[BLOCK_SIZE_WORDS * 2] __attribute__((section(".RAM_D1"), aligned(32)));

// 定义变焦等级变量 (0-100)
volatile uint8_t OV5640_ZoomLevel = 0;

/* ==================================================================== */
/*                          SDRAM 显存 Ping-Pong 配置                    */
/* ==================================================================== */
// 单帧字节数 (800 * 480 * 2 = 768,000 Bytes, 约 0.75MB)
#define FRAME_SIZE_BYTES (RGB_WIDTH * RGB_HEIGHT * 2)

// 定义两个显存地址 (Ping-Pong Buffer)
// Buffer 1: SDRAM 起始地址
// Buffer 2: SDRAM 起始地址 + 两帧偏移
#define SDRAM_FRAME_BUFFER_1 SDRAM_BANK_ADDR
#define SDRAM_FRAME_BUFFER_2 (SDRAM_BANK_ADDR + FRAME_SIZE_BYTES * 2)

// 当前摄像头正在写入的缓冲区地址 (Back Buffer)
volatile uint32_t Current_Write_Buffer = SDRAM_FRAME_BUFFER_1;
// 当前 LTDC 正在显示的缓冲区地址 (Front Buffer)
volatile uint32_t Current_Display_Buffer = SDRAM_FRAME_BUFFER_2;

// 当前处理的块索引 (0 ~ 4)
volatile uint32_t OV5640_BlockIndex = 0;

/* ==================================================================== */
/*                          DMA 回调函数                                 */
/* ==================================================================== */
/**
 * @brief  DMA 传输完成回调 (双缓冲逻辑 + 软件变焦)
 * @note   当 DMA 填满一个缓冲区时触发，CPU 负责将该缓冲区数据搬运到 SDRAM
 */
void OV5640_RGB_DMA_Callback(DMA_HandleTypeDef *hdma)
{
    // 获取当前 DMA 正在使用的目标缓冲区 (CT 位)
    uint32_t current_target = (((DMA_Stream_TypeDef *)hdma->Instance)->CR & DMA_SxCR_CT);
    uint16_t *src_addr; // 使用 uint16_t 指针方便像素操作

    if (current_target == 0)
    {
        // DMA 正在写 Buffer 0，我们搬运 Buffer 1
        src_addr = (uint16_t *)&RGB_DoubleBuffer[BLOCK_SIZE_WORDS];
    }
    else
    {
        // DMA 正在写 Buffer 1，我们搬运 Buffer 0
        src_addr = (uint16_t *)&RGB_DoubleBuffer[0];
    }

    //  变焦逻辑分支
    if (OV5640_ZoomLevel == 0)
    {
        /* ================= 无变焦  ================= */
        //  memcpy(dst_addr, src_addr, BLOCK_SIZE_BYTES);

        uint32_t dst_addr = Current_Write_Buffer + (OV5640_BlockIndex * BLOCK_SIZE_BYTES);

        DMA2D->CR = 0;
        DMA2D->FGMAR = (uint32_t)src_addr; // 源地址 (SRAM)
        DMA2D->OMAR = dst_addr;            // 目标地址 (SDRAM)
        DMA2D->FGOR = 0;
        DMA2D->OOR = 0;
        DMA2D->NLR = (RGB_WIDTH << 16) | BLOCK_LINES; // 设定宽高 (800x96)
        DMA2D->FGPFCCR = DMA2D_INPUT_RGB565;
        DMA2D->OPFCCR = DMA2D_OUTPUT_RGB565;
        DMA2D->CR = DMA2D_M2M; // 存储器到存储器模式

        DMA2D->CR |= DMA2D_CR_START; // 启动传输

        // 等待传输完成 (DMA2D 速度极快，约 0.5ms，比 memcpy 快得多)
        while (DMA2D->CR & DMA2D_CR_START);
    }
    else
    {
        /* ================= 软件变焦 (裁剪 + 放大) ================= */
        // 1. 计算缩放参数
        // Scale = 1.0 + (ZoomLevel / 100.0) -> 最大 2.0 倍
        float scale = 1.0f + (OV5640_ZoomLevel / 100.0f);
        float inv_scale = 1.0f / scale; // 逆向映射因子

        // 计算裁剪窗口大小 (在原始 800x480 图像中取多大区域)
        int crop_width = (int)(RGB_WIDTH * inv_scale);
        int crop_height = (int)(RGB_HEIGHT * inv_scale);

        // 计算裁剪起始偏移 (居中裁剪)
        int x_offset = (RGB_WIDTH - crop_width) / 2;
        int y_offset = (RGB_HEIGHT - crop_height) / 2;

        // 当前 SRAM 块对应的原始图像行范围
        int block_start_y = OV5640_BlockIndex * BLOCK_LINES;
        int block_end_y = block_start_y + BLOCK_LINES;

        // 2. 遍历屏幕(目标)的每一行，寻找需要的数据是否在当前 SRAM 块中
        // 注意：这里我们遍历的是“输出图像”的行，而不是“输入图像”的行
        // 只有当输出行映射到的输入行位于当前 SRAM 块时，才进行处理

        // 预计算定点数步长，加速内层循环 (Q16格式)
        int fp_inc_x = (int)(inv_scale * 65536.0f);
        int fp_start_x = (x_offset << 16);

        for (int y_dst = 0; y_dst < RGB_HEIGHT; y_dst++)
        {
            // 计算该输出行对应的输入行 (最近邻插值)
            int y_src = y_offset + (int)(y_dst * inv_scale);

            // 判断该输入行是否在当前 DMA 接收到的块中
            if (y_src >= block_start_y && y_src < block_end_y)
            {
                // 命中！开始处理这一行

                // 计算目标地址 (SDRAM)
                uint16_t *pDstLine = (uint16_t *)(Current_Write_Buffer + (y_dst * RGB_WIDTH * 2));

                // 计算源地址 (SRAM) - 需要减去块的起始行偏移
                uint16_t *pSrcLine = src_addr + ((y_src - block_start_y) * RGB_WIDTH);

                // 水平方向缩放 (内层循环，需极致优化)
                int fp_x = fp_start_x;

                for (int x_dst = 0; x_dst < RGB_WIDTH; x_dst++)
                {
                    // 取高16位作为整数坐标
                    int x_src = fp_x >> 16;
                    pDstLine[x_dst] = pSrcLine[x_src];
                    fp_x += fp_inc_x;
                }
            }
        }
    }

    // 更新块索引
    OV5640_BlockIndex++;
    if (OV5640_BlockIndex >= (RGB_HEIGHT / BLOCK_LINES))
    {
        OV5640_BlockIndex = 0;

        static uint32_t last_tick = 0;
        static uint8_t frame_count = 0;

        frame_count++;
        // 每1000ms更新一次FPS
        if ((HAL_GetTick() - last_tick) >= 1000)
        {
            OV5640_FPS = frame_count;
            frame_count = 0;
            last_tick = HAL_GetTick();
        }

        /* ============================================================ */
        /*                  一帧传输完成，执行 Ping-Pong 切换             */
        /* ============================================================ */

        // 1. 交换读写指针
        // 此时 Camera 写完了 Write_Buffer，它变成了最新的完整帧，应该交给 LTDC 显示
        uint32_t temp = Current_Write_Buffer;
        Current_Write_Buffer = Current_Display_Buffer;
        Current_Display_Buffer = temp;

        // 2. 切换 LTDC 显示层地址
        // 使用 NoReload 配合 Reload(VERTICAL_BLANKING) 确保在垂直消隐期切换，彻底消除撕裂
        // 假设使用的是 Layer 0 (通常 RGB 屏驱动只开一层，如果是 Layer 1 请改为 1)
        HAL_LTDC_SetAddress_NoReload(&hltdc, Current_Display_Buffer, 0);

        // 这一步至关重要：告诉 LTDC 在下一次垂直同步（VSYNC）时才更新地址
        HAL_LTDC_Reload(&hltdc, LTDC_RELOAD_VERTICAL_BLANKING);
    }
}

/* ==================================================================== */
/*                          启动函数                                     */
/* ==================================================================== */
/**
 * @brief  启动 RGB 屏摄像头显示 (使用 SRAM 双缓冲 + SDRAM Ping-Pong 全帧缓冲)
 */
void OV5640_RGB_Start(void)
{
    OV5640_BlockIndex = 0;

    // 初始化 Ping-Pong 缓冲区指针
    Current_Write_Buffer = SDRAM_FRAME_BUFFER_1;   // 摄像头先写 Buffer 1
    Current_Display_Buffer = SDRAM_FRAME_BUFFER_2; // 屏幕先显示 Buffer 2 (初始可能为黑或杂色)

    // 确保 LTDC 一开始显示的是 Display Buffer
    HAL_LTDC_SetAddress(&hltdc, Current_Display_Buffer, 0);

    // 1. 初始化 DMA 为循环模式
    hdma_dcmi.Init.Mode = DMA_CIRCULAR;
    hdma_dcmi.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_dcmi.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_dcmi.Init.MemInc = DMA_MINC_ENABLE;
    hdma_dcmi.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_dcmi.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    hdma_dcmi.Init.Priority = DMA_PRIORITY_VERY_HIGH; // 必须最高优先级
    HAL_DMA_Init(&hdma_dcmi);

    // 2. 注册 DMA 回调函数
    // 无论是 Buffer0 满还是 Buffer1 满，都调用同一个处理函数
    hdma_dcmi.XferCpltCallback = OV5640_RGB_DMA_Callback;
    hdma_dcmi.XferM1CpltCallback = OV5640_RGB_DMA_Callback;

    // 3. 启动 DMA 双缓冲模式
    // 参数说明: DMA句柄, 源地址(DCMI_DR), 目标地址0, 目标地址1, 单个缓冲区长度(Word)
    HAL_DMAEx_MultiBufferStart_IT(&hdma_dcmi,
                                  (uint32_t)&hdcmi.Instance->DR,
                                  (uint32_t)&RGB_DoubleBuffer[0],
                                  (uint32_t)&RGB_DoubleBuffer[BLOCK_SIZE_WORDS],
                                  BLOCK_SIZE_WORDS);

    // 4. 手动开启 DCMI 捕获
    // 因为我们绕过了 HAL_DCMI_Start_DMA，所以需要手动使能
    __HAL_DCMI_ENABLE(&hdcmi);
    hdcmi.Instance->CR |= DCMI_CR_CAPTURE;
}
/* ==================================================================== */
/*                          截屏与录像                                     */
/* ==================================================================== */
/**
 * @brief  截图当前摄像头画面变成jpg到SD卡
 * @param  filename: 保存的文件名地址（如："0:capture.jpg"）,写NULL则运用默认配置
 */
#ifdef JPEG_ENABLE
#include "jpeg_app.h"
void OV5640_RGB_Capture(const char *filename)
{
    // 暂停摄像头捕获，防止DMA总线冲突导致SD卡写入失败(FR_DISK_ERR)
    OV5640_DCMI_Suspend();
    HAL_Delay(10); // 等待总线空闲
    JPEG_Save_Screenshot(filename, 0); // 恢复摄像头捕获
    OV5640_DCMI_Resume();
}
#endif

#endif
