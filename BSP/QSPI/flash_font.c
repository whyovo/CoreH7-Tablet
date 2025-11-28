/**
 ******************************************************************************
 * @file    flash_font.c
 * @author  菜菜why（B站：菜菜whyy）
 * @brief   Flash字库驱动实现文件（内存映射模式）
 ******************************************************************************
 */

#include "flash_font.h"
#include <string.h>

#ifdef FLASH_FONT_ENABLE

/*******************************************************************************
 *                              私有宏定义
 ******************************************************************************/
#define FLAG_MAGIC 0x464C4147 /*!< 标志位魔数 "FLAG" */
#define FONT_MAGIC 0x47423332 /*!< 字库魔数 "GB23" (GB2312) */
#define FLASHFONT_OK 0        /*!< 操作成功 */

/*******************************************************************************
 *                              私有函数与变量声明
 ******************************************************************************/
static uint32_t GetFontBaseAddr(uint8_t font_size);

static uint8_t g_font_initialized = 0;      /*!< 初始化标志 */

/*******************************************************************************
 *                              私有函数实现
 ******************************************************************************/

/**
 * @brief  获取字体存储基地址
 * @param  font_size: 字体大小(12/16/20/24/32)
 * @retval Flash地址偏移, 0表示无效尺寸
 */
static uint32_t GetFontBaseAddr(uint8_t font_size) {
  switch (font_size) {
  case 12:
    return FONT_12x12_ADDR;
  case 16:
    return FONT_16x16_ADDR;
  case 20:
    return FONT_20x20_ADDR;
  case 24:
    return FONT_24x24_ADDR;
  case 32:
    return FONT_32x32_ADDR;
  default:
    return 0;
  }
}

/*******************************************************************************
 *                              导出函数实现
 ******************************************************************************/

/**
 * @brief  初始化Flash字库驱动
 * @retval 0-成功, <0-失败
 * @note   前提：QSPI已开启内存映射模式，字库已预烧录
 */
int8_t FlashFont_Init(void) {
  const FontWriteFlag_t *flag;
  // 通过内存映射读取标志位
  flag = (const FontWriteFlag_t *)(W25Qxx_Mem_Addr + FONT_FLAG_ADDR);

  // 验证标志位魔数
  if (flag->magic != FLAG_MAGIC) {
    DEBUG_ERROR("字库标志无效，字库可能未烧录");
    return -1;
  }

  g_font_initialized = 1;

  return FLASHFONT_OK;
}

/**
 * @brief  获取指定字体每字符占用的字节数
 * @param  font_size: 字体大小(12/16/20/24/32)
 * @retval >=0: 每字符字节数, <0: 无效字体大小
 */
int16_t FlashFont_BytesPerChar(uint8_t font_size) {
  switch (font_size) {
  case 12:
    return 24;
  case 16:
    return 32;
  case 20:
    return 60;
  case 24:
    return 72;
  case 32:
    return 128;
  default:
    return -1;
  }
}
/*******************************************************************************
 *                          GB2312对照表Flash访问实现
 ******************************************************************************/

/**
 * @brief  从Flash查找汉字对应的字库索引(线性查找)
 * @param  text: 汉字字符串(GBK编码，2字节)
 * @retval 字库索引(0-7463), 未找到返回-1
 * @note   使用线性查找，时间复杂度O(n)
 * @note   若需要高性能，可改用二分查找
 */
int16_t GB2312_FindIndex_Flash(const char *text) {
  uint16_t search_gbk;
  const uint8_t *pData;
  const GB2312_TableEntry_t *pEntry;

  if (!g_font_initialized) {
    DEBUG_ERROR("GB2312_FindIndex_Flash: 字库未初始化");
    return -1;
  }

  // 将输入字符转为GBK码
  search_gbk = ((uint16_t)text[0] << 8) | (uint8_t)text[1];

  // 计算数据区起始地址(内存映射)
  pData = (const uint8_t *)(W25Qxx_Mem_Addr + GB2312_TABLE_ADDR +
                            12);
  pEntry = (const GB2312_TableEntry_t *)pData;

  // 线性查找
  for (uint32_t i = 0; i < 7464; i++) {
    if (pEntry[i].gbk_code == 0xFFFF) {
      break; // 遇到特殊标记，提前结束
    }

    if (pEntry[i].gbk_code == search_gbk) {
      return pEntry[i].index; // 找到
    }
  }

  return -1; // 未找到
}

