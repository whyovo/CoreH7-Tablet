#include "lv_port_jpeg.h"
#include "lv_port_fs.h"
#include "jpeg_code.h"
#include "fatfs.h"
#include "lcd_rgb.h"
#include "main.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

/* 引用外部 DMA2D 句柄 */
extern DMA2D_HandleTypeDef hdma2d;

/* 简单的 JPEG 头解析 */
static bool get_jpeg_size(const char *filename, uint32_t *width, uint32_t *height)
{
    /* 使用 static 并强制 32 字节对齐，确保 FIL 内部缓冲区满足 DMA 和 Cache 要求 */
    /* 放在静态区（SRAM/SDRAM）比放在堆栈（可能在 DTCM）更安全 */
    /* 强制放入 .ram_d2_data 段，防止被分配到 DTCM (IDMA无法访问) */
    static FIL file __attribute__((aligned(32), section(".ram_d2_data")));
    /* 保持 static 以确保 32 字节对齐 */
    static uint8_t buf[1024] __attribute__((aligned(32), section(".ram_d2_data")));

    FRESULT fr;
    /* 增加简单的重试机制，应对 SD 卡偶尔忙碌的情况 */
    for (int i = 0; i < 5; i++)
    {
        fr = f_open(&file, filename, FA_READ);
        if (fr == FR_OK)
            break;
        HAL_Delay(10);
    }

    if (fr != FR_OK)
    {
        char dbg[64];
        snprintf(dbg, sizeof(dbg), "[JPEG] Info: Open failed %s (Res:%d)", filename, fr);
        DEBUG_INFO(dbg);
        return false;
    }

    /* 增加一个微小的延时，让 SD 卡状态机稳定 */
    HAL_Delay(1);

    UINT br;
    /* 修改：增加 f_read 的重试机制 */
    for (int i = 0; i < 6; i++)
    {
        fr = f_read(&file, buf, 1024, &br);
        if (fr == FR_OK)
            break;

        DEBUG_INFO("[JPEG] Info: Read retry %d (Res:%d)", i + 1, fr);
        /* 如果读取失败，尝试复位文件指针或稍作等待 */
        f_lseek(&file, 0);
        HAL_Delay(5);
    }

    if (fr != FR_OK)
    {
        DEBUG_INFO("[JPEG] Info: Read failed");
        f_close(&file);
        return false;
    }
    f_close(&file);

    /* 使用 CleanInvalidate */
    /* 如果 f_read 用 CPU 拷贝，Clean 会把数据写回 RAM；*/
    /* 如果 f_read 用 DMA，Invalidate 会让 CPU 重新从 RAM 读取；*/
    /* 这样无论 FatFs 内部如何实现，都能保证数据一致性。*/
    SCB_CleanInvalidateDCache_by_Addr((uint32_t *)buf, 1024);

    bool ret = false;
    uint32_t i = 0;
    if (buf[i] == 0xFF && buf[i + 1] == 0xD8) /* Is JPEG */
    {
        i += 2;
        while (i < br - 10)
        {
            if (buf[i] != 0xFF)
                break;
            uint8_t marker = buf[i + 1];
            uint16_t len = (buf[i + 2] << 8) | buf[i + 3];

            if (marker == 0xC0) /* SOF0 */
            {
                *height = (buf[i + 5] << 8) | buf[i + 6];
                *width = (buf[i + 7] << 8) | buf[i + 8];
                ret = true;
                break;
            }
            i += 2 + len;
        }
    }

    if (!ret)
    {
        DEBUG_INFO("[JPEG] Info: Parse failed (Not a JPEG or Header error)");
    }
    return ret;
}

