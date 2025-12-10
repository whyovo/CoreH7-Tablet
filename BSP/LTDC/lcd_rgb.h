/**
 ******************************************************************************
 * @file    lcd_rgb.h
 * @author  菜菜why（B站：菜菜whyy）
 * @brief   RGB LCD 800*480分辨率屏幕驱动头文件 (LTDC + DMA2D)
 *          提供RGB888/RGB565/ARGB8888等格式图形显示、中英文字符显示、基本2D绘图等功能
 ******************************************************************************
 * @attention
 * 配置说明：
 * - 屏幕分辨率：800x480
 * - 显存位置：外部SDRAM (起始地址0xC0000000)
 * - LTDC时钟：33MHz (约60Hz刷新率)
 * - 支持双层显示
 *
 * 使用示例：
 * ```c
 * // 初始化
 * RGB_LCD_Init();
 *
 * // 显示中英文混合文本
 * RGB_LCD_SetTextFont(24);
 * RGB_LCD_SetColor(RGB_LCD_BLUE);
 * RGB_LCD_DisplayText(10, 10, "你好hello");
 *
 * // 绘制矩形和圆形
 * RGB_LCD_SetColor(RGB_LCD_RED);
 * RGB_LCD_DrawRect(50, 50, 100, 80);
 * RGB_LCD_FillCircle(120, 160, 30);
 * ```
 *
 ******************************************************************************
 */

#ifndef __LCD_RGB_H
#define __LCD_RGB_H

#include "config.h"
#include "lcd_fonts.h"
#include "lcd_image.h"
#include "sdram.h"
#include <stdio.h>

#ifdef LCD_RGB_ENABLE
typedef struct _pFont pFONT; // 前向声明
/*******************************************************************************
 *                             配置是否用外部字库
 ******************************************************************************/
#define USE_FLASH_FONT_RGB /*!< 定义了：使用Flash字库,                 \
                              注释后：使用内置取模字库 */
// #define IS_GB2312_RGB     /*!< 定义了：使用gb2312, 注释后：使用UTF8 */

#ifndef FLASH_FONT_ENABLE
#ifdef USE_FLASH_FONT_RGB
#undef USE_FLASH_FONT_RGB
#endif
#endif /*!< 确保当有定义的时候才能使用Flash字库 */

#ifdef USE_FLASH_FONT_RGB
#include "flash_font.h"
#endif
/*******************************************************************************
 *                              LTDC层配置
 ******************************************************************************/

/**
 * @brief 显示层数定义
 * @note  H7可驱动两层显示，过多层会增加SDRAM占用导致花屏
 */
#define RGB_LCD_NUM_LAYERS 1

/**
 * @brief Layer0 颜色格式定义
 * @note  可选：LTDC_PIXEL_FORMAT_RGB565/ARGB1555/ARGB4444/RGB888/ARGB8888
 */
#define RGB_ColorMode_0 LTDC_PIXEL_FORMAT_RGB565
// #define	RGB_ColorMode_0   LTDC_PIXEL_FORMAT_ARGB1555
// #define	RGB_ColorMode_0   LTDC_PIXEL_FORMAT_ARGB4444
// #define	RGB_ColorMode_0   LTDC_PIXEL_FORMAT_RGB888
// #define	RGB_ColorMode_0   LTDC_PIXEL_FORMAT_ARGB8888

#if RGB_LCD_NUM_LAYERS == 2 // 如果开启了双层，则在此处定义 layer1 的颜色格式

/**
 * @brief Layer1 颜色格式定义 (前景层)
 * @note  建议使用带透明通道的格式
 *        - ARGB1555: 1位透明度 (透明/不透明)
 *        - ARGB4444: 4位透明度 (16级)
 *        - ARGB8888: 8位透明度 (256级)
 */
//	#define	RGB_ColorMode_1   LTDC_PIXEL_FORMAT_RGB565
#define RGB_ColorMode_1 LTDC_PIXEL_FORMAT_ARGB1555
//	#define	RGB_ColorMode_1   LTDC_PIXEL_FORMAT_ARGB4444
// #define	RGB_ColorMode_1   LTDC_PIXEL_FORMAT_RGB888
//	#define	RGB_ColorMode_1   LTDC_PIXEL_FORMAT_ARGB8888

