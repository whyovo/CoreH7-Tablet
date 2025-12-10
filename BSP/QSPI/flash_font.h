/**
 ******************************************************************************
 * @file    flash_font.h
 * @author  菜菜why(B站:菜菜whyy)
 * @brief   Flash字库驱动头文件 - GB2312完整字库存储方案（内存映射模式）
 ******************************************************************************
 * @attention
 *
 * 使用前提：
 * ---------------------------------------------------------------
 * 1. 字库已通过STM32CubeProgrammer预烧录到QSPI Flash
 * 2. QSPI已配置为内存映射模式（由外部QSPI驱动负责）
 * 3. 字库数据可通过 W25Qxx_Mem_Addr (0x90000000) 直接访问
 *
 * 字库烧录地址分配:
 * ┌──────────────────┬──────────────┬──────────────────┐
 * │ 文件名           │ 烧录地址      │ 说明              │
 * ├──────────────────┼──────────────┼──────────────────┤
 * │ merged_fonts.bin │ 0x91D00000   │ 汉字、英文与数字 全家桶 │
 * └──────────────────┴──────────────┴──────────────────┘
 */

#ifndef FLASH_FONT_H
#define FLASH_FONT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include <stdint.h>
#ifdef FLASH_FONT_ENABLE
/*******************************************************************************
 *                          内存映射基地址定义
 ******************************************************************************/
#define W25Qxx_Mem_Addr 0x90000000 /*!< QSPI内存映射模式基地址 */

/*******************************************************************************
 *                          Flash字体存储地址分配
 *******************************************************************************/
/**
 * @brief QSPI Flash字库存储区域定义(相对于Flash起始地址的偏移)
 */
#define BASE_ADDR 0x1D00000
#define FONT_12x12_ADDR BASE_ADDR + 0x0 /*!< 12x12字体区域起始地址  */
#define FONT_16x16_ADDR BASE_ADDR + 0x2BBE0 /*!< 16x16字体区域起始地址  */
#define FONT_20x20_ADDR BASE_ADDR + 0x66100 /*!< 20x20字体区域起始地址  */
#define FONT_24x24_ADDR BASE_ADDR + 0xD3680 /*!< 24x24字体区域起始地址 */
#define FONT_32x32_ADDR BASE_ADDR + 0x1569E0 /*!< 32x32字体区域起始地址 */

#define GB2312_TABLE_ADDR BASE_ADDR + 0x23FE00 /*!< GB2312对照表地址 */
#define UTF8_TABLE_ADDR BASE_ADDR + 0x2472D0   /*!< utf8对照表地址 */
#define FONT_FLAG_ADDR BASE_ADDR + 0x2572F0    /*!< 字库标志存储地址 */
#define ASCII_FONTS_ADDR BASE_ADDR + 0x267310  /*!< ASCII字库地址 */
/*******************************************************************************
 *                          字库标志结构定义
 ******************************************************************************/
/**
 * @brief  字库写入标志结构体
 * @note   存储在Flash固定地址FONT_FLAG_ADDR,用于标记字库是否已写入
 */
typedef struct {
  uint32_t magic; /*!< 魔数 0x464C4147 ("FLAG") 用于验证数据有效性 */
  uint8_t font_12_ok; /*!< 12x12字库写入完成标志: 0=未写入, 1=已写入 */
  uint8_t font_16_ok; /*!< 16x16字库写入完成标志: 0=未写入, 1=已写入 */
  uint8_t font_20_ok; /*!< 20x20字库写入完成标志: 0=未写入, 1=已写入 */
  uint8_t font_24_ok; /*!< 24x24字库写入完成标志: 0=未写入, 1=已写入 */
  uint8_t font_32_ok; /*!< 32x32字库写入完成标志: 0=未写入, 1=已写入 */
  uint8_t reserved[3]; /*!< 保留字节(4字节对齐) */
} FontWriteFlag_t;

/*******************************************************************************
 *                          结构定义
 ******************************************************************************/

/**
 * @brief  GB2312对照表数据项(每条4字节)
 * @note   格式: uint16_t gbk_code + uint16_t index
 */
