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

/* === 缓存变量 === */
/* 数据缓存：记录上一次成功解码并驻留在显存中的图片路径 */
static char s_data_cache_src[256] = {0};

/* 信息缓存：记录上一次成功读取头信息的图片路径和宽高 */
/* 这能极大减少 decoder_info 对 SD 卡的访问频率 */
static char s_info_cache_src[256] = {0};
static uint32_t s_info_cache_w = 0;
static uint32_t s_info_cache_h = 0;

/* 简单的 JPEG 头解析 */
static bool get_jpeg_size(const char *filename, uint32_t *width, uint32_t *height)
{
    /* 使用静态 buffer 避免频繁 malloc/free 造成的碎片和开销 */
    /* 注意：这意味着此函数不可重入，但在 LVGL 任务中通常是安全的 */
    static FIL file;
    static uint8_t buf[1024];

    if (f_open(&file, filename, FA_READ) != FR_OK)
    {
        return false;
    }

    UINT br;
    f_read(&file, buf, 1024, &br);
    f_close(&file);

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
    if (lv_img_src_get_type(src) != LV_IMG_SRC_FILE)
        return LV_RES_INV;

    /* === 1. 信息缓存检查 (关键修复) === */
    /* 如果请求的文件与上次相同，直接返回缓存的宽高，不访问 SD 卡 */
    if (strncmp((const char *)src, s_info_cache_src, sizeof(s_info_cache_src)) == 0)
    {
        header->w = s_info_cache_w;
        header->h = s_info_cache_h;
        header->cf = LV_IMG_CF_TRUE_COLOR;
        header->always_zero = 0;
        return LV_RES_OK;
    }

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
        /* 更新信息缓存 */
        strncpy(s_info_cache_src, (const char *)src, sizeof(s_info_cache_src) - 1);
        s_info_cache_w = w;
        s_info_cache_h = h;

        header->always_zero = 0;
        header->cf = LV_IMG_CF_TRUE_COLOR;
        header->w = w;
        header->h = h;
        return LV_RES_OK;
    }
    return LV_RES_INV;
}

/* LVGL 解码器回调：打开图片 */
static lv_res_t decoder_open(lv_img_decoder_t *decoder, lv_img_decoder_dsc_t *dsc)
{
    char dbg[128];

    if (dsc->src_type != LV_IMG_SRC_FILE)
        return LV_RES_INV;

    /* === 1. 数据缓存检查 === */
    /* 如果请求的文件已经解码在显存中，直接返回地址 */
    if (strncmp((const char *)dsc->src, s_data_cache_src, sizeof(s_data_cache_src)) == 0)
    {
        dsc->img_data = (void *)JPEG_ENCODE_OUTPUT_BUFFER;
        return LV_RES_OK;
    }

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
    /* 使用静态 FIL 避免 malloc */
    static FIL file;
    if (f_open(&file, path, FA_READ) != FR_OK)
    {
        return LV_RES_INV;
    }

    uint32_t size = f_size(&file);
    if (size > 0x200000)
    {
        snprintf(dbg, sizeof(dbg), "[JPEG] Error: File too large (%lu)", size);
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
        /* 失败时清空缓存记录 */
        memset(s_data_cache_src, 0, sizeof(s_data_cache_src));
        return LV_RES_INV;
    }

    /* 获取 JPEG 信息 */
    JPEG_ConfTypeDef JPEG_Info;
    extern JPEG_HandleTypeDef hjpeg;
    HAL_JPEG_GetInfo(&hjpeg, &JPEG_Info);

    /* 4. 颜色转换 */
    DMA2D_Convert_To_Buffer((uint32_t *)JPEG_OUTPUT_DATA_BUFFER,
                            (uint32_t *)JPEG_ENCODE_OUTPUT_BUFFER,
                            JPEG_Info.ImageWidth,
                            JPEG_Info.ImageHeight,
                            JPEG_Info.ChromaSubsampling);

    DEBUG_INFO("[JPEG] Convert Done.");

    /* === 5. 更新数据缓存记录 === */
    strncpy(s_data_cache_src, (const char *)dsc->src, sizeof(s_data_cache_src) - 1);

    dsc->img_data = (void *)JPEG_ENCODE_OUTPUT_BUFFER;
    return LV_RES_OK;
}

static void decoder_close(lv_img_decoder_t *decoder, lv_img_decoder_dsc_t *dsc)
{
    /* 不做任何操作，因为我们使用的是静态缓冲区 */
}

void lv_port_jpeg_init(void)
{
    lv_img_decoder_t *dec = lv_img_decoder_create();
    lv_img_decoder_set_info_cb(dec, decoder_info);
    lv_img_decoder_set_open_cb(dec, decoder_open);
    lv_img_decoder_set_close_cb(dec, decoder_close);
}
