/**
 ******************************************************************************
 * @file    lcd_spi.h
 * @author  菜菜why（B站：菜菜whyy）
 * @brief   SPI LCD 2.00寸屏幕驱动头文件 (ST7789 控制器)
 *          提供RGB565格式图形显示、中英文字符显示、基本2D绘图等功能
 ******************************************************************************
 * @attention
 * 配置说明：
 * - 屏幕格式：16位RGB565
 * - SPI通信速度：60MHz
 * - 显存刷新：60Hz
 *
 * 使用示例：
 * ```c
 * // 初始化
 * SPI_LCD_Init();
 *
 * // 显示中英文混合文本
 * LCD_SetTextFont(&CH_Font24);
 * LCD_SetColor(LCD_BLUE);
 * LCD_DisplayText(10, 10, "反客科技STM32");
 *
 * // 绘制矩形和圆形
 * LCD_SetColor(LCD_RED);
 * LCD_DrawRect(50, 50, 100, 80);
 * LCD_FillCircle(120, 160, 30);
 * ```
 *
 ******************************************************************************
 */

#ifndef LCD_SPI_H
#define LCD_SPI_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "init.h"
#include <stdio.h>
#include "lcd_fonts.h"
#include "lcd_image.h"

#ifdef LCD_SPI_ENABLE
    typedef struct _pFont pFONT; // 前向声明

    /*******************************************************************************
     *                              GPIO引脚定义
     ******************************************************************************/

#define LCD_Backlight_PIN GPIO_PIN_6                               // 背光  引脚
#define LCD_Backlight_PORT GPIOH                                   // 背光 GPIO端口
#define GPIO_LDC_Backlight_CLK_ENABLE __HAL_RCC_GPIOH_CLK_ENABLE() // 背光 GPIO时钟

#define LCD_Backlight_OFF HAL_GPIO_WritePin(LCD_Backlight_PORT, LCD_Backlight_PIN, GPIO_PIN_RESET); // 低电平，关闭背光
#define LCD_Backlight_ON HAL_GPIO_WritePin(LCD_Backlight_PORT, LCD_Backlight_PIN, GPIO_PIN_SET);    // 高电平，开启背光

#define LCD_DC_PIN GPIO_PIN_12                              // 数据指令选择  引脚
#define LCD_DC_PORT GPIOG                                   // 数据指令选择  GPIO端口
#define GPIO_LDC_DC_CLK_ENABLE __HAL_RCC_GPIOG_CLK_ENABLE() // 数据指令选择  GPIO时钟

#define LCD_DC_Command HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_RESET); // 低电平，指令传输
#define LCD_DC_Data HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET);      // 高电平，数据传输

    /*******************************************************************************
     *                              屏幕参数配置
     ******************************************************************************/

#define LCD_Width 240    /*!< LCD像素宽度 */
#define LCD_Height 320   /*!< LCD像素高度 */


    /*******************************************************************************
     *                             配置是否用外部字库，以及编码方式
     ******************************************************************************/
#define USE_FLASH_FONT /*!< 定义了：使用Flash字库, 注释后：使用内置取模字库 */
// #define IS_GB2312 /*!< 定义了：使用gb2312, 注释后：使用UTF8 */

#ifndef FLASH_FONT_ENABLE
    #ifdef USE_FLASH_FONT
    #undef USE_FLASH_FONT 
    #endif
#endif /*!< 确保当有定义的时候才能使用Flash字库 */

#ifdef USE_FLASH_FONT
#include "../QSPI/flash_font.h"
#endif
/*******************************************************************************
 *                              显示方向定义
 ******************************************************************************/
/**
 * @brief 屏幕显示方向枚举
 * @note  使用示例：LCD_SetDirection(Direction_H) 设置横屏显示
 */
#define Direction_H 0      /*!< 横屏显示 */
#define Direction_H_Flip 1 /*!< 横屏显示，上下翻转 */
#define Direction_V 2      /*!< 竖屏显示 */
#define Direction_V_Flip 3 /*!< 竖屏显示，上下翻转 */

/*******************************************************************************
 *                              数字显示模式
 ******************************************************************************/
