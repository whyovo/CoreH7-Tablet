/**
 * @file oled.h
 * @author 菜菜why（B站：菜菜whyy）
 * @brief OLED 统一驱动接口 (支持 硬件I2C / 软件I2C 切换)
 * @attention
 * - 请在 config.h 中定义 OLED_I2C_ENABLE 以启用此模块
 * - 在本文件中选择 OLED_USE_HARD_I2C 或 OLED_USE_SOFT_I2C
 *
 *  【汉字】取模方式：阴码、逆向、逐行式、C51格式
 * 【单色图标】
 *  1.取模参数：阴码、逆向、逐行式、C51格式
 *  2.数据格式：1位/像素 (黑白两色)
 */

#ifndef __OLED_H__
#define __OLED_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "config.h"

/* 仅当 config.h 中定义了 OLED_I2C_ENABLE 时编译 */
#ifdef OLED_I2C_ENABLE

#include <stdint.h>
#include "oledfont.h"

/* ================= 用户配置区域 ================= */

/**
 * @brief 驱动模式选择
 * @note  请取消注释下方宏定义之一以选择硬件或软件 I2C
 */
 #define OLED_USE_HARD_I2C    /*!< 使用硬件 I2C (DMA模式) */
//#define OLED_USE_SOFT_I2C /*!< 使用软件 I2C (GPIO模拟) */

#if defined(OLED_USE_HARD_I2C) && defined(OLED_USE_SOFT_I2C)
#error "Please select only one OLED driver mode (Hard or Soft) in oled.h"
#endif
#if !defined(OLED_USE_HARD_I2C) && !defined(OLED_USE_SOFT_I2C)
#error "Please select at least one OLED driver mode in oled.h"
#endif

/* ---------------- 硬件 I2C 配置 ---------------- */
#ifdef OLED_USE_HARD_I2C
    extern I2C_HandleTypeDef hi2c1;
#define OLED_HARD_HANDLE hi2c1
#endif

/* ---------------- 软件 I2C 配置 ---------------- */
#ifdef OLED_USE_SOFT_I2C
#define OLED_SOFT_SCL_PORT GPIOB
#define OLED_SOFT_SCL_PIN GPIO_PIN_9
#define OLED_SOFT_SDA_PORT GPIOB
#define OLED_SOFT_SDA_PIN GPIO_PIN_8

/**
 * @brief 软件 I2C 的默认单比特延时（微秒）
 * @details 增大该值会降低 SCL 频率（更保守、抗干扰更好）
 */
#define OLED_SOFT_DELAY_US 5
#endif

    /* ================= 核心控制函数 ================= */

    /**
     * @brief 初始化 OLED
     * @note  发送初始化命令序列，配置 I2C 接口。
     *        在调用其它显示接口前应首先调用此函数。
     */
    void OLED_Init(void);

    /**
     * @brief 清屏（将显存所有字节写为 0 并刷新）
     * @note  将显示全部置黑，并立即更新屏幕。
     */
    void OLED_Clear(void);

    /**
     * @brief 清空显存（不立即刷新屏幕）
     * @note  仅清除 RAM 中的缓冲区，需要调用 OLED_Refresh() 才会更新到屏幕。
     */
    void OLED_ClearRAM(void);

    /**
     * @brief 将显存内容刷新到屏幕
     * @note  硬件模式：DMA 异步传输；软件模式：阻塞传输。
     *        所有绘图函数只改变显存，必须调用此函数才会显示。
     */
    void OLED_Refresh(void);

    /**
     * @brief 开启显示
     * @note  打开电荷泵和显示开关 (Display ON)
     */
    void OLED_Display_On(void);

    /**
     * @brief 关闭显示
     * @note  关闭电荷泵和显示开关 (Display OFF)，进入休眠
     */
    void OLED_Display_Off(void);

    /**
     * @brief 全屏点亮
     * @note  测试用，将显存全部置为 0xFF 并刷新
     */
    void OLED_On(void);

    /* ================= 绘图函数 (操作显存) ================= */

    /**
     * @brief 画点
     * @param x: 列坐标 (0~127)
     * @param y: 行坐标 (0~63)
     * @param t: 1 亮, 0 灭
     */
    void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t t);

    /**
     * @brief 显示单个 ASCII 字符
     * @param x: 起始列
     * @param y: 起始页 (0~7)
     * @param chr: 字符（如 'A' 或 '0'）
     * @param Char_Size: 字体大小 (16: 8x16, 其他: 6x8)
     */
    void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t Char_Size);

    /**
     * @brief 显示字符串
     * @param x: 起始列
     * @param y: 起始页
     * @param chr: 字符串指针 (以 '\0' 结尾)
     * @param Char_Size: 字体大小
     * @note  当到达行末时，函数会自动换行到下一页
     */
    void OLED_ShowString(uint8_t x, uint8_t y, uint8_t *chr, uint8_t Char_Size);

    /**
     * @brief 显示整数（按固定宽度显示）
     * @param x: 起始列
     * @param y: 起始页
     * @param num: 要显示的无符号整数
     * @param len: 显示的位数（总宽度），前导零替换为空格
     * @param size2: 字体高度（像素），影响每位占据的列宽
     */
    void OLED_ShowNum(uint8_t x, uint8_t y, unsigned int num, uint8_t len, uint8_t size2);

    /**
     * @brief 显示浮点数
     * @param x: 起始列
     * @param y: 起始页
     * @param num: 浮点数
     * @param a: 整数部分长度
     * @param b: 小数部分长度
     * @param size: 字体大小
     */
    void OLED_ShowFloat(uint8_t x, uint8_t y, float num, uint8_t a, uint8_t b, uint8_t size);

    /**
     * @brief 显示汉字
     * @param x: 起始列
     * @param y: 起始页
     * @param no: 汉字索引（在 Hzk 取模数组中的序号）
     * @note  每个汉字通常为 16x16 点阵
     */
    void OLED_ShowChinese(uint8_t x, uint8_t y, uint8_t no);

    /**
     * @brief 显示位图图像
     * @param x: 起始列 (0~127)
     * @param y: 起始页 (0~7)
     * @param w: 图片宽度 (像素)
     * @param h: 图片高度 (像素)
     * @param bitmap: 图片数据指针 (按页格式，每字节 8 像素竖列)
     */
    void OLED_ShowImage(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *bitmap);

    /**
     * @brief 播放 GIF 动画
     * @param x: 起始列
     * @param y: 起始页
     * @param w: 帧宽
     * @param h: 帧高
     * @param frames: 帧数据指针数组
     * @param frame_count: 帧数
     * @param frame_delay_ms: 每帧延时 (毫秒)
     * @param loop_count: 循环次数 (0 表示 1 次)
     * @note  本函数阻塞执行
     */
    void OLED_ShowGIF(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                      const uint8_t *frames[], uint16_t frame_count,
                      uint16_t frame_delay_ms, uint16_t loop_count);

    /**
     * @brief 功能测试演示
     */
    void OLED_Test_Demo(void);

#endif /* OLED_I2C_ENABLE */

#ifdef __cplusplus
}
#endif

#endif /* __OLED_H__ */
											