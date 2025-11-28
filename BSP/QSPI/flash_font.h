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
 * │ 文件名           │ 烧录地址      │ 说明             │
 * ├──────────────────┼──────────────┼──────────────────┤
 * │ font_12x12_*.bin │ 0x91D00000   │ 12x12字库        │
 * │ font_16x16_*.bin │ 0x91D2BBE0   │ 16x16字库        │
 * │ font_20x20_*.bin │ 0x91D66100   │ 20x20字库        │
 * │ font_24x24_*.bin │ 0x91DD3680   │ 24x24字库        │
 * │ font_32x32_*.bin │ 0x91E569E0   │ 32x32字库        │
 * │ gb2312_table.bin │ 0x91F3FE00   │ GB2312对照表     │
 * │ utf8_table.bin   │ 0x91F472D0   │ UTF8  对照表     │
 * │ flag.bin         │ 0x91F572F0   │ 标志位           │
 * └──────────────────┴──────────────┴──────────────────┘
 */

#ifndef FLASH_FONT_H
#define FLASH_FONT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "init.h"
#include <stdint.h>

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
#define FONT_12x12_ADDR 0x1D00000 /*!< 12x12字体区域起始地址  */
#define FONT_16x16_ADDR 0x1D2BBE0 /*!< 16x16字体区域起始地址  */
#define FONT_20x20_ADDR 0x1D66100 /*!< 20x20字体区域起始地址  */
#define FONT_24x24_ADDR 0x1DD3680 /*!< 24x24字体区域起始地址 */
#define FONT_32x32_ADDR 0x1E569E0 /*!< 32x32字体区域起始地址 */

#define GB2312_TABLE_ADDR 0x1F3FE00 /*!< GB2312对照表地址 */
#define UTF8_TABLE_ADDR 0x1F472D0 /*!< utf8对照表地址 */
#define FONT_FLAG_ADDR 0x1F572F0    /*!< 字库标志存储地址 */
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
     *                          对照表结构定义
     ******************************************************************************/

    /**
     * @brief  GB2312对照表数据项(每条4字节)
     * @note   格式: uint16_t gbk_code + uint16_t index
     */
    typedef struct
    {
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
    const uint8_t *UTF8_FindFont_Flash(const uint8_t *utf8_text,
                                       uint8_t font_size);
    
#ifdef __cplusplus
}
#endif

#endif // FLASH_FONT_H