/**
 * @brief 数字显示时多余位填充模式
 * @note  仅用于 LCD_DisplayNumber() 和 LCD_DisplayDecimals()
 * @note  示例：LCD_ShowNumMode(Fill_Zero) 设置填充0，123显示为000123
 */
#define Fill_Zero 0  /*!< 多余位填充0 */
#define Fill_Space 1 /*!< 多余位填充空格 */

/*******************************************************************************
 *                              常用颜色定义 (RGB888)
 ******************************************************************************/
/**
 * @brief 24位RGB888颜色定义，会自动转换为16位RGB565
 * @note  可用调色板获取RGB值，然后通过 LCD_SetColor() 设置
 */

/* 基础颜色 */
#define LCD_WHITE 0xFFFFFF   /*!< 纯白色 */
#define LCD_BLACK 0x000000   /*!< 纯黑色 */
#define LCD_BLUE 0x0000FF    /*!< 纯蓝色 */
#define LCD_GREEN 0x00FF00   /*!< 纯绿色 */
#define LCD_RED 0xFF0000     /*!< 纯红色 */
#define LCD_CYAN 0x00FFFF    /*!< 蓝绿色 */
#define LCD_MAGENTA 0xFF00FF /*!< 紫红色 */
#define LCD_YELLOW 0xFFFF00  /*!< 黄色 */
#define LCD_GREY 0x2C2C2C    /*!< 灰色 */

/* 亮色系 */
#define LIGHT_BLUE 0x8080FF    /*!< 亮蓝色 */
#define LIGHT_GREEN 0x80FF80   /*!< 亮绿色 */
#define LIGHT_RED 0xFF8080     /*!< 亮红色 */
#define LIGHT_CYAN 0x80FFFF    /*!< 亮蓝绿色 */
#define LIGHT_MAGENTA 0xFF80FF /*!< 亮紫红色 */
#define LIGHT_YELLOW 0xFFFF80  /*!< 亮黄色 */
#define LIGHT_GREY 0xA3A3A3    /*!< 亮灰色 */

