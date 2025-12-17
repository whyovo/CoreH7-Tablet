#ifndef LV_FONT_QSPI_H
#define LV_FONT_QSPI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "flash_font.h"
#include "lvgl.h"

/**
 * @brief 初始化QSPI字库 (支持UTF-8)
 */
void lv_font_qspi_init(void);

/**
 * @brief 获取指定大小的LVGL字体对象
 * @param font_size: 字体大小(12/16/20/24/32)
 * @retval lv_font_t* 字体指针，失败返回NULL
 */
lv_font_t *lv_font_qspi_get_by_size(uint8_t font_size);

#ifdef __cplusplus
}
#endif

#endif