#endif

/*******************************************************************************
 *                              显示方向定义
 ******************************************************************************/

/**
 * @brief 屏幕显示方向枚举
 * @note  使用示例：RGB_LCD_DisplayDirection(Direction_H_RGB) 设置横屏显示
 */
#define Direction_H_RGB 0 /*!< LCD横屏显示 */
#define Direction_V_RGB 1 /*!< LCD竖屏显示 */

/*******************************************************************************
 *                              数字显示模式
 ******************************************************************************/

/**
 * @brief 数字显示时多余位填充模式
 * @note  仅用于 RGB_LCD_DisplayNumber() 和 RGB_LCD_DisplayDecimals()
 * @note  示例：RGB_LCD_ShowNumMode(Fill_Zero_RGB) 设置填充0，123显示为000123
 */
#define Fill_Zero_RGB 0  /*!< 填充0 */
#define Fill_Space_RGB 1 /*!< 填充空格 */

/*******************************************************************************
 *                              常用颜色定义 (ARGB8888)
 ******************************************************************************/

/**
 * @brief 32位ARGB8888颜色定义，自动转换为对应颜色格式
 * @note  格式：0xAARRGGBB (A:透明度 R:红 G:绿 B:蓝)
 *        - A通道：0xFF不透明，0x00完全透明
 *        - 仅ARGB格式支持透明度，RGB格式忽略A通道
 */

/* 基础颜色 */
#define RGB_LCD_WHITE 0xffFFFFFF   /*!< 纯白色 */
#define RGB_LCD_BLACK 0xff000000   /*!< 纯黑色 */
#define RGB_LCD_BLUE 0xff0000FF    /*!< 纯蓝色 */
#define RGB_LCD_GREEN 0xff00FF00   /*!< 纯绿色 */
#define RGB_LCD_RED 0xffFF0000     /*!< 纯红色 */
#define RGB_LCD_CYAN 0xff00FFFF    /*!< 蓝绿色 */
#define RGB_LCD_MAGENTA 0xffFF00FF /*!< 紫红色 */
#define RGB_LCD_YELLOW 0xffFFFF00  /*!< 黄色 */
#define RGB_LCD_GREY 0xff2C2C2C    /*!< 灰色 */

/* 亮色系 */
#define RGB_LIGHT_BLUE 0xff8080FF    /*!< 亮蓝色 */
#define RGB_LIGHT_GREEN 0xff80FF80   /*!< 亮绿色 */
#define RGB_LIGHT_RED 0xffFF8080     /*!< 亮红色 */
#define RGB_LIGHT_CYAN 0xff80FFFF    /*!< 亮蓝绿色 */
#define RGB_LIGHT_MAGENTA 0xffFF80FF /*!< 亮紫红色 */
#define RGB_LIGHT_YELLOW 0xffFFFF80  /*!< 亮黄色 */
#define RGB_LIGHT_GREY 0xffA3A3A3    /*!< 亮灰色 */

/* 暗色系 */
#define RGB_DARK_BLUE 0xff000080    /*!< 暗蓝色 */
#define RGB_DARK_GREEN 0xff008000   /*!< 暗绿色 */
#define RGB_DARK_RED 0xff800000     /*!< 暗红色 */
#define RGB_DARK_CYAN 0xff008080    /*!< 暗蓝绿色 */
#define RGB_DARK_MAGENTA 0xff800080 /*!< 暗紫红色 */
#define RGB_DARK_YELLOW 0xff808000  /*!< 暗黄色 */
#define RGB_DARK_GREY 0xff404040    /*!< 暗灰色 */

/*******************************************************************************
 *                              屏幕参数配置
 ******************************************************************************/

#define RGB_LCD_Width 800                 /*!< LCD像素宽度 */
#define RGB_LCD_Height 480                /*!< LCD像素高度 */
#define RGB_LCD_MemoryAdd SDRAM_BANK_ADDR /*!< 显存起始地址 */

/**
 * @brief 计算每像素字节数
 * @note  显存所需 = 分辨率 × 字节数 (如800×480×2 = 768000字节)
 */