/* 暗色系 */
#define DARK_BLUE 0x000080    /*!< 暗蓝色 */
#define DARK_GREEN 0x008000   /*!< 暗绿色 */
#define DARK_RED 0x800000     /*!< 暗红色 */
#define DARK_CYAN 0x008080    /*!< 暗蓝绿色 */
#define DARK_MAGENTA 0x800080 /*!< 暗紫红色 */
#define DARK_YELLOW 0x808000  /*!< 暗黄色 */
#define DARK_GREY 0x404040    /*!< 暗灰色 */

    /*******************************************************************************
     *                              基础控制函数
     ******************************************************************************/

    /**
     * @brief  初始化SPI LCD
     * @note   配置GPIO、SPI通信、ST7789控制器参数
     * @note   初始化后自动清屏、点亮背光
     * @retval None
     */
    void SPI_LCD_Init(void);

    /**
     * @brief  清屏函数
     * @note   将整个屏幕清除为当前背景色
     * @note   需先调用 LCD_SetBackColor() 设置背景色
     * @retval None
     */
    void LCD_Clear(void);

    /**
     * @brief  局部清屏函数
     * @param  x 起始水平坐标
     * @param  y 起始垂直坐标
     * @param  width 清除区域宽度
     * @param  height 清除区域高度
     * @note   示例：LCD_ClearRect(10, 10, 100, 50) 清除(10,10)起始的100x50区域
     * @retval None
     */
    void LCD_ClearRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height);

    /**
     * @brief  设置显示区域坐标
     * @param  x1 起始水平坐标
     * @param  y1 起始垂直坐标
     * @param  x2 终止水平坐标
     * @param  y2 终止垂直坐标
     * @note   内部函数，用户一般不直接调用
     * @retval None
     */
    void LCD_SetAddress(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

    /*******************************************************************************
     *                              颜色和方向设置
     ******************************************************************************/

    /**
     * @brief  设置画笔颜色
     * @param  Color 24位RGB888颜色值
     * @note   用于字符、图形绘制
     * @note   示例：LCD_SetColor(0x0000FF) 设置蓝色
     * @retval None
     */
    void LCD_SetColor(uint32_t Color);

    /**
     * @brief  设置背景颜色
     * @param  Color 24位RGB888颜色值
     * @note   用于清屏和字符显示背景
     * @note   示例：LCD_SetBackColor(0x000000) 设置黑色背景
     * @retval None
     */
    void LCD_SetBackColor(uint32_t Color);

    /**
     * @brief  设置显示方向
     * @param  direction 显示方向 (Direction_H/Direction_V/Direction_H_Flip/Direction_V_Flip)
     * @note   示例：LCD_SetDirection(Direction_H) 横屏显示
     * @retval None
     */
    void LCD_SetDirection(uint8_t direction);

    /*******************************************************************************
     *                              ASCII字符显示
     ******************************************************************************/

    /**
     * @brief  显示单个ASCII字符
     * @param  x 起始水平坐标
     * @param  y 起始垂直坐标
     * @param  c ASCII字符
     * @note   示例：LCD_DisplayChar(10, 10, 'A')
     * @retval None
     */
    void LCD_DisplayChar(uint16_t x, uint16_t y, uint8_t c);

    /**
     * @brief  显示ASCII字符串
     * @param  x 起始水平坐标
     * @param  y 起始垂直坐标
     * @param  p 字符串首地址
     * @note   示例：LCD_DisplayString(10, 10, "Hello")
     * @retval None
     */
    void LCD_DisplayString(uint16_t x, uint16_t y, char *p);

    /*******************************************************************************
     *                              中文字符显示
     ******************************************************************************/
    /**
     * @brief  获取当前中文字体大小
     * @retval 字体大小 (12/16/20/24/32)
     */
    uint8_t LCD_GetChineseFontSize(void);
    /**
     * @brief  设置中英文混合字体
     * @param  font_size 字体大小 (12/16/20/24/32)
     * @note   自动匹配对应的ASCII和中文字体
     * @note   示例：LCD_SetTextFont(24) 设置24x24中文+24x12 ASCII
     * @retval None
     */
    void LCD_SetTextFont(uint8_t font_size);

    /**
     * @brief  显示单个汉字
     * @param  x 起始水平坐标
     * @param  y 起始垂直坐标
     * @param  pText 汉字字符串（单个汉字）
     * @note   示例：LCD_DisplayChinese(10, 10, "反")
     * @retval None
     */
    void LCD_DisplayChinese(uint16_t x, uint16_t y, char *pText);

    /**
     * @brief  显示中英文混合字符串
     * @param  x 起始水平坐标
     * @param  y 起始垂直坐标
     * @param  pText 字符串首地址
     * @note   示例：LCD_DisplayText(10, 10, "反客科技STM32")
     * @retval None
     */
    void LCD_DisplayText(uint16_t x, uint16_t y, char *pText);

    /*******************************************************************************
     *                              数字显示
     ******************************************************************************/

    /**
     * @brief  设置数字显示模式
     * @param  mode 填充模式 (Fill_Zero/Fill_Space)
     * @note   示例：LCD_ShowNumMode(Fill_Zero) 多余位填充0
     * @retval None
     */
    void LCD_ShowNumMode(uint8_t mode);

    /**
     * @brief  显示整数
     * @param  x 起始水平坐标
     * @param  y 起始垂直坐标
     * @param  number 整数值 (-2147483648~2147483647)
     * @param  len 显示位数（含符号位）
     * @note   示例：LCD_DisplayNumber(10, 10, 123, 5) 显示为"  123"或"00123"
     * @retval None
     */
    void LCD_DisplayNumber(uint16_t x, uint16_t y, int32_t number, uint8_t len);

    /**
     * @brief  显示小数
     * @param  x 起始水平坐标
     * @param  y 起始垂直坐标
     * @param  number 浮点数值
     * @param  len 总位数（含小数点和符号）
     * @param  decs 小数位数
     * @note   示例：LCD_DisplayDecimals(10, 10, 1.12345, 8, 4) 显示"  1.1235"
     * @retval None
     */
    void LCD_DisplayDecimals(uint16_t x, uint16_t y, double number, uint8_t len, uint8_t decs);

    /*******************************************************************************
     *                              2D图形绘制
     ******************************************************************************/

    /**
     * @brief  绘制单个像素点
     * @param  x 水平坐标
     * @param  y 垂直坐标
     * @param  color 24位RGB888颜色值
     * @note   示例：LCD_DrawPoint(10, 10, 0x0000FF) 绘制蓝色点
     * @retval None
     */
    void LCD_DrawPoint(uint16_t x, uint16_t y, uint32_t color);

    /**
     * @brief  绘制垂直线（快速版）
     * @param  x 水平坐标
     * @param  y 起始垂直坐标
     * @param  height 线段高度
     * @note   比 LCD_DrawLine() 快，优先使用
     * @retval None
     */
    void LCD_DrawLine_V(uint16_t x, uint16_t y, uint16_t height);

    /**
     * @brief  绘制水平线（快速版）
     * @param  x 起始水平坐标
     * @param  y 垂直坐标
     * @param  width 线段宽度
     * @note   比 LCD_DrawLine() 快，优先使用
     * @retval None
     */
    void LCD_DrawLine_H(uint16_t x, uint16_t y, uint16_t width);

    /**
     * @brief  两点间绘制直线（Bresenham算法）
     * @param  x1 起点水平坐标
     * @param  y1 起点垂直坐标
     * @param  x2 终点水平坐标
     * @param  y2 终点垂直坐标
     * @note   移植自ST官方例程
     * @retval None
     */
    void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

    /**
     * @brief  绘制矩形框
     * @param  x 起始水平坐标
     * @param  y 起始垂直坐标
     * @param  width 矩形宽度
     * @param  height 矩形高度
     * @retval None
     */
    void LCD_DrawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height);

    /**
     * @brief  绘制圆形轮廓
     * @param  x 圆心水平坐标
     * @param  y 圆心垂直坐标
     * @param  r 半径
     * @note   移植自ST官方例程
     * @retval None
     */
    void LCD_DrawCircle(uint16_t x, uint16_t y, uint16_t r);

    /**
     * @brief  绘制椭圆轮廓
     * @param  x 中心水平坐标
     * @param  y 中心垂直坐标
     * @param  r1 水平半轴长度
     * @param  r2 垂直半轴长度
     * @note   移植自ST官方例程
     * @retval None
     */
    void LCD_DrawEllipse(int x, int y, int r1, int r2);

    /*******************************************************************************
     *                              区域填充
     ******************************************************************************/

    /**
     * @brief  填充矩形区域
     * @param  x 起始水平坐标
     * @param  y 起始垂直坐标
     * @param  width 矩形宽度
     * @param  height 矩形高度
     * @note   使用当前画笔色填充
     * @retval None
     */
    void LCD_FillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height);

    /**
     * @brief  填充圆形区域
     * @param  x 圆心水平坐标
     * @param  y 圆心垂直坐标
     * @param  r 半径
     * @note   移植自ST官方例程
     * @retval None
     */
    void LCD_FillCircle(uint16_t x, uint16_t y, uint16_t r);

    /*******************************************************************************
     *                              图像显示
     ******************************************************************************/

    /**
     * @brief  显示单色图像
     * @param  x 起始水平坐标
     * @param  y 起始垂直坐标
     * @param  width 图像宽度
     * @param  height 图像高度
     * @param  pImage 图像数据首地址（取模数据）
     * @note   需事先通过取模软件生成图像数据
     * @retval None
     */
    void LCD_DrawImage(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t *pImage);

    /**
     * @brief  批量复制缓冲区到屏幕（LVGL移植用）
     * @param  x 起始水平坐标
     * @param  y 起始垂直坐标
     * @param  width 区域宽度
     * @param  height 区域高度
     * @param  DataBuff RGB565数据缓冲区
     * @note   用于LVGL或摄像头图像显示
     * @retval None
     */
    void LCD_CopyBuffer(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t *DataBuff);

#endif

#ifdef __cplusplus
}
#endif

#endif // __spi_lcd