/* 自定义 DMA2D 转换 */
static void DMA2D_Convert_To_Buffer(uint32_t *pSrc, uint32_t *pDst, uint32_t width, uint32_t height, uint32_t chroma_subsampling)
{
    uint32_t cssMode = DMA2D_CSS_420;
    uint32_t inputLineOffset = 0;

    if (chroma_subsampling == JPEG_420_SUBSAMPLING)
    {
        cssMode = DMA2D_CSS_420;
        inputLineOffset = width % 16;
        if (inputLineOffset != 0)
            inputLineOffset = 16 - inputLineOffset;
    }
    else if (chroma_subsampling == JPEG_444_SUBSAMPLING)
    {
        cssMode = DMA2D_NO_CSS;
        inputLineOffset = width % 8;
        if (inputLineOffset != 0)
            inputLineOffset = 8 - inputLineOffset;
    }
    else if (chroma_subsampling == JPEG_422_SUBSAMPLING)
    {
        cssMode = DMA2D_CSS_422;
        inputLineOffset = width % 16;
        if (inputLineOffset != 0)
            inputLineOffset = 16 - inputLineOffset;
    }

    hdma2d.Init.Mode = DMA2D_M2M_PFC;
    hdma2d.Init.ColorMode = DMA2D_OUTPUT_RGB565;
    hdma2d.Init.OutputOffset = 0;
    hdma2d.Init.AlphaInverted = DMA2D_REGULAR_ALPHA;
    hdma2d.Init.RedBlueSwap = DMA2D_RB_REGULAR;

    hdma2d.LayerCfg[1].AlphaMode = DMA2D_REPLACE_ALPHA;
    hdma2d.LayerCfg[1].InputAlpha = 0xFF;
    hdma2d.LayerCfg[1].InputColorMode = DMA2D_INPUT_YCBCR;
    hdma2d.LayerCfg[1].ChromaSubSampling = cssMode;
    hdma2d.LayerCfg[1].InputOffset = inputLineOffset;
    hdma2d.LayerCfg[1].AlphaInverted = DMA2D_REGULAR_ALPHA;
    hdma2d.LayerCfg[1].RedBlueSwap = DMA2D_RB_REGULAR;

    hdma2d.Instance = DMA2D;
    HAL_DMA2D_Init(&hdma2d);
    HAL_DMA2D_ConfigLayer(&hdma2d, 1);

    HAL_DMA2D_Start(&hdma2d, (uint32_t)pSrc, (uint32_t)pDst, width, height);

    if (HAL_DMA2D_PollForTransfer(&hdma2d, 1000) != HAL_OK)
    {
        DEBUG_INFO("Error: DMA2D Transfer Timeout!");
    }

    /* Invalidate D-Cache to ensure CPU reads correct data from SDRAM */
    SCB_InvalidateDCache_by_Addr((uint32_t *)pDst, width * height * 2);
}

/* LVGL 解码器回调：获取图片信息 */
static lv_res_t decoder_info(lv_img_decoder_t *decoder, const void *src, lv_img_header_t *header)
{
    (void)decoder; /* 消除未使用参数警告 */
    if (lv_img_src_get_type(src) != LV_IMG_SRC_FILE)
        return LV_RES_INV;

    /* === 信息缓存检查已移除 === */

    const char *fn = src;
    /* 简单检查扩展名 */
    size_t len = strlen(fn);
    if (len < 4 || (strcmp(&fn[len - 3], "jpg") != 0 && strcmp(&fn[len - 3], "JPG") != 0))
    {
        return LV_RES_INV;
    }

    char path[256];
    if (fn[0] == 'S' && fn[1] == ':')
    {
        snprintf(path, sizeof(path), "0:/%s", &fn[2]);
    }
    else
    {
        return LV_RES_INV;
    }

    uint32_t w, h;
    if (get_jpeg_size(path, &w, &h))
    {
        /* 更新信息缓存已移除 */

        header->always_zero = 0;
        header->cf = LV_IMG_CF_TRUE_COLOR;
        header->w = w;
        header->h = h;
        return LV_RES_OK;
    }

    /* 添加错误日志以便调试 */
    char dbg[128];
    snprintf(dbg, sizeof(dbg), "[JPEG] Info failed: %s", path);
    DEBUG_INFO(dbg);

    return LV_RES_INV;
}

