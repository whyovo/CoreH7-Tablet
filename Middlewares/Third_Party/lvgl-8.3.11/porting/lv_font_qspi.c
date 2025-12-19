#include "lv_font_qspi.h"
#include <string.h>

/*******************************************************************************
 *                          私有变量与常量
 ******************************************************************************/
typedef struct {
  uint8_t size;
  uint16_t width;
  uint16_t height;
  uint32_t base_addr;
  uint32_t data_offset;
} FontInfo_t;

static FontInfo_t g_font_info[5] = {
    {12, 12, 12, FONT_12x12_ADDR, 18}, // 字库头18字节
    {16, 16, 16, FONT_16x16_ADDR, 18}, {20, 20, 20, FONT_20x20_ADDR, 18},
    {24, 24, 24, FONT_24x24_ADDR, 18}, {32, 32, 32, FONT_32x32_ADDR, 18},
};

static lv_font_t g_custom_fonts[5];
static uint8_t g_font_init_done = 0;

/* 声明 LVGL 内置字体 (确保在 lv_conf.h 中已启用这些字体) */
LV_FONT_DECLARE(lv_font_montserrat_12)
LV_FONT_DECLARE(lv_font_montserrat_16)
LV_FONT_DECLARE(lv_font_montserrat_20)
LV_FONT_DECLARE(lv_font_montserrat_24)
LV_FONT_DECLARE(lv_font_montserrat_32)

#define MAX_GLYPH_SIZE 128 // 32x32 / 8 = 128 bytes
#define CACHE_CAPACITY 230 // 32KB / sizeof(Entry) ≈ 230 个汉字

typedef struct {
  uint32_t unicode;             // 键：Unicode编码
  uint8_t font_size;            // 键：字体大小
  uint8_t is_valid;             // 有效标志
  uint32_t last_access;         // LRU时间戳
  uint8_t data[MAX_GLYPH_SIZE]; // 预处理后的字模数据(已反转位序)
} GlyphCacheEntry_t;

/* 将缓存定义在 AXI SRAM 段中，提高访问速度 */
static __attribute__((section(".sram")))  GlyphCacheEntry_t g_font_cache[CACHE_CAPACITY];

static uint32_t g_access_counter = 0; // 全局时间计数器
/*******************************************************************************
 *                          辅助函数
 ******************************************************************************/
/**
 * @brief 字节位序反转 (LSB <-> MSB)
 * @param b 输入字节
 * @return 反转后的字节
 */
static uint8_t reverse_byte(uint8_t b) {
  b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
  b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
  b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
  return b;
}

/**
 * @brief 将Unicode(UTF-32)转换为UTF-8序列
 * @param v Unicode码点
 * @param buf 输出缓冲区(至少4字节)
 * @return UTF-8字节长度(0-4)
 */
static uint8_t lv_u32_to_utf8(uint32_t v, uint8_t *buf) {
  if (v <= 0x7F) {
    buf[0] = v;
    return 1;
  }
  if (v <= 0x7FF) {
    buf[0] = 0xC0 | (v >> 6);
    buf[1] = 0x80 | (v & 0x3F);
    return 2;
  }
  if (v <= 0xFFFF) {
    buf[0] = 0xE0 | (v >> 12);
    buf[1] = 0x80 | ((v >> 6) & 0x3F);
    buf[2] = 0x80 | (v & 0x3F);
    return 3;
  }
  if (v <= 0x10FFFF) {
    buf[0] = 0xF0 | (v >> 18);
    buf[1] = 0x80 | ((v >> 12) & 0x3F);
    buf[2] = 0x80 | ((v >> 6) & 0x3F);
    buf[3] = 0x80 | (v & 0x3F);
    return 4;
  }
  return 0;
}
/**
 * @brief 从缓存中查找字模
 */
static GlyphCacheEntry_t *cache_lookup(uint32_t unicode, uint8_t font_size) {
  for (int i = 0; i < CACHE_CAPACITY; i++) {
    if (g_font_cache[i].is_valid && g_font_cache[i].unicode == unicode &&
        g_font_cache[i].font_size == font_size) {

      // 命中缓存，更新时间戳 (LRU)
      g_font_cache[i].last_access = ++g_access_counter;
      return &g_font_cache[i];
    }
  }
  return NULL;
}