#if (RGB_ColorMode_0 == LTDC_PIXEL_FORMAT_RGB565 ||                            \
     RGB_ColorMode_0 == LTDC_PIXEL_FORMAT_ARGB1555 ||                          \
     RGB_ColorMode_0 == LTDC_PIXEL_FORMAT_ARGB4444)
#define RGB_BytesPerPixel_0 2 /*!< 16位色模式每像素2字节 */
#elif RGB_ColorMode_0 == LTDC_PIXEL_FORMAT_RGB888
#define RGB_BytesPerPixel_0 3 /*!< 24位色模式每像素3字节 */
#else
#define RGB_BytesPerPixel_0 4 /*!< 32位色模式每像素4字节 */
#endif

#if RGB_LCD_NUM_LAYERS == 2

#if (RGB_ColorMode_1 == LTDC_PIXEL_FORMAT_RGB565 ||                            \
     RGB_ColorMode_1 == LTDC_PIXEL_FORMAT_ARGB1555 ||                          \
     RGB_ColorMode_1 == LTDC_PIXEL_FORMAT_ARGB4444)
#define RGB_BytesPerPixel_1 2 /*!< 16位色模式每像素2字节 */
#elif RGB_ColorMode_1 == LTDC_PIXEL_FORMAT_RGB888
#define RGB_BytesPerPixel_1 3 /*!< 24位色模式每像素3字节 */
#else
#define RGB_BytesPerPixel_1 4 /*!< 32位色模式每像素4字节 */
#endif

#define RGB_LCD_MemoryAdd_OFFSET                                                   \
  RGB_LCD_Width *RGB_LCD_Height *RGB_BytesPerPixel_0 /*!< Layer1显存偏移地址 \
                                                      */

#endif

/*******************************************************************************
 *                              LCD背光引脚
 ******************************************************************************/

#define RGB_LCD_Backlight_PIN GPIO_PIN_6
#define RGB_LCD_Backlight_PORT GPIOH
#define RGB_GPIO_LDC_Backlight_CLK_ENABLE __HAL_RCC_GPIOH_CLK_ENABLE()

#define RGB_LCD_Backlight_OFF                                                  \
  HAL_GPIO_WritePin(RGB_LCD_Backlight_PORT, RGB_LCD_Backlight_PIN,             \
                    GPIO_PIN_RESET); /*!< 关闭背光 */
#define RGB_LCD_Backlight_ON                                                   \
  HAL_GPIO_WritePin(RGB_LCD_Backlight_PORT, RGB_LCD_Backlight_PIN,             \
                    GPIO_PIN_SET); /*!< 开启背光 */

/*******************************************************************************
 *                              基础控制函数
 ******************************************************************************/

/**
 * @brief  初始化RGB LCD
 * @note   配置LTDC、DMA2D、背光GPIO
 * @note   初始化后自动清屏、点亮背光
 * @retval None
 */
void RGB_LCD_Init(void);

/**
 * @brief  清屏函数
 * @note   将整个屏幕清除为当前背景色，使用DMA2D加速
 * @note   需先调用 RGB_LCD_SetBackColor() 设置背景色
 * @retval None
 */
void RGB_LCD_Clear(void);

/**
 * @brief  局部清屏函数
 * @param  x 起始水平坐标 (0~799)
 * @param  y 起始垂直坐标 (0~479)
 * @param  width 清除区域宽度
 * @param  height 清除区域高度
 * @note   示例：RGB_LCD_ClearRect(10, 10, 100, 50) 清除(10,10)起始的100×50区域
 * @retval None
 */
void RGB_LCD_ClearRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height);

/*******************************************************************************
 *                              图层和颜色设置
 ******************************************************************************/

/**
 * @brief  设置当前操作的图层
 * @param  layer 图层编号 (0或1)
 * @note   Layer1在Layer0之上，通常Layer1用于前景(带透明)
 * @retval None
 */
void RGB_LCD_SetLayer(uint8_t layer);

/**
 * @brief  设置画笔颜色
 * @param  Color 32位ARGB8888颜色值
 * @note   用于字符、图形绘制
 * @note   示例：RGB_LCD_SetColor(0xFF0000FF) 设置不透明蓝色
 * @retval None
 */
void RGB_LCD_SetColor(uint32_t Color);