/**
 * @brief  从Flash查找汉字并返回字模数据指针
 * @param  text: 汉字字符串(GBK编码，2字节)
 * @param  font_size: 字体大小(12/16/20/24/32)
 * @retval 字模数据指针，查找失败返回NULL
 */
const uint8_t *GB2312_FindFont_Flash(const char *text, uint8_t font_size) {
  int16_t index = GB2312_FindIndex_Flash(text);
  if (index < 0) {
    return NULL;
  }

  uint32_t font_offset = GetFontBaseAddr(font_size);
  uint16_t bytes_per_char = FlashFont_BytesPerChar(font_size);

  if (font_offset == 0) {
    return NULL;
  }

  return (const uint8_t *)(W25Qxx_Mem_Addr + font_offset + 18 +
                           index * bytes_per_char);
}

/*******************************************************************************
 *                          utf8对照表Flash访问实现
 ******************************************************************************/
/**
 * @brief  从Flash查找UTF8字符对应的字库索引(线性查找)
 * @param  utf8_text: UTF8字符字符串(1-4字节)
 * @param  utf8_len: UTF8字符的字节长度(1-4)
 * @retval 字库索引, 未找到返回-1
 */
int16_t UTF8_FindIndex_Flash(const uint8_t *utf8_text, uint8_t utf8_len) {
  const uint8_t *pData;
  const UTF8_TableEntry_t *pEntry;

  if (!g_font_initialized) {
    DEBUG_ERROR("UTF8_FindIndex_Flash: 字库未初始化");
    return -1;
  }

  if (utf8_len < 1 || utf8_len > 4 || utf8_text == NULL) {
    DEBUG_ERROR("UTF8_FindIndex_Flash: 无效的UTF8参数");
    return -1;
  }

  // 计算数据区起始地址(内存映射)
  pData = (const uint8_t *)(W25Qxx_Mem_Addr + UTF8_TABLE_ADDR +
                            12);
  pEntry = (const UTF8_TableEntry_t *)pData;

  // 线性查找
  for (uint32_t i = 0; i < 7464; i++) {
    // 检查UTF8长度是否匹配
    if (pEntry[i].utf8_len == utf8_len) {
      // 比较UTF8编码
      uint8_t match = 1;
      for (uint8_t j = 0; j < utf8_len; j++) {
        if (pEntry[i].utf8[j] != utf8_text[j]) {
          match = 0;
          break;
        }
      }
      if (match) {
        return pEntry[i].index; // 找到
      }
    }
  }

  return -1; // 未找到
}

/**
 * @brief  获取UTF-8字符的字节长度
 * @note   内部私有函数
 */
static uint8_t GetUTF8CharLen(const uint8_t *pText) {
  if (pText == NULL)
    return 0;

  uint8_t first_byte = *pText;

  // 根据UTF-8编码规则判断字符长度
  if ((first_byte & 0x80) == 0x00) {
    return 1; // 0xxxxxxx - ASCII字符
  } else if ((first_byte & 0xE0) == 0xC0) {
    return 2; // 110xxxxx - 2字节UTF-8
  } else if ((first_byte & 0xF0) == 0xE0) {
    return 3; // 1110xxxx - 3字节UTF-8
  } else if ((first_byte & 0xF8) == 0xF0) {
    return 4; // 11110xxx - 4字节UTF-8
  }

  return 1; // 默认返回1
}

/**
 * @brief  从Flash查找UTF8字符并返回字模数据指针
 * @param  utf8_text: UTF8字符字符串(1-4字节)
 * @param  font_size: 字体大小(12/16/20/24/32)
 * @retval 字模数据指针，查找失败返回NULL
 * @note   自动识别UTF-8字符长度
 */
const uint8_t *UTF8_FindFont_Flash(const uint8_t *utf8_text,
                                   uint8_t font_size) {
  uint8_t utf8_len = GetUTF8CharLen(utf8_text); // 自动获取UTF-8长度
  int16_t index = UTF8_FindIndex_Flash(utf8_text, utf8_len);

  if (index < 0) {
    return NULL;
  }

  uint32_t font_offset = GetFontBaseAddr(font_size);
  uint16_t bytes_per_char = FlashFont_BytesPerChar(font_size);

  if (font_offset == 0) {
    return NULL;
  }

  return (const uint8_t *)(W25Qxx_Mem_Addr + font_offset + 18 +
                           index * bytes_per_char);
}
#endif // FLASH_FONT_ENABLE