/* LVGL 解码器回调：打开图片 */
static lv_res_t decoder_open(lv_img_decoder_t *decoder, lv_img_decoder_dsc_t *dsc)
{
    (void)decoder; /* 消除未使用参数警告 */
    char dbg[128];

    if (dsc->src_type != LV_IMG_SRC_FILE)
        return LV_RES_INV;

    /* === 数据缓存检查已移除 === */

    snprintf(dbg, sizeof(dbg), "[JPEG] Open: %s", (const char *)dsc->src);
    DEBUG_INFO(dbg);

    const char *fn = dsc->src;
    char path[256];
    if (fn[0] == 'S' && fn[1] == ':')
    {
        snprintf(path, sizeof(path), "0:/%s", &fn[2]);
    }
    else
    {
        return LV_RES_INV;
    }

    /* 2. 读取文件 */
    /* 使用 static 并强制 32 字节对齐 */
    /* 强制放入 .ram_d2_data 段 */
    static FIL file __attribute__((aligned(32), section(".ram_d2_data")));

    /* 增加重试机制 */
    FRESULT fr;
    for (int i = 0; i < 3; i++)
    {
        fr = f_open(&file, path, FA_READ);
        if (fr == FR_OK)
            break;
        HAL_Delay(5);
    }

    if (fr != FR_OK)
    {
        snprintf(dbg, sizeof(dbg), "[JPEG] Open failed: %d", fr);
        DEBUG_INFO(dbg);
        return LV_RES_INV;
    }

    uint32_t size = f_size(&file);
    if (size > 0x200000)
    {
        /* 强制转换为 unsigned long 以匹配 %lu 格式符，消除警告 */
        snprintf(dbg, sizeof(dbg), "[JPEG] Error: File too large (%lu)", (unsigned long)size);
        DEBUG_INFO(dbg);
        f_close(&file);
        return LV_RES_INV;
    }

    UINT br;
    f_read(&file, (void *)File_BUFFER, size, &br);
    f_close(&file);

    /* === 关键：清除 D-Cache === */
    SCB_CleanDCache_by_Addr((uint32_t *)File_BUFFER, size + 32);

    /* 3. 硬件解码 */
    JPEG_Decode_DMA((uint32_t)File_BUFFER, size, JPEG_OUTPUT_DATA_BUFFER);

    /* 等待解码完成 */
    if (JPEG_Decode_WaitingforEnd() != JPEG_OpComplete)
    {
        DEBUG_INFO("[JPEG] Error: HW Decode Failed!");

        return LV_RES_INV;
    }

    /* 获取 JPEG 信息 */
    JPEG_ConfTypeDef JPEG_Info;
    extern JPEG_HandleTypeDef hjpeg;
    HAL_JPEG_GetInfo(&hjpeg, &JPEG_Info);

    /* 4. 颜色转换 */

    /* 计算所需内存大小 (RGB565 = 2 bytes per pixel) */
    uint32_t img_data_size = JPEG_Info.ImageWidth * JPEG_Info.ImageHeight * 2;

    /* 使用 lv_mem_alloc 分配内存 (确保 LVGL 堆足够大，通常在 SDRAM 中) */
    uint8_t *img_data = lv_mem_alloc(img_data_size);
    if (img_data == NULL)
    {
        DEBUG_INFO("[JPEG] Error: Memory allocation failed");
        return LV_RES_INV;
    }

    /* 将解码后的数据通过 DMA2D 转换到新分配的内存中 */
    DMA2D_Convert_To_Buffer((uint32_t *)JPEG_OUTPUT_DATA_BUFFER,
                            (uint32_t *)img_data,
                            JPEG_Info.ImageWidth,
                            JPEG_Info.ImageHeight,
                            JPEG_Info.ChromaSubsampling);

    DEBUG_INFO("[JPEG] Convert Done.");

    /* === 更新数据缓存记录已移除 === */

    /* 将 dsc->img_data 指向新分配的独立内存 */
    dsc->img_data = (void *)img_data;
    return LV_RES_OK;
}

static void decoder_close(lv_img_decoder_t *decoder, lv_img_decoder_dsc_t *dsc)
{
    /* 释放动态分配的内存 */
    if (dsc->img_data)
    {
        lv_mem_free((void *)dsc->img_data);
        dsc->img_data = NULL;
    }
}

void lv_port_jpeg_init(void)
{
    lv_img_decoder_t *dec = lv_img_decoder_create();
    lv_img_decoder_set_info_cb(dec, decoder_info);
    lv_img_decoder_set_open_cb(dec, decoder_open);
    lv_img_decoder_set_close_cb(dec, decoder_close);
}