/**
 * @brief  设置背景颜色
 * @param  Color 32位ARGB8888颜色值
 * @note   用于清屏和字符显示背景
 * @note   示例：RGB_LCD_SetBackColor(0xFF000000) 设置黑色背景
 * @retval None
 */
void RGB_LCD_SetBackColor(uint32_t Color);

/**
 * @brief  设置显示方向
 * @param  direction 显示方向 (Direction_H_RGB横屏/Direction_V_RGB竖屏)
 * @note   示例：RGB_LCD_DisplayDirection(Direction_H_RGB) 设置横屏
 * @retval None
 */
void RGB_LCD_DisplayDirection(uint8_t direction);

/*******************************************************************************
 *                              ASCII字符显示
 ******************************************************************************/

/**
 * @brief  显示单个ASCII字符
 * @param  x 起始水平坐标 (0~799)
 * @param  y 起始垂直坐标 (0~479)
 * @param  c ASCII字符
 * @note   示例：RGB_LCD_DisplayChar(10, 10, 'A')
 * @retval None
 */
void RGB_LCD_DisplayChar(uint16_t x, uint16_t y, uint8_t c);

/**
 * @brief  显示ASCII字符串
 * @param  x 起始水平坐标 (0~799)
 * @param  y 起始垂直坐标 (0~479)
 * @param  p 字符串首地址
 * @note   示例：RGB_LCD_DisplayString(10, 10, "Hello")
 * @retval None
 */
void RGB_LCD_DisplayString(uint16_t x, uint16_t y, char *p);

/*******************************************************************************
 *                              中文字符显示
 ******************************************************************************/

/**
 * @brief  获取当前中文字体大小
 * @retval 字体大小 (12/16/20/24/32)
 */
uint8_t RGB_LCD_GetChineseFontSize(void);

/**
 * @brief  设置中英文混合字体
 * @param  font_size 字体大小 (12/16/20/24/32)
 * @note   自动匹配对应的ASCII和中文字体
 * @note   示例：RGB_LCD_SetTextFont(24) 设置24x24中文+24x12 ASCII
 * @retval None
 */
void RGB_LCD_SetTextFont(uint8_t font_size);

/**
 * @brief  显示单个汉字
 * @param  x 起始水平坐标 (0~799)
 * @param  y 起始垂直坐标 (0~479)
 * @param  pText 汉字字符串（单个汉字）
 * @note   示例：RGB_LCD_DisplayChinese(10, 10, "你")
 * @note   中文字库为小字库，需提前取模
 * @retval None
 */
void RGB_LCD_DisplayChinese(uint16_t x, uint16_t y, char *pText);

/**
 * @brief  显示中英文混合字符串
 * @param  x 起始水平坐标 (0~799)
 * @param  y 起始垂直坐标 (0~479)
 * @param  pText 字符串首地址
 * @note   示例：RGB_LCD_DisplayText(10, 10, "你好STM32")
 * @note   自动识别中英文字符
 * @retval None
 */
void RGB_LCD_DisplayText(uint16_t x, uint16_t y, char *pText);

/*******************************************************************************
 *                              数字显示
 ******************************************************************************/

/**
 * @brief  设置数字显示模式
 * @param  mode 填充模式 (Fill_Zero_RGB/Fill_Space_RGB)
 * @note   示例：RGB_LCD_ShowNumMode(Fill_Zero_RGB) 多余位填充0
 * @retval None
 */
void RGB_LCD_ShowNumMode(uint8_t mode);

/**
 * @brief  显示整数
 * @param  x 起始水平坐标 (0~799)
 * @param  y 起始垂直坐标 (0~479)
 * @param  number 整数值 (-2147483648~2147483647)
 * @param  len 显示位数（含符号位）
 * @note   示例：RGB_LCD_DisplayNumber(10, 10, 123, 5) 显示为"  123"或"00123"
 * @retval None
 */
void RGB_LCD_DisplayNumber(uint16_t x, uint16_t y, int32_t number, uint8_t len);

/**
 * @brief  显示小数
 * @param  x 起始水平坐标 (0~799)
 * @param  y 起始垂直坐标 (0~479)
 * @param  decimals 浮点数值
 * @param  len 总位数（含小数点和符号）
 * @param  decs 小数位数
 * @note   示例：RGB_LCD_DisplayDecimals(10, 10, 1.12345, 8, 4) 显示"  1.1235"
 * @retval None
 */