typedef struct {
  uint16_t gbk_code; /*!< GBK编码(2字节) */
  uint16_t index;    /*!< 字库索引(2字节) */
} GB2312_TableEntry_t;

/**
 * @brief  UTF8对照表数据项(每条8字节)
 * @note   格式: uint8_t utf8_len + uint8_t utf8[4] + uint16_t index
 */
typedef struct __attribute__((packed)) {
  uint8_t utf8_len; /*!< UTF8字节长度(1-4) */
  uint8_t utf8[4];  /*!< UTF8编码(最多4字节,不足补0) */
  uint16_t index;   /*!< 字库索引(2字节) */
  uint8_t reserved; /*!< 保留字节，必须存在以匹配生成脚本的8字节对齐 */
} UTF8_TableEntry_t;

/**
 * @brief  ASCII字库信息结构体 (对应二进制文件头)
 * @note   每项16字节
 */
typedef struct __attribute__((packed)) {
  uint32_t offset;     /*!< 数据相对于文件头的偏移 */
  uint32_t size;       /*!< 该字体数据总大小 */
  uint16_t width;      /*!< 字符宽度 */
  uint16_t height;     /*!< 字符高度 */
  uint8_t reserved[4]; /*!< 保留字节(补齐16字节) */
} ASCII_FontInfo_t;

/**
 * @brief  ASCII字库文件头结构体
 */
typedef struct __attribute__((packed)) {
  uint32_t magic;            /*!< 魔数 "ASCI" */
  uint32_t num_fonts;        /*!< 包含的字体数量 */
  ASCII_FontInfo_t fonts[5]; /*!< 字体信息数组 */
} ASCII_FontHeader_t;
/*******************************************************************************
 *                          导出函数声明
 ******************************************************************************/

/**
 * @brief  初始化Flash字库驱动
 * @note   前提：QSPI已开启内存映射模式，字库已预烧录
 * @retval 0-成功, <0-失败
 */
int8_t FlashFont_Init(void);

/**
 * @brief  获取指定字体每字符占用的字节数
 * @param  font_size: 字体大小(12/16/20/24/32)
 * @retval >=0: 每字符字节数, <0: 无效字体大小
 */
int16_t FlashFont_BytesPerChar(uint8_t font_size);

/**
 * @brief  从Flash查找汉字对应的字库索引
 * @param  text: 汉字字符串(GBK编码，2字节)
 * @retval 字库索引(0-7463), 未找到返回-1
 * @note   使用线性查找，适合少量查询
 */
int16_t GB2312_FindIndex_Flash(const char *text);

/**
 * @brief  从Flash查找汉字并返回字模数据指针
 * @param  text: 汉字字符串(GBK编码，2字节)
 * @param  font_size: 字体大小(12/16/20/24/32)
 * @retval 字模数据指针，查找失败返回NULL
 */
const uint8_t *GB2312_FindFont_Flash(const char *text, uint8_t font_size);

/**
 * @brief  从Flash查找UTF8字符对应的字库索引
 * @param  utf8_text: UTF8字符字符串(1-4字节)
 * @param  utf8_len: UTF8字符的字节长度(1-4)
 * @retval 字库索引, 未找到返回-1
 * @note   使用线性查找，适合少量查询
 */
int16_t UTF8_FindIndex_Flash(const uint8_t *utf8_text, uint8_t utf8_len);

/**
 * @brief  从Flash查找UTF8字符并返回字模数据指针
 * @param  utf8_text: UTF8字符字符串(1-4字节)
 * @param  font_size: 字体大小(12/16/20/24/32)
 * @retval 字模数据指针，查找失败返回NULL
 * @note   自动识别UTF-8字符长度，无需手动指定
 */
const uint8_t *UTF8_FindFont_Flash(const uint8_t *utf8_text, uint8_t font_size);
/**
 * @brief  从Flash查找ASCII字符并返回字模数据指针
 * @param  c: ASCII字符 (0x20-0x7E)
 * @param  font_size: 字体大小(12/16/20/24/32)
 * @retval 字模数据指针，查找失败返回NULL
 */
const uint8_t *ASCII_FindFont_Flash(char c, uint8_t font_size);
#endif
#ifdef __cplusplus
}
#endif

#endif // FLASH_FONT_H