/**
 * @brief 添加字模到缓存 (LRU替换策略)
 */
static GlyphCacheEntry_t *cache_alloc(uint32_t unicode, uint8_t font_size) {
  int target_idx = -1;
  uint32_t min_time = 0xFFFFFFFF;

  // 1. 查找空闲槽位
  for (int i = 0; i < CACHE_CAPACITY; i++) {
    if (!g_font_cache[i].is_valid) {
      target_idx = i;
      break;
    }
  }

  // 2. 如果没有空闲，查找最久未使用的 (LRU)
  if (target_idx < 0) {
    for (int i = 0; i < CACHE_CAPACITY; i++) {
      if (g_font_cache[i].last_access < min_time) {
        min_time = g_font_cache[i].last_access;
        target_idx = i;
      }
    }
  }

  // 3. 初始化槽位
  if (target_idx >= 0) {
    g_font_cache[target_idx].is_valid = 1;
    g_font_cache[target_idx].unicode = unicode;
    g_font_cache[target_idx].font_size = font_size;
    g_font_cache[target_idx].last_access = ++g_access_counter;
    return &g_font_cache[target_idx];
  }

  return NULL; // 理论上不会到达这里
}
/*******************************************************************************
 *                          自定义字体获取回调函数
 ******************************************************************************/

/**
 * @brief LVGL字体字形描述符获取回调函数（LVGL 8.3.11兼容版本）
 * @note 由LVGL调用以获取指定字符的字模信息
 */
static bool lv_font_qspi_get_glyph_dsc(const lv_font_t *font,
                                       lv_font_glyph_dsc_t *dsc_out,
                                       uint32_t unicode_letter,
                                       uint32_t unicode_letter_next) {
  (void)unicode_letter_next;
  FontInfo_t *pFont = (FontInfo_t *)(font->user_data);
  if (!pFont)
    return false;

  // ASCII字符处理：交给 Fallback 字体
  if (unicode_letter <= 0x7E)
    return false;

  // 检查字库中是否存在 (这里为了速度，我们只做简单的UTF8转换检查，
  // 实际应用中，如果确定字库全，可以直接返回true，进一步减少QSPI访问)
  uint8_t utf8_buf[4];
  uint8_t utf8_len = lv_u32_to_utf8(unicode_letter, utf8_buf);
  if (utf8_len == 0)
    return false;

  // 优化：这里可以尝试先查缓存，如果缓存有，直接返回true，省去一次QSPI查找
  if (cache_lookup(unicode_letter, pFont->size) != NULL) {
    // 缓存命中，直接填参数
    dsc_out->adv_w = pFont->width;
    dsc_out->box_h = pFont->height;
    dsc_out->box_w = pFont->width;
    dsc_out->ofs_x = 0;
    dsc_out->ofs_y = 0;
    dsc_out->bpp = 1;
    return true;
  }

  // 缓存未命中，才去查 Flash 索引
  int16_t index = UTF8_FindIndex_Flash(utf8_buf, utf8_len);
  if (index < 0)
    return false;

  dsc_out->adv_w = pFont->width;
  dsc_out->box_h = pFont->height;
  dsc_out->box_w = pFont->width;
  dsc_out->ofs_x = 0;
  dsc_out->ofs_y = 0;
  dsc_out->bpp = 1;

  return true;
}

/**
 * @brief LVGL字体位图获取回调函数
 * @note 由LVGL调用以获取字符的实际位图数据
 */
