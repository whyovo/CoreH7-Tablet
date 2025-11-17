/**
 ******************************************************************************
 * @file    flash_font.c
 * @author  菜菜why（B站：菜菜whyy）
 * @brief   Flash字库驱动实现文件（内存映射模式）
 ******************************************************************************
 * @attention
 *
 * 本文件实现：
 * - 字库数据访问（纯内存映射模式）
 * - GB2312字模数据访问
 * - 字库完整性校验
 *
 * 使用前提：
 * - 字库已通过STM32CubeProgrammer预烧录到QSPI Flash
 * - QSPI已配置为内存映射模式（由外部模块负责）
 * - 字库数据可通过 W25Qxx_Mem_Addr 直接访问
 *
 * 性能说明：
 * - 读取速度: >50MB/s (仅受QSPI时钟和Cache影响)
 * - 零拷贝访问，无需额外缓冲区
 *
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

static GB2312_TableHeader_t g_table_header; /*!< 对照表头信息缓存 */
static uint8_t g_font_initialized = 0;      /*!< 初始化标志 */

/*******************************************************************************
 *                              私有函数实现
 ******************************************************************************/

/**
 * @brief  获取字体存储基地址
 * @param  font_size: 字体大小(12/16/20/24/32)
 * @retval Flash地址偏移, 0表示无效尺寸
 */
static uint32_t GetFontBaseAddr(uint8_t font_size)
{
    switch (font_size)
    {
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
int8_t FlashFont_Init(void)
{
    const FontWriteFlag_t *flag;
    const GB2312_TableHeader_t *table_header;

    // 通过内存映射读取标志位
    flag = (const FontWriteFlag_t *)(W25Qxx_Mem_Addr + FONT_FLAG_ADDR);

    // 验证标志位魔数
    if (flag->magic != FLAG_MAGIC)
    {
        DEBUG_ERROR("字库标志无效，字库可能未烧录");
        return -1;
    }

    // 通过内存映射读取GB2312对照表头部
    table_header = (const GB2312_TableHeader_t *)(W25Qxx_Mem_Addr + GB2312_TABLE_ADDR);

    // 验证对照表魔数
    if (table_header->magic != 0x54424C47) // "TBLG"
    {
        DEBUG_ERROR("GB2312对照表标志无效，对照表可能未烧录");
        return -2;
    }

    // 缓存对照表头信息
    memcpy(&g_table_header, table_header, sizeof(GB2312_TableHeader_t));
    g_font_initialized = 1;

    return FLASHFONT_OK;
}

/**
 * @brief  验证指定字库是否已烧录
 * @param  font_size: 字体大小(12/16/20/24/32)
 * @retval 1-已烧录, 0-未烧录
 */
int8_t FlashFont_IsValid(uint8_t font_size)
{
    uint32_t base_addr = GetFontBaseAddr(font_size);
    const FontHeader_GB2312_t *header;

    if (base_addr == 0)
    {
        return 0;
    }

    // 通过内存映射读取字库头信息
    header = (const FontHeader_GB2312_t *)(W25Qxx_Mem_Addr + base_addr);

    // 验证魔数
    return (header->magic == FONT_MAGIC) ? 1 : 0;
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
int16_t GB2312_FindIndex_Flash(const char *text)
{
    uint16_t search_gbk;
    const uint8_t *pData;
    const GB2312_TableEntry_t *pEntry;

    if (!g_font_initialized)
    {
        DEBUG_ERROR("GB2312_FindIndex_Flash: 字库未初始化");
        return -1;
    }

    // 将输入字符转为GBK码
    search_gbk = ((uint16_t)text[0] << 8) | (uint8_t)text[1];

    // 计算数据区起始地址(内存映射)
    pData = (const uint8_t *)(W25Qxx_Mem_Addr + GB2312_TABLE_ADDR +
                              g_table_header.data_offset);
    pEntry = (const GB2312_TableEntry_t *)pData;

    // 线性查找
    for (uint32_t i = 0; i < g_table_header.char_count; i++)
    {
        if (pEntry[i].gbk_code == 0xFFFF)
        {
            break; // 遇到特殊标记，提前结束
        }

        if (pEntry[i].gbk_code == search_gbk)
        {
            return pEntry[i].index; // 找到
        }
    }

    return -1; // 未找到
}

#endif // FLASH_FONT_ENABLE