void RGB_LCD_DisplayDecimals(uint16_t x, uint16_t y, double decimals,
                             uint8_t len, uint8_t decs);

/*******************************************************************************
 *                              2D图形绘制
 ******************************************************************************/

/**
 * @brief  绘制单个像素点
 * @param  x 水平坐标 (0~799)
 * @param  y 垂直坐标 (0~479)
 * @param  color 颜色值（需与当前颜色格式对应）
 * @note   示例：RGB_LCD_DrawPoint(10, 10, 0x001F) 绘制RGB565蓝色点
 * @retval None
 */
void RGB_LCD_DrawPoint(uint16_t x, uint16_t y, uint32_t color);

/**
 * @brief  读取指定坐标点的颜色
 * @param  x 水平坐标 (0~799)
 * @param  y 垂直坐标 (0~479)
 * @note   示例：color = RGB_LCD_ReadPoint(10, 10)
 * @retval 读取到的颜色值
 */
uint32_t RGB_LCD_ReadPoint(uint16_t x, uint16_t y);

/**
 * @brief  两点间绘制直线（Bresenham算法）
 * @param  x1 起点水平坐标 (0~799)
 * @param  y1 起点垂直坐标 (0~479)
 * @param  x2 终点水平坐标 (0~799)
 * @param  y2 终点垂直坐标 (0~479)
 * @note   移植自ST官方例程
 * @retval None
 */
void RGB_LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

/**
 * @brief  绘制矩形框
 * @param  x 起始水平坐标 (0~799)
 * @param  y 起始垂直坐标 (0~479)
 * @param  width 矩形宽度
 * @param  height 矩形高度
 * @retval None
 */
void RGB_LCD_DrawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height);

/**
 * @brief  绘制圆形轮廓
 * @param  x 圆心水平坐标 (0~799)
 * @param  y 圆心垂直坐标 (0~479)
 * @param  r 半径
 * @note   移植自ST官方例程
 * @retval None
 */
void RGB_LCD_DrawCircle(uint16_t x, uint16_t y, uint16_t r);

/**
 * @brief  绘制椭圆轮廓
 * @param  x 中心水平坐标 (0~799)
 * @param  y 中心垂直坐标 (0~479)
 * @param  r1 水平半轴长度
 * @param  r2 垂直半轴长度
 * @note   移植自ST官方例程
 * @retval None
 */
void RGB_LCD_DrawEllipse(int x, int y, int r1, int r2);

/*******************************************************************************
 *                              区域填充
 ******************************************************************************/

/**
 * @brief  填充矩形区域
 * @param  x 起始水平坐标 (0~799)
 * @param  y 起始垂直坐标 (0~479)
 * @param  width 矩形宽度
 * @param  height 矩形高度
 * @note   使用当前画笔色填充，DMA2D加速
 * @retval None
 */
void RGB_LCD_FillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height);

/**
 * @brief  填充圆形区域
 * @param  x 圆心水平坐标 (0~799)
 * @param  y 圆心垂直坐标 (0~479)
 * @param  r 半径
 * @note   移植自ST官方例程
 * @retval None
 */
void RGB_LCD_FillCircle(uint16_t x, uint16_t y, uint16_t r);

/*******************************************************************************
 *                              图像显示
 ******************************************************************************/

/**
 * @brief  显示单色图像
 * @param  x 起始水平坐标 (0~799)
 * @param  y 起始垂直坐标 (0~479)
 * @param  width 图像宽度
 * @param  height 图像高度
 * @param  pImage 图像数据首地址（取模数据）
 * @note   需事先通过取模软件生成图像数据
 * @retval None
 */
void RGB_LCD_DrawImage(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                       const uint8_t *pImage);

/**
 * @brief  RGB LCD 综合功能测试
 * @param  None
 * @retval None
 * @note   演示所有主要功能，包括：
 *         1. 清屏和颜色设置
 *         2. 文字显示（ASCII、中文、混合）
 *         3. 数字显示
 *         4. 2D图形绘制（直线、矩形、圆形、椭圆）
 *         5. 区域填充
 */
void lcd_test(void);
#endif

#endif //__LCD_RGB_H