static const uint8_t *lv_font_qspi_get_bitmap(const lv_font_t *font,
                                              uint32_t unicode_letter) {
  FontInfo_t *pFont = (FontInfo_t *)(font->user_data);
  if (!pFont)
    return NULL;
  if (unicode_letter <= 0x7E)
    return NULL;

  // 1. 优先查询缓存 (AXI SRAM)
  GlyphCacheEntry_t *entry = cache_lookup(unicode_letter, pFont->size);
  if (entry != NULL) {
    // 命中！直接返回处理好的数据，无需QSPI访问，无需位反转
    return entry->data;
  }

  // 2. 缓存未命中，从 Flash 读取
  uint8_t utf8_buf[4];
  uint8_t utf8_len = lv_u32_to_utf8(unicode_letter, utf8_buf);
  if (utf8_len == 0)
    return NULL;

  const uint8_t *pFontData = UTF8_FindFont_Flash(utf8_buf, pFont->size);
  if (pFontData == NULL)
    return NULL;

  // 3. 申请缓存槽位
  entry = cache_alloc(unicode_letter, pFont->size);
  if (entry == NULL)
    return NULL; // 极端情况

  // 4. 数据处理：拷贝并进行位序反转 (只在加载时做一次)
  uint16_t bytes_per_row = (pFont->width + 7) / 8;
  uint32_t total_bytes = bytes_per_row * pFont->height;

  if (total_bytes > MAX_GLYPH_SIZE)
    total_bytes = MAX_GLYPH_SIZE;

  for (uint32_t i = 0; i < total_bytes; i++) {
    // 存入缓存的是已经反转好的数据
    entry->data[i] = reverse_byte(pFontData[i]);
  }

  // 5. 返回缓存中的数据指针
  return entry->data;
}

/*******************************************************************************
 *                          导出函数实现
 ******************************************************************************/

/**
 * @brief 初始化QSPI字库 (支持UTF-8)
 */
void lv_font_qspi_init(void) {
  if (g_font_init_done)
    return;
  // 清空缓存
  memset(g_font_cache, 0, sizeof(g_font_cache));
  g_access_counter = 0;
  // 初始化Flash字库驱动
  if (FlashFont_Init() != 0) {
    LV_LOG_ERROR("QSPI字库初始化失败");
    return;
  }

  // 为每个字体大小创建LVGL字体对象
  for (int i = 0; i < 5; i++) {

    lv_font_t *font = &g_custom_fonts[i];

    // 初始化字体结构
    memset(font, 0, sizeof(lv_font_t));
    font->get_glyph_dsc = lv_font_qspi_get_glyph_dsc;
    font->get_glyph_bitmap = lv_font_qspi_get_bitmap;
    font->line_height = g_font_info[i].height;

    /*将基线设置为0 (基线位于行底) */
    font->base_line = 0;

    font->subpx = LV_FONT_SUBPX_NONE;
    font->user_data = (void *)&g_font_info[i];

    /* 当遇到 ASCII 字符时，LVGL 会自动去 fallback 字体中查找 */
    switch (g_font_info[i].size) {
#if LV_FONT_MONTSERRAT_12
    case 12:
      font->fallback = &lv_font_montserrat_12;
      break;
#endif
#if LV_FONT_MONTSERRAT_16
    case 16:
      font->fallback = &lv_font_montserrat_16;
      break;
#endif
#if LV_FONT_MONTSERRAT_20
    case 20:
      font->fallback = &lv_font_montserrat_20;
      break;
#endif
#if LV_FONT_MONTSERRAT_24
    case 24:
      font->fallback = &lv_font_montserrat_24;
      break;
#endif
#if LV_FONT_MONTSERRAT_32
    case 32:
      font->fallback = &lv_font_montserrat_32;
      break;
#endif
    default:
      font->fallback = NULL;
      break;
    }

    LV_LOG_INFO("字体%d初始化完成", g_font_info[i].size);
  }
    g_font_init_done = 1;
  }

/**
 * @brief 获取指定大小的LVGL字体对象
 * @param font_size: 字体大小(12/16/20/24/32)
 * @retval lv_font_t* 字体指针，失败返回NULL
 */
lv_font_t *lv_font_qspi_get_by_size(uint8_t font_size) {
  if (!g_font_init_done) {
    LV_LOG_ERROR("字库未初始化，请先调用 lv_font_qspi_init()");
    return NULL;
  }

  for (int i = 0; i < 5; i++) {
    if (g_font_info[i].size == font_size) {
      return &g_custom_fonts[i];
    }
  }

  LV_LOG_ERROR("不支持的字体大小: %d", font_size);
  return NULL;
}
