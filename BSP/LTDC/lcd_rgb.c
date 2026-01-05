/**
 ******************************************************************************
 * @file    lcd_rgb.c
 * @author  菜菜why（B站：菜菜whyy）
 * @brief   RGB LCD 800*480分辨率屏幕驱动实现 (LTDC + DMA2D)
 ******************************************************************************
 * @attention
 * 说明：
 * 1. 使用外部SDRAM作为显存，起始地址0xC0000000
 * 2. LTDC时钟配置为32MHz，刷新率约60Hz
 * 3. 支持双层显示（Layer0背景层 + Layer1前景层）
 * 4. 中文字库为小字库，需提前取模
 * 5. 刚下载程序时屏幕可能有轻微抖动，等待片刻或重新上电即可恢复
 ******************************************************************************
 */

#include "lcd_rgb.h"

#ifdef LCD_RGB_ENABLE

extern DMA2D_HandleTypeDef hdma2d; // DMA2D句柄
extern LTDC_HandleTypeDef hltdc;   // LTDC句柄

static pFONT *LCD_RGB_Fonts;   // ASCII字体
static pFONT *LCD_RGB_CHFonts; // 中文字体

/**
 * @brief RGB LCD参数结构体
 */
struct {
  uint32_t Color;          // RGB LCD当前画笔颜色
  uint32_t BackColor;      // 背景色
  uint32_t ColorMode;      // 颜色格式
  uint32_t LayerMemoryAdd; // 层显存地址
  uint8_t Layer;           // 当前层
  uint8_t Direction;       // 显示方向
  uint8_t BytesPerPixel;   // 每个像素所占字节数
  uint8_t ShowNum_Mode;    // 数字显示模式
} LCD_RGB;

/****************************************************************************************************************************************
 * @name   RGB_LCD_GPIO_Init
 * @brief  初始化RGB LCD背光引脚
 * @param  None
 * @retval None
 ****************************************************************************************************************************************/
void RGB_LCD_GPIO_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  RGB_GPIO_LDC_Backlight_CLK_ENABLE;

  // 初始化背光引脚
  GPIO_InitStruct.Pin = RGB_LCD_Backlight_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(RGB_LCD_Backlight_PORT, &GPIO_InitStruct);

  RGB_LCD_Backlight_OFF;
}

/****************************************************************************************************************************************
 * @name   RGB_LCD_Init
 * @brief  初始化RGB LCD屏幕
 * @param  None
 * @retval None
 * @note   配置LTDC层参数、DMA2D、背光GPIO
 ****************************************************************************************************************************************/
void RGB_LCD_Init(void) {

  RGB_LCD_GPIO_Init(); // 初始化LTDC引脚的背光引脚

  /*---------------------------------- 初始化一些默认配置
   * --------------------------------*/

  RGB_LCD_DisplayDirection(Direction_H_RGB); // 设置横屏显示
  RGB_LCD_SetTextFont(24);                   // 设置默认中英文字体
  RGB_LCD_ShowNumMode(Fill_Space_RGB);       // 设置数字显示默认填充空格

#if RGB_LCD_NUM_LAYERS == 2 // 如果开启了双层显示
  RGB_LCD_SetLayer(1);                 // 切换到 layer1
  RGB_LCD_SetBackColor(0x00000000);    // 设置背景色
  RGB_LCD_SetColor(RGB_LCD_WHITE);     // 设置画笔颜色
  RGB_LCD_Clear();                     // 清屏，刷背景色
#endif

  RGB_LCD_SetLayer(0);                 // 切换到 layer0
  RGB_LCD_SetBackColor(RGB_LCD_BLACK); // 设置背景色
  RGB_LCD_SetColor(RGB_LCD_WHITE);     // 设置画笔颜色
  RGB_LCD_Clear();                     // 清屏，刷背景色

  // LTDC在初始化之后，上电的瞬间会有一个短暂的白屏，
  // 即使一开始就将背光引脚拉低并且屏幕背光引脚用电阻下拉还是会有这个现象，
  // 如果需要消除这个现象，可以在初始化完毕之后，进行一个短暂的延时再打开背光
  //
  HAL_Delay(200); // 延时200ms

  RGB_LCD_Backlight_ON; // 开启背光
}

/****************************************************************************************************************************************
 * @name   RGB_LCD_SetLayer
 * @brief  设置当前操作的图层
 * @param  layer - 图层编号 (0或1)
 * @retval None
 * @note
 *Layer1在Layer0之上，开启两层显示时Layer1是前景层（通常使用带透明色的格式），Layer0是背景层
 ****************************************************************************************************************************************/
void RGB_LCD_SetLayer(uint8_t layer) {
#if RGB_LCD_NUM_LAYERS == 2 // 如果开了双层

  if (layer == 0) // 如果设置的是 layer0
  {
    LCD_RGB.LayerMemoryAdd = RGB_LCD_MemoryAdd; // 获取 layer0 的显存地址
    LCD_RGB.ColorMode = RGB_ColorMode_0;        // 获取 layer0 的颜色格式
    LCD_RGB.BytesPerPixel =
        RGB_BytesPerPixel_0; // 获取 layer0 的每个像素所需字节数的大小
  } else if (layer == 1)     // 如果设置的是 layer1
  {
    LCD_RGB.LayerMemoryAdd =
        RGB_LCD_MemoryAdd + RGB_LCD_MemoryAdd_OFFSET; // 获取 layer1 的显存地址
    LCD_RGB.ColorMode = RGB_ColorMode_1;              // 获取 layer1 的颜色格式
    LCD_RGB.BytesPerPixel =
        RGB_BytesPerPixel_1; // 获取 layer1 的每个像素所需字节数的大小
  }
  LCD_RGB.Layer = layer; // 记录当前所在的层

#else // 如果只开启单层，默认操作 layer0

  LCD_RGB.LayerMemoryAdd = RGB_LCD_MemoryAdd; // 获取 layer0 的显存地址
  LCD_RGB.ColorMode = RGB_ColorMode_0;        // 获取 layer0 的颜色格式
  LCD_RGB.BytesPerPixel =
      RGB_BytesPerPixel_0; // 获取 layer0 的每个像素所需字节数的大小
  LCD_RGB.Layer = 0;       // 层标记设置为 layer0

#endif
}

/****************************************************************************************************************************************
 * @name   RGB_LCD_SetColor
 * @brief  设置RGB LCD画笔颜色
 * @param  Color - 32位ARGB8888颜色值
 * @retval None
 * @note   1.
 *为了方便用户使用自定义颜色，入口参数Color使用32位颜色格式，自动转换
 * @note   2. 32位颜色格式：0xAARRGGBB (A:透明度 R:红 G:绿 B:蓝)
 * @note   3. 透明通道：0xFF不透明，0x00完全透明
 * @note   4. 仅ARGB格式支持透明，RGB格式忽略A通道
 * @note   5.
 *ARGB1555仅支持1位透明（透明/不透明），ARGB4444支持16级，ARGB8888支持256级
 ****************************************************************************************************************************************/
void RGB_LCD_SetColor(uint32_t Color) {
  uint16_t Alpha_Value = 0, Red_Value = 0, Green_Value = 0,
           Blue_Value = 0; // 各个颜色通道的值

  if (LCD_RGB.ColorMode == LTDC_PIXEL_FORMAT_RGB565) // 将32位色转换为16位色
  {
    Red_Value = (uint16_t)((Color & 0x00F80000) >> 8);
    Green_Value = (uint16_t)((Color & 0x0000FC00) >> 5);
    Blue_Value = (uint16_t)((Color & 0x000000F8) >> 3);
    LCD_RGB.Color = (uint16_t)(Red_Value | Green_Value | Blue_Value);
  } else if (LCD_RGB.ColorMode ==
             LTDC_PIXEL_FORMAT_ARGB1555) // 将32位色转换为ARGB1555颜色
  {
    if ((Color & 0xFF000000) == 0) // 判断是否使用透明色
      Alpha_Value = 0x0000;
    else
      Alpha_Value = 0x8000;

    Red_Value = (uint16_t)((Color & 0x00F80000) >> 9);
    Green_Value = (uint16_t)((Color & 0x0000F800) >> 6);
    Blue_Value = (uint16_t)((Color & 0x000000F8) >> 3);
    LCD_RGB.Color =
        (uint16_t)(Alpha_Value | Red_Value | Green_Value | Blue_Value);
  } else if (LCD_RGB.ColorMode ==
             LTDC_PIXEL_FORMAT_ARGB4444) // 将32位色转换为ARGB4444颜色
  {
    Alpha_Value = (uint16_t)((Color & 0xf0000000) >> 16);
    Red_Value = (uint16_t)((Color & 0x00F00000) >> 12);
    Green_Value = (uint16_t)((Color & 0x0000F000) >> 8);
    Blue_Value = (uint16_t)((Color & 0x000000F8) >> 4);
    LCD_RGB.Color =
        (uint16_t)(Alpha_Value | Red_Value | Green_Value | Blue_Value);
  } else
    LCD_RGB.Color = Color; // 24位色或32位色不需要转换
}

/****************************************************************************************************************************************
 * @name   RGB_LCD_SetBackColor
 * @brief  设置RGB LCD背景颜色
 * @param  Color - 32位ARGB8888颜色值
 * @retval None
 * @note   用于清屏以及显示字符的背景色
 ****************************************************************************************************************************************/
void RGB_LCD_SetBackColor(uint32_t Color) {
  uint16_t Alpha_Value = 0, Red_Value = 0, Green_Value = 0,
           Blue_Value = 0; // 各个颜色通道的值

  if (LCD_RGB.ColorMode == LTDC_PIXEL_FORMAT_RGB565) // 将32位色转换为16位色
  {
    Red_Value = (uint16_t)((Color & 0x00F80000) >> 8);
    Green_Value = (uint16_t)((Color & 0x0000FC00) >> 5);
    Blue_Value = (uint16_t)((Color & 0x000000F8) >> 3);
    LCD_RGB.BackColor = (uint16_t)(Red_Value | Green_Value | Blue_Value);
  } else if (LCD_RGB.ColorMode ==
             LTDC_PIXEL_FORMAT_ARGB1555) // 将32位色转换为ARGB1555颜色
  {
    if ((Color & 0xFF000000) == 0) // 判断是否使用透明色
      Alpha_Value = 0x0000;
    else
      Alpha_Value = 0x8000;

    Red_Value = (uint16_t)((Color & 0x00F80000) >> 9);
    Green_Value = (uint16_t)((Color & 0x0000F800) >> 6);
    Blue_Value = (uint16_t)((Color & 0x000000F8) >> 3);
    LCD_RGB.BackColor =
        (uint16_t)(Alpha_Value | Red_Value | Green_Value | Blue_Value);
  } else if (LCD_RGB.ColorMode ==
             LTDC_PIXEL_FORMAT_ARGB4444) // 将32位色转换为ARGB4444颜色
  {
    Alpha_Value = (uint16_t)((Color & 0xf0000000) >> 16);
    Red_Value = (uint16_t)((Color & 0x00F00000) >> 12);
    Green_Value = (uint16_t)((Color & 0x0000F000) >> 8);
    Blue_Value = (uint16_t)((Color & 0x000000F8) >> 4);
    LCD_RGB.BackColor =
        (uint16_t)(Alpha_Value | Red_Value | Green_Value | Blue_Value);
  } else
    LCD_RGB.BackColor = Color; // 24位色或32位色不需要转换
}

/****************************************************************************************************************************************
 * @name   RGB_LCD_DisplayDirection
 * @brief  设置RGB LCD显示方向
 * @param  direction - 显示方向 (Direction_H_RGB横屏/Direction_V_RGB竖屏)
 * @retval None
 * @note   示例：RGB_LCD_DisplayDirection(Direction_H_RGB) 设置横屏显示
 ****************************************************************************************************************************************/
void RGB_LCD_DisplayDirection(uint8_t direction) {
  LCD_RGB.Direction = direction;
}

/****************************************************************************************************************************************
 * @name   RGB_LCD_Clear
 * @brief  RGB LCD清屏函数
 * @param  None
 * @retval None
 * @note   将整个屏幕清除为当前背景色，使用DMA2D加速
 * @note   等待垂直数据使能状态，避免撕裂效应
 ****************************************************************************************************************************************/
void RGB_LCD_Clear(void) {
  DMA2D->CR &= ~(DMA2D_CR_START);                        // 停止DMA2D
  DMA2D->CR = DMA2D_R2M;                                 // 寄存器到SDRAM
  DMA2D->OPFCCR = LCD_RGB.ColorMode;                     // 设置颜色格式
  DMA2D->OOR = 0;                                        // 设置行偏移
  DMA2D->OMAR = LCD_RGB.LayerMemoryAdd;                  // 地址
  DMA2D->NLR = (RGB_LCD_Width << 16) | (RGB_LCD_Height); // 设定长度和宽度
  DMA2D->OCOLR = LCD_RGB.BackColor;                      // 颜色

  /******
  等待垂直数据使能显示状态，即LTDC即将刷完一整屏数据的时候
  因为在屏幕没有刷完一帧时进行刷屏，会有撕裂的现象
  用户也可以使用寄存器重载中断进行判断，不过为了保证例程的简洁以及移植的方便性，这里直接使用判断寄存器的方法

  如果不做判断，DMA2D刷屏速度如下：
  颜色格式	RGB565	 RGB888	 ARGB888
  耗时	   4.3ms	 7.5ms	 11.9ms

  加了之后，不管哪种格式，都需要17.6ms刷一屏，不过屏幕本身的刷新率只有60帧左右（LTDC时钟33MHz），
  17.6ms的速度已经足够了，除非是对速度要求特别高的场合，不然建议加上判断垂直等待的语句，可以避免撕裂效应
  ******/

  while (LTDC->CDSR != 0X00000001)
    ; // 判断显示状态寄存器LTDC_CDSR的第0位 VDES：垂直数据使能显示状态

  DMA2D->CR |= DMA2D_CR_START; // 启动DMA2D
  while (DMA2D->CR & DMA2D_CR_START)
    ; // 等待传输完成
}

/****************************************************************************************************************************************
 * @name   RGB_LCD_ClearRect
 * @brief  RGB LCD局部清屏函数
 * @param  x - 起始水平坐标 (0~799)
 * @param  y - 起始垂直坐标 (0~479)
 * @param  width - 清除区域宽度
 * @param  height - 清除区域高度
 * @retval None
 * @note   将指定区域清除为当前背景色，使用DMA2D加速
 ****************************************************************************************************************************************/
void RGB_LCD_ClearRect(uint16_t x, uint16_t y, uint16_t width,
                       uint16_t height) {
  DMA2D->CR &= ~(DMA2D_CR_START);    // 停止DMA2D
  DMA2D->CR = DMA2D_R2M;             // 寄存器到SDRAM
  DMA2D->OPFCCR = LCD_RGB.ColorMode; // 设置颜色格式
  DMA2D->OCOLR = LCD_RGB.BackColor;  // 颜色

  if (LCD_RGB.Direction == Direction_H_RGB) // 横屏填充
  {
    DMA2D->OOR = RGB_LCD_Width - width; // 设置行偏移
    DMA2D->OMAR = LCD_RGB.LayerMemoryAdd +
                  LCD_RGB.BytesPerPixel * (RGB_LCD_Width * y + x); // 地址
    DMA2D->NLR = (width << 16) | (height); // 设定长度和宽度
  } else                                   // 竖屏填充
  {
    DMA2D->OOR = RGB_LCD_Width - height; // 设置行偏移
    DMA2D->OMAR =
        LCD_RGB.LayerMemoryAdd +
        LCD_RGB.BytesPerPixel *
            ((RGB_LCD_Height - x - width) * RGB_LCD_Width + y); // 地址
    DMA2D->NLR = (width) | (height << 16); // 设定长度和宽度
  }

  DMA2D->CR |= DMA2D_CR_START; // 启动DMA2D
  while (DMA2D->CR & DMA2D_CR_START)
    ; // 等待传输完成
}

/****************************************************************************************************************************************
 * @name   RGB_LCD_DrawPoint
 * @brief  RGB LCD绘制单个像素点
 * @param  x - 水平坐标 (0~799)
 * @param  y - 垂直坐标 (0~479)
 * @param  color - 颜色值（需与当前颜色格式对应）
 * @retval None
 * @note   直接在显存对应位置写入颜色值
 ****************************************************************************************************************************************/
void RGB_LCD_DrawPoint(uint16_t x, uint16_t y, uint32_t color) {
  /*----------------------- 32位色 ARGB8888 模式 ----------------------*/

  if (LCD_RGB.ColorMode == LTDC_PIXEL_FORMAT_ARGB8888) {
    if (LCD_RGB.Direction == Direction_H_RGB) // 水平方向
    {
      *(__IO uint32_t *)(LCD_RGB.LayerMemoryAdd + 4 * (x + y * RGB_LCD_Width)) =
          color;
    } else if (LCD_RGB.Direction == Direction_V_RGB) // 垂直方向
    {
      *(__IO uint32_t *)(LCD_RGB.LayerMemoryAdd +
                         4 * ((RGB_LCD_Height - x - 1) * RGB_LCD_Width + y)) =
          color;
    }
  }
  /*----------------------------- 24位色 RGB888 模式 -------------------------*/

  else if (LCD_RGB.ColorMode == LTDC_PIXEL_FORMAT_RGB888) {
    if (LCD_RGB.Direction == Direction_H_RGB) // 水平方向
    {
      *(__IO uint16_t *)(LCD_RGB.LayerMemoryAdd + 3 * (x + y * RGB_LCD_Width)) =
          color;
      *(__IO uint8_t *)(LCD_RGB.LayerMemoryAdd + 3 * (x + y * RGB_LCD_Width) +
                        2) = color >> 16;
    } else if (LCD_RGB.Direction == Direction_V_RGB) // 垂直方向
    {
      *(__IO uint16_t *)(LCD_RGB.LayerMemoryAdd +
                         3 * ((RGB_LCD_Height - x - 1) * RGB_LCD_Width + y)) =
          color;
      *(__IO uint8_t *)(LCD_RGB.LayerMemoryAdd +
                        3 * ((RGB_LCD_Height - x - 1) * RGB_LCD_Width + y) +
                        2) = color >> 16;
    }
  }

  /*----------------------- 16位色 ARGB1555、RGB565或者ARGB4444 模式
     ----------------------*/
  else {
    if (LCD_RGB.Direction == Direction_H_RGB) // 水平方向
    {
      *(__IO uint16_t *)(LCD_RGB.LayerMemoryAdd + 2 * (x + y * RGB_LCD_Width)) =
          color;
    } else if (LCD_RGB.Direction == Direction_V_RGB) // 垂直方向
    {
      *(__IO uint16_t *)(LCD_RGB.LayerMemoryAdd +
                         2 * ((RGB_LCD_Height - x - 1) * RGB_LCD_Width + y)) =
          color;
    }
  }
}

/****************************************************************************************************************************************
 * @name   RGB_LCD_ReadPoint
 * @brief  读取RGB LCD指定坐标点的颜色
 * @param  x - 水平坐标 (0~799)
 * @param  y - 垂直坐标 (0~479)
 * @retval 读取到的颜色值
 * @note   直接读取显存对应位置的值
 ****************************************************************************************************************************************/
uint32_t RGB_LCD_ReadPoint(uint16_t x, uint16_t y) {
  uint32_t color = 0;

  /*----------------------- 32位色 ARGB8888 模式 ----------------------*/
  if (LCD_RGB.ColorMode == LTDC_PIXEL_FORMAT_ARGB8888) {
    if (LCD_RGB.Direction == Direction_H_RGB) // 水平方向
    {
      color = *(__IO uint32_t *)(LCD_RGB.LayerMemoryAdd +
                                 4 * (x + y * RGB_LCD_Width));
    } else if (LCD_RGB.Direction == Direction_V_RGB) // 垂直方向
    {
      color = *(
          __IO uint32_t *)(LCD_RGB.LayerMemoryAdd +
                           4 * ((RGB_LCD_Height - x - 1) * RGB_LCD_Width + y));
    }
  }

  /*----------------------------- 24位色 RGB888 模式 -------------------------*/
  else if (LCD_RGB.ColorMode == LTDC_PIXEL_FORMAT_RGB888) {
    if (LCD_RGB.Direction == Direction_H_RGB) // 水平方向
    {
      color = *(__IO uint32_t *)(LCD_RGB.LayerMemoryAdd +
                                 3 * (x + y * RGB_LCD_Width)) &
              0x00ffffff;
    } else if (LCD_RGB.Direction == Direction_V_RGB) // 垂直方向
    {
      color = *(__IO uint32_t *)(LCD_RGB.LayerMemoryAdd +
                                 3 * ((RGB_LCD_Height - x - 1) * RGB_LCD_Width +
                                      y)) &
              0x00ffffff;
    }
  }

  /*----------------------- 16位色 ARGB1555、RGB565或者ARGB4444 模式
     ----------------------*/
  else {
    if (LCD_RGB.Direction == Direction_H_RGB) // 水平方向
    {
      color = *(__IO uint16_t *)(LCD_RGB.LayerMemoryAdd +
                                 2 * (x + y * RGB_LCD_Width));
    } else if (LCD_RGB.Direction == Direction_V_RGB) // 垂直方向
    {
      color = *(
          __IO uint16_t *)(LCD_RGB.LayerMemoryAdd +
                           2 * ((RGB_LCD_Height - x - 1) * RGB_LCD_Width + y));
    }
  }
  return color;
}



/****************************************************************************************************************************************
 * @name   RGB_LCD_DisplayString
 * @brief  RGB LCD显示ASCII字符串
 * @param  x - 起始水平坐标 (0~799)
 * @param  y - 起始垂直坐标 (0~479)
 * @param  p - 字符串首地址
 * @retval None
 * @note   循环调用RGB_LCD_DisplayChar()显示每个字符
 ****************************************************************************************************************************************/
void RGB_LCD_DisplayString(uint16_t x, uint16_t y, char *p) {
  while ((x < RGB_LCD_Width) &&
         (*p != 0)) // 判断显示坐标是否超出显示区域并且字符是否为空字符
  {
    RGB_LCD_DisplayChar(x, y, *p);
    x += LCD_RGB_Fonts->Width; // 显示下一个字符
    p++;                       // 取下一个字符地址
  }
}

/****************************************************************************************************************************************
 * @name   RGB_LCD_SetTextFont
 * @brief  设置RGB LCD中英文混合字体
 * @param  font_size - 字体大小 (12/16/20/24/32)
 * @retval None
 * @note   自动匹配对应的ASCII和中文字体
 ****************************************************************************************************************************************/
void RGB_LCD_SetTextFont(uint8_t font_size) {
  switch (font_size) {
  case 12:
    LCD_RGB_Fonts = &ASCII_Font12;
    LCD_RGB_CHFonts = &CH_Font12;
    break;
  case 16:
    LCD_RGB_Fonts = &ASCII_Font16;
    LCD_RGB_CHFonts = &CH_Font16;
    break;
  case 20:
    LCD_RGB_Fonts = &ASCII_Font20;
    LCD_RGB_CHFonts = &CH_Font20;
    break;
  case 24:
    LCD_RGB_Fonts = &ASCII_Font24;
    LCD_RGB_CHFonts = &CH_Font24;
    break;
  case 32:
    LCD_RGB_Fonts = &ASCII_Font32;
    LCD_RGB_CHFonts = &CH_Font32;
    break;
  default:
    LCD_RGB_Fonts = &ASCII_Font12;
    LCD_RGB_CHFonts = &CH_Font12;
    break;
  }
}
/**
 * @brief  获取当前RGB LCD中文字体大小
 * @note   内部函数,用于Flash字库模式
 */
uint8_t RGB_LCD_GetChineseFontSize(void) {
  if (LCD_RGB_CHFonts != NULL) {
    return LCD_RGB_CHFonts->Width;
  }
  return 12; // 默认12号字体
}

#ifdef USE_FLASH_FONT_RGB
/**
 * @brief  绘制字模到RGB LCD(支持12/16/20/24/32)
 * @note   内部函数,用于Flash字库模式
 */
static void RGB_DrawFont_Bitmap(uint16_t x, uint16_t y, uint16_t width,
                                uint16_t height, const uint8_t *pData) {
  uint16_t i = 0;
  // 计算每行占用的字节数。例如宽度12，(12+7)/8 = 2字节
  uint16_t bytes_per_row = (width + 7) / 8;
  uint16_t total_bytes = bytes_per_row * height;

  // 逐字节解析字模数据
  for (uint16_t byte_idx = 0; byte_idx < total_bytes; byte_idx++) {
    uint8_t byte_data = pData[byte_idx];

    // 每个字节代表8个像素点
    for (uint8_t bit = 0; bit < 8; bit++) {
      uint16_t px = x + (i % width);
      uint16_t py = y + (i / width);

      if (byte_data & (0x01 << bit)) {
        RGB_LCD_DrawPoint(px, py, LCD_RGB.Color); // 前景色
      } else {
        RGB_LCD_DrawPoint(px, py, LCD_RGB.BackColor); // 背景色
      }
      i++;

      // 如果当前像素点计数能够整除宽度，说明这一行画完了
      if (i % width == 0) {
        break; // 跳出内层循环，进入下一行
      }
    }
  }
}
#endif

/****************************************************************************************************************************************
 * @name   RGB_LCD_DisplayChinese
 * @brief  RGB LCD显示单个汉字
 * @param  x - 起始水平坐标 (0~799)
 * @param  y - 起始垂直坐标 (0~479)
 * @param  pText - 汉字字符串（单个汉字）
 * @retval None
 * @note   支持Flash字库和内置字库两种方式
 ****************************************************************************************************************************************/
void RGB_LCD_DisplayChinese(uint16_t x, uint16_t y, char *pText) {
#ifdef USE_FLASH_FONT_RGB
  uint8_t font_size = RGB_LCD_GetChineseFontSize();

#ifdef IS_GB2312_RGB
  // 使用查找表获得的索引计算最终地址
  const uint8_t *pFontData = GB2312_FindFont_Flash(pText, font_size);
#else
  const uint8_t *pFontData =
      UTF8_FindFont_Flash((const uint8_t *)pText, font_size);
#endif

  // 渲染到屏幕
  RGB_DrawFont_Bitmap(x, y, font_size, font_size, pFontData);
#else
  uint16_t  index = 0, counter = 0; // 计数变量
  uint16_t addr = 0;                      // 字模地址
  uint8_t disChar;                        // 字模的值
  uint16_t Xaddress = 0;                  // 水平坐标

  while (1) {
    // 对比数组中的汉字编码，用以定位该汉字字模的地址
    if ((LCD_RGB_CHFonts->pTable[addr] == *pText) &&
        (LCD_RGB_CHFonts->pTable[addr + 1] == *(pText + 1))) {
      addr = addr + 2; // 偏移到汉字字模的起始地址
      break;
    } else {
      addr += (LCD_RGB_CHFonts->Sizes + 2); // 指向下一个汉字字模的地址
    }
  }

  for (index = 0; index < LCD_RGB_CHFonts->Sizes; index++) {
    disChar = LCD_RGB_CHFonts->pTable[addr]; // 获取字模的值
    addr += 1;
    Xaddress = x; // 水平坐标复位

    for (counter = 0; counter < 8; counter++) {
      if (disChar & 0x01) {
        RGB_LCD_DrawPoint(Xaddress, y,
                          LCD_RGB.Color); // 当前模值不为0时，使用画笔色绘点
      } else {
        RGB_LCD_DrawPoint(Xaddress, y,
                          LCD_RGB.BackColor); // 当前模值为0时，使用背景色绘制点
      }
      disChar >>= 1;
      Xaddress++; // 水平坐标自加

      if ((Xaddress - x) ==
          LCD_RGB_CHFonts->Width) // 如果水平坐标达到了字符宽度，则退出当前循环
      {                           // 进入下一行的绘制
        Xaddress = x;
        y++;
        break;
      }
    }
  }
#endif
}
/****************************************************************************************************************************************
 * @name   RGB_LCD_DisplayChar
 * @brief  RGB LCD显示单个ASCII字符
 * @param  x - 起始水平坐标 (0~799)
 * @param  y - 起始垂直坐标 (0~479)
 * @param  c - ASCII字符
 * @retval None
 * @note   根据字模数据逐点绘制字符
 ****************************************************************************************************************************************/
void RGB_LCD_DisplayChar(uint16_t x, uint16_t y, uint8_t c) {
#ifdef USE_FLASH_FONT_RGB
  uint8_t font_size = RGB_LCD_GetChineseFontSize();
  uint8_t ascii_width = font_size / 2; // ASCII字符宽度通常是高度的一半

  const uint8_t *pFontData = ASCII_FindFont_Flash(c, font_size);

  if (pFontData != NULL) {
    RGB_DrawFont_Bitmap(x, y, ascii_width, font_size, pFontData);
    return;
  }
#else
  uint16_t index = 0, counter = 0; // 计数变量
  uint8_t disChar;                 // 存储字符的地址
  uint16_t Xaddress = x;           // 水平坐标

  c = c - 32; // 计算ASCII字符的偏移

  for (index = 0; index < LCD_RGB_Fonts->Sizes; index++) {
    disChar = LCD_RGB_Fonts
                  ->pTable[c * LCD_RGB_Fonts->Sizes + index]; // 获取字符的模值
    for (counter = 0; counter < 8; counter++) {
      if (disChar & 0x01) {
        RGB_LCD_DrawPoint(Xaddress, y,
                          LCD_RGB.Color); // 当前模值不为0时，使用画笔色绘点
      } else {
        RGB_LCD_DrawPoint(Xaddress, y,
                          LCD_RGB.BackColor); // 否则使用背景色绘制点
      }
      disChar >>= 1;
      Xaddress++; // 水平坐标自加

      if ((Xaddress - x) ==
          LCD_RGB_Fonts->Width) // 如果水平坐标达到了字符宽度，则退出当前循环
      {                         // 进入下一行的绘制
        Xaddress = x;
        y++;
        break;
      }
    }
  }
  #endif
}
/****************************************************************************************************************************************
 * @name   RGB_LCD_DisplayText
 * @brief  RGB LCD显示中英文混合字符串
 * @param  x - 起始水平坐标 (0~799)
 * @param  y - 起始垂直坐标 (0~479)
 * @param  pText - 字符串首地址
 * @retval None
 * @note   自动识别中英文字符，支持换行
 ****************************************************************************************************************************************/
void RGB_LCD_DisplayText(uint16_t x, uint16_t y, char *pText) {
#ifdef USE_FLASH_FONT_RGB
  uint8_t font_size = RGB_LCD_GetChineseFontSize();
#endif
  uint16_t x_start = x; // 记录起始X坐标,用于换行

  while (*pText != 0) {
    if (*pText <= 0x7F) // ASCII字符
    {
      // 检查是否需要换行
      if (x + LCD_RGB_Fonts->Width > RGB_LCD_Width) {
        x = x_start;
#ifdef USE_FLASH_FONT_RGB
        y += font_size;
#else
        y += LCD_RGB_CHFonts->Height;
#endif
      }

      RGB_LCD_DisplayChar(x, y, *pText);
      x += LCD_RGB_Fonts->Width;
      pText++;
    } else // 汉字(GBK编码,双字节)
    {
#ifdef USE_FLASH_FONT_RGB
      // 检查是否需要换行
      if (x + font_size > RGB_LCD_Width) {
        x = x_start;
        y += font_size;
      }
#else
      // 检查是否需要换行
      if (x + LCD_RGB_CHFonts->Width > RGB_LCD_Width) {
        x = x_start;
        y += LCD_RGB_CHFonts->Height;
      }
#endif

      RGB_LCD_DisplayChinese(x, y, pText);

#ifdef USE_FLASH_FONT_RGB
      x += font_size; // Flash字库使用动态字体大小
#else
      x += LCD_RGB_CHFonts->Width; // 内置字库使用固定宽度
#endif
#if defined(USE_FLASH_FONT_RGB) && !defined(IS_GB2312_RGB)
      uint8_t utf8_code = (uint8_t)*pText;
      if ((utf8_code & 0xE0) == 0xC0)
        pText += 2; // 2字节 UTF-8
      else if ((utf8_code & 0xF0) == 0xE0)
        pText += 3; // 3字节 UTF-8 (常用汉字)
      else if ((utf8_code & 0xF8) == 0xF0)
        pText += 4;
#else
      pText += 2; // GBK双字节
#endif
    }
  }
}

/****************************************************************************************************************************************
 * @name   RGB_LCD_ShowNumMode
 * @brief  设置RGB LCD数字显示模式
 * @param  mode - 填充模式 (Fill_Zero_RGB/Fill_Space_RGB)
 * @retval None
 * @note   仅用于RGB_LCD_DisplayNumber()和RGB_LCD_DisplayDecimals()
 ****************************************************************************************************************************************/
void RGB_LCD_ShowNumMode(uint8_t mode) { LCD_RGB.ShowNum_Mode = mode; }

/****************************************************************************************************************************************
 * @name   RGB_LCD_DisplayNumber
 * @brief  RGB LCD显示整数
 * @param  x - 起始水平坐标 (0~799)
 * @param  y - 起始垂直坐标 (0~479)
 * @param  number - 整数值 (-2147483648~2147483647)
 * @param  len - 显示位数（含符号位）
 * @retval None
 * @note   根据RGB_LCD_ShowNumMode()的设置，多余位填充0或空格
 ****************************************************************************************************************************************/
void RGB_LCD_DisplayNumber(uint16_t x, uint16_t y, int32_t number,
                           uint8_t len) {
  char Number_Buffer[15]; // 用于存储转换后的字符串

  if (LCD_RGB.ShowNum_Mode == Fill_Zero_RGB) // 多余位补0
  {
    sprintf(Number_Buffer, "%0.*d", len,
            number); // 将 number 转换成字符串，便于显示
  } else             // 多余位补空格
  {
    sprintf(Number_Buffer, "%*d", len,
            number); // 将 number 转换成字符串，便于显示
  }

  RGB_LCD_DisplayString(x, y,
                        (char *)Number_Buffer); // 将转换得到的字符串显示出来
}

/****************************************************************************************************************************************
 * @name   RGB_LCD_DisplayDecimals
 * @brief  RGB LCD显示小数
 * @param  x - 起始水平坐标 (0~799)
 * @param  y - 起始垂直坐标 (0~479)
 * @param  decimals - 浮点数值
 * @param  len - 总位数（含小数点和符号）
 * @param  decs - 小数位数
 * @retval None
 * @note   根据RGB_LCD_ShowNumMode()的设置，多余位填充0或空格
 * @note   小数部分超过指定位数时四舍五入
 ****************************************************************************************************************************************/
void RGB_LCD_DisplayDecimals(uint16_t x, uint16_t y, double decimals,
                             uint8_t len, uint8_t decs) {
  char Number_Buffer[20]; // 用于存储转换后的字符串

  if (LCD_RGB.ShowNum_Mode == Fill_Zero_RGB) // 多余位填充0模式
  {
    sprintf(Number_Buffer, "%0*.*lf", len, decs,
            decimals); // 将 number 转换成字符串，便于显示
  } else               // 多余位填充空格
  {
    sprintf(Number_Buffer, "%*.*lf", len, decs,
            decimals); // 将 number 转换成字符串，便于显示
  }

  RGB_LCD_DisplayString(x, y,
                        (char *)Number_Buffer); // 将转换得到的字符串显示出来
}

/****************************************************************************************************************************************
 * @name   RGB_LCD_DrawImage
 * @brief  RGB LCD显示单色图像
 * @param  x - 起始水平坐标 (0~799)
 * @param  y - 起始垂直坐标 (0~479)
 * @param  width - 图像宽度
 * @param  height - 图像高度
 * @param  pImage - 图像数据首地址（取模数据）
 * @retval None
 * @note   需事先通过取模软件生成图像数据
 ****************************************************************************************************************************************/
void RGB_LCD_DrawImage(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                       const uint8_t *pImage) {
  uint8_t disChar;       // 字模的值
  uint16_t Xaddress = x; // 水平坐标
  uint16_t i = 0, j = 0, m = 0;

  for (i = 0; i < height; i++) {
    for (j = 0; j < (float)width / 8; j++) {
      disChar = *pImage;

      for (m = 0; m < 8; m++) {
        if (disChar & 0x01) {
          RGB_LCD_DrawPoint(Xaddress, y,
                            LCD_RGB.Color); // 当前模值不为0时，使用画笔色绘点
        } else {
          RGB_LCD_DrawPoint(Xaddress, y,
                            LCD_RGB.BackColor); // 否则使用背景色绘制点
        }
        disChar >>= 1;
        Xaddress++; // 水平坐标自加

        if ((Xaddress - x) ==
            width) // 如果水平坐标达到了字符宽度，则退出当前循环
        {          // 进入下一行的绘制
          Xaddress = x;
          y++;
          break;
        }
      }
      pImage++;
    }
  }
}

/****************************************************************************************************************************************
 * @name   RGB_LCD_DrawLine
 * @brief  RGB LCD两点间绘制直线（Bresenham算法）
 * @param  x1 - 起点水平坐标 (0~799)
 * @param  y1 - 起点垂直坐标 (0~479)
 * @param  x2 - 终点水平坐标 (0~799)
 * @param  y2 - 终点垂直坐标 (0~479)
 * @retval None
 * @note   移植自ST官方评估板例程
 ****************************************************************************************************************************************/

#define RGB_ABS(X) ((X) > 0 ? (X) : -(X))

void RGB_LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
  int16_t deltax = 0, deltay = 0, x = 0, y = 0, xinc1 = 0, xinc2 = 0, yinc1 = 0,
          yinc2 = 0, den = 0, num = 0, numadd = 0, numpixels = 0, curpixel = 0;

  deltax = RGB_ABS(x2 - x1); /* The difference between the x's */
  deltay = RGB_ABS(y2 - y1); /* The difference between the y's */
  x = x1;                    /* Start x off at the first pixel */
  y = y1;                    /* Start y off at the first pixel */

  if (x2 >= x1) /* The x-values are increasing */
  {
    xinc1 = 1;
    xinc2 = 1;
  } else /* The x-values are decreasing */
  {
    xinc1 = -1;
    xinc2 = -1;
  }

  if (y2 >= y1) /* The y-values are increasing */
  {
    yinc1 = 1;
    yinc2 = 1;
  } else /* The y-values are decreasing */
  {
    yinc1 = -1;
    yinc2 = -1;
  }

  if (deltax >= deltay) /* There is at least one x-value for every y-value */
  {
    xinc1 = 0; /* Don't change the x when numerator >= denominator */
    yinc2 = 0; /* Don't change the y for every iteration */
    den = deltax;
    num = deltax / 2;
    numadd = deltay;
    numpixels = deltax; /* There are more x-values than y-values */
  } else                /* There is at least one y-value for every x-value */
  {
    xinc2 = 0; /* Don't change the x for every iteration */
    yinc1 = 0; /* Don't change the y when numerator >= denominator */
    den = deltay;
    num = deltay / 2;
    numadd = deltax;
    numpixels = deltay; /* There are more y-values than x-values */
  }
  for (curpixel = 0; curpixel <= numpixels; curpixel++) {
    RGB_LCD_DrawPoint(x, y, LCD_RGB.Color); /* Draw the current pixel */
    num += numadd;  /* Increase the numerator by the top of the fraction */
    if (num >= den) /* Check if numerator >= denominator */
    {
      num -= den; /* Calculate the new numerator value */
      x += xinc1; /* Change the x as appropriate */
      y += yinc1; /* Change the y as appropriate */
    }
    x += xinc2; /* Change the x as appropriate */
    y += yinc2; /* Change the y as appropriate */
  }
}

/****************************************************************************************************************************************
 * @name   RGB_LCD_DrawRect
 * @brief  RGB LCD绘制矩形框
 * @param  x - 起始水平坐标 (0~799)
 * @param  y - 起始垂直坐标 (0~479)
 * @param  width - 矩形宽度
 * @param  height - 矩形高度
 * @retval None
 * @note   移植自ST官方评估板例程
 ****************************************************************************************************************************************/
void RGB_LCD_DrawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
  /* draw horizontal lines */
  RGB_LCD_DrawLine(x, y, x + width, y);
  RGB_LCD_DrawLine(x, y + height, x + width, y + height);

  /* draw vertical lines */
  RGB_LCD_DrawLine(x, y, x, y + height);
  RGB_LCD_DrawLine(x + width, y, x + width, y + height);
}

/****************************************************************************************************************************************
 * @name   RGB_LCD_DrawCircle
 * @brief  RGB LCD绘制圆形轮廓
 * @param  x - 圆心水平坐标 (0~799)
 * @param  y - 圆心垂直坐标 (0~479)
 * @param  r - 半径
 * @retval None
 * @note   移植自ST官方评估板例程
 ****************************************************************************************************************************************/
void RGB_LCD_DrawCircle(uint16_t x, uint16_t y, uint16_t r) {
  int Xadd = -r, Yadd = 0, err = 2 - 2 * r, e2;
  do {
    RGB_LCD_DrawPoint(x - Xadd, y + Yadd, LCD_RGB.Color);
    RGB_LCD_DrawPoint(x + Xadd, y + Yadd, LCD_RGB.Color);
    RGB_LCD_DrawPoint(x + Xadd, y - Yadd, LCD_RGB.Color);
    RGB_LCD_DrawPoint(x - Xadd, y - Yadd, LCD_RGB.Color);

    e2 = err;
    if (e2 <= Yadd) {
      err += ++Yadd * 2 + 1;
      if (-Xadd == Yadd && e2 <= Xadd)
        e2 = 0;
    }
    if (e2 > Xadd)
      err += ++Xadd * 2 + 1;
  } while (Xadd <= 0);
}

/****************************************************************************************************************************************
 * @name   RGB_LCD_DrawEllipse
 * @brief  RGB LCD绘制椭圆轮廓
 * @param  x - 中心水平坐标 (0~799)
 * @param  y - 中心垂直坐标 (0~479)
 * @param  r1 - 水平半轴长度
 * @param  r2 - 垂直半轴长度
 * @retval None
 * @note   移植自ST官方评估板例程
 ****************************************************************************************************************************************/
void RGB_LCD_DrawEllipse(int x, int y, int r1, int r2) {
  int Xadd = -r1, Yadd = 0, err = 2 - 2 * r1, e2;
  float K = 0, rad1 = 0, rad2 = 0;

  rad1 = r1;
  rad2 = r2;

  if (r1 > r2) {
    do {
      K = (float)(rad1 / rad2);

      RGB_LCD_DrawPoint(x - Xadd, y + (uint16_t)(Yadd / K), LCD_RGB.Color);
      RGB_LCD_DrawPoint(x + Xadd, y + (uint16_t)(Yadd / K), LCD_RGB.Color);
      RGB_LCD_DrawPoint(x + Xadd, y - (uint16_t)(Yadd / K), LCD_RGB.Color);
      RGB_LCD_DrawPoint(x - Xadd, y - (uint16_t)(Yadd / K), LCD_RGB.Color);

      e2 = err;
      if (e2 <= Yadd) {
        err += ++Yadd * 2 + 1;
        if (-Xadd == Yadd && e2 <= Xadd)
          e2 = 0;
      }
      if (e2 > Xadd)
        err += ++Xadd * 2 + 1;
    } while (Xadd <= 0);
  } else {
    Yadd = -r2;
    Xadd = 0;
    do {
      K = (float)(rad2 / rad1);

      RGB_LCD_DrawPoint(x - (uint16_t)(Xadd / K), y + Yadd, LCD_RGB.Color);
      RGB_LCD_DrawPoint(x + (uint16_t)(Xadd / K), y + Yadd, LCD_RGB.Color);
      RGB_LCD_DrawPoint(x + (uint16_t)(Xadd / K), y - Yadd, LCD_RGB.Color);
      RGB_LCD_DrawPoint(x - (uint16_t)(Xadd / K), y - Yadd, LCD_RGB.Color);

      e2 = err;
      if (e2 <= Xadd) {
        err += ++Xadd * 3 + 1;
        if (-Yadd == Xadd && e2 <= Yadd)
          e2 = 0;
      }
      if (e2 > Yadd)
        err += ++Yadd * 3 + 1;
    } while (Yadd <= 0);
  }
}

/****************************************************************************************************************************************
 * @name   RGB_LCD_FillRect
 * @brief  RGB LCD填充矩形区域
 * @param  x - 起始水平坐标 (0~799)
 * @param  y - 起始垂直坐标 (0~479)
 * @param  width - 矩形宽度
 * @param  height - 矩形高度
 * @retval None
 * @note   使用当前画笔色填充，DMA2D加速
 ****************************************************************************************************************************************/
void RGB_LCD_FillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
  DMA2D->CR &= ~(DMA2D_CR_START);    // 停止DMA2D
  DMA2D->CR = DMA2D_R2M;             // 寄存器到SDRAM
  DMA2D->OPFCCR = LCD_RGB.ColorMode; // 设置颜色格式
  DMA2D->OCOLR = LCD_RGB.Color;      // 颜色

  if (LCD_RGB.Direction == Direction_H_RGB) // 横屏填充
  {
    DMA2D->OOR = RGB_LCD_Width - width; // 设置行偏移
    DMA2D->OMAR = LCD_RGB.LayerMemoryAdd +
                  LCD_RGB.BytesPerPixel * (RGB_LCD_Width * y + x); // 地址
    DMA2D->NLR = (width << 16) | (height); // 设定长度和宽度
  } else                                   // 竖屏填充
  {
    DMA2D->OOR = RGB_LCD_Width - height; // 设置行偏移
    DMA2D->OMAR =
        LCD_RGB.LayerMemoryAdd +
        LCD_RGB.BytesPerPixel *
            ((RGB_LCD_Height - x - 1 - width) * RGB_LCD_Width + y); // 地址
    DMA2D->NLR = (width) | (height << 16); // 设定长度和宽度
  }

  DMA2D->CR |= DMA2D_CR_START; // 启动DMA2D
  while (DMA2D->CR & DMA2D_CR_START)
    ; // 等待传输完成
}

/****************************************************************************************************************************************
 * @name   RGB_LCD_FillCircle
 * @brief  RGB LCD填充圆形区域
 * @param  x - 圆心水平坐标 (0~799)
 * @param  y - 圆心垂直坐标 (0~479)
 * @param  r - 半径
 * @retval None
 * @note   移植自ST官方评估板例程
 ****************************************************************************************************************************************/
void RGB_LCD_FillCircle(uint16_t x, uint16_t y, uint16_t r) {
  int32_t D;     /* Decision Variable */
  uint32_t CurX; /* Current X Value */
  uint32_t CurY; /* Current Y Value */

  D = 3 - (r << 1);

  CurX = 0;
  CurY = r;

  while (CurX <= CurY) {
    if (CurY > 0) {
      RGB_LCD_DrawLine(x - CurX, y - CurY, x - CurX, y - CurY + 2 * CurY);
      RGB_LCD_DrawLine(x + CurX, y - CurY, x + CurX, y - CurY + 2 * CurY);
    }

    if (CurX > 0) {
      RGB_LCD_DrawLine(x - CurY, y - CurX, x - CurY, y - CurX + 2 * CurX);
      RGB_LCD_DrawLine(x + CurY, y - CurX, x + CurY, y - CurX + 2 * CurX);
    }
    if (D < 0) {
      D += (CurX << 2) + 6;
    } else {
      D += ((CurX - CurY) << 2) + 10;
      CurY--;
    }
    CurX++;
  }

  RGB_LCD_DrawCircle(x, y, r);
}
/* ================================================================== */
/*                        图层动态控制函数实现                     */
/* ================================================================== */

/**
 * @brief  开启或关闭指定硬件图层
 * @param  LayerIndex: 0 (底层) 或 1 (顶层)
 * @param  State: ENABLE 或 DISABLE
 */
void RGB_LCD_LayerEnable(uint8_t LayerIndex, FunctionalState State)
{
  if (State == ENABLE)
  {
    __HAL_LTDC_LAYER_ENABLE(&hltdc, LayerIndex);
  }
  else
  {
    __HAL_LTDC_LAYER_DISABLE(&hltdc, LayerIndex);
  }
  // 立即重载配置使其生效
  __HAL_LTDC_RELOAD_CONFIG(&hltdc);
}

/**
 * @brief  设置指定图层的显存地址
 * @param  LayerIndex: 0 或 1
 * @param  Address: 显存地址 (如 SDRAM_BANK_ADDR)
 */
void RGB_LCD_SetLayerAddress(uint8_t LayerIndex, uint32_t Address)
{
  HAL_LTDC_SetAddress(&hltdc, Address, LayerIndex);
  // 注意：HAL_LTDC_SetAddress 内部通常不包含 Reload，需要手动重载
  // 使用垂直消隐期重载防止撕裂
  HAL_LTDC_Reload(&hltdc, LTDC_RELOAD_VERTICAL_BLANKING);

  // 如果当前绘图上下文也是这一层，同步更新结构体，防止后续绘图出错
  if (LCD_RGB.Layer == LayerIndex)
  {
    LCD_RGB.LayerMemoryAdd = Address;
  }
}

/**
 * @brief  设置指定图层的恒定透明度 (Constant Alpha)
 * @param  LayerIndex: 0 或 1
 * @param  Alpha: 0(全透) ~ 255(不透)
 */
void RGB_LCD_SetLayerAlpha(uint8_t LayerIndex, uint8_t Alpha)
{
  HAL_LTDC_SetAlpha(&hltdc, Alpha, LayerIndex);
  HAL_LTDC_Reload(&hltdc, LTDC_RELOAD_VERTICAL_BLANKING);
}



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
void lcd_test(void) {
  /* ===== 测试1: 基础清屏和颜色设置 ===== */
  RGB_LCD_SetBackColor(RGB_LCD_WHITE);
  RGB_LCD_Clear();
  HAL_Delay(1000);

  /* ===== 测试2: 显示标题 ===== */
  RGB_LCD_SetColor(RGB_LCD_BLUE);
  RGB_LCD_SetTextFont(24);
  RGB_LCD_DisplayString(10, 10, "LCD Test");
  RGB_LCD_DisplayText(10, 40, "LCD测试");
  HAL_Delay(1000);

  /* ===== 测试3: ASCII字符显示 ===== */
  RGB_LCD_SetBackColor(RGB_LCD_WHITE);
  RGB_LCD_SetColor(RGB_LCD_RED);
  RGB_LCD_SetTextFont(20);
  RGB_LCD_DisplayString(10, 80, "ASCII Font:");
  RGB_LCD_SetTextFont(16);
  RGB_LCD_DisplayString(10, 110, "Hello World 0123456789");
  HAL_Delay(1000);

  /* ===== 测试4: 中英文混合文本 ===== */
  RGB_LCD_SetColor(RGB_LCD_GREEN);
  RGB_LCD_SetTextFont(20);
  RGB_LCD_DisplayString(10, 150, "Chinese Text:");
  RGB_LCD_SetTextFont(24);
  RGB_LCD_DisplayText(10, 180, "你好 Hello STM32");
  HAL_Delay(1000);

  /* ===== 测试5: 数字显示 ===== */
  RGB_LCD_SetBackColor(RGB_LCD_WHITE);
  RGB_LCD_SetColor(RGB_LCD_MAGENTA);
  RGB_LCD_SetTextFont(20);
  RGB_LCD_DisplayString(10, 230, "Numbers:");
  RGB_LCD_SetTextFont(16);

  /* 显示整数（填充空格模式） */
  RGB_LCD_ShowNumMode(Fill_Space_RGB);
  RGB_LCD_DisplayString(10, 260, "Integer:");
  RGB_LCD_DisplayNumber(120, 260, 12345, 6);

  /* 显示整数（填充0模式） */
  RGB_LCD_ShowNumMode(Fill_Zero_RGB);
  RGB_LCD_DisplayString(10, 285, "Fill Zero:");
  RGB_LCD_DisplayNumber(120, 285, 42, 6);

  /* 显示小数 */
  RGB_LCD_DisplayString(10, 310, "Decimal:");
  RGB_LCD_DisplayDecimals(120, 310, 3.14159, 8, 4);

  HAL_Delay(2000);

  /* ===== 测试6: 2D图形 - 直线 ===== */
  RGB_LCD_SetBackColor(RGB_LCD_WHITE);
  RGB_LCD_Clear();

  RGB_LCD_SetColor(RGB_LCD_RED);
  RGB_LCD_SetTextFont(20);
  RGB_LCD_DisplayString(10, 10, "2D Graphics - Lines:");

  RGB_LCD_SetColor(RGB_LCD_RED);
  RGB_LCD_DrawLine(50, 50, 300, 50);
  RGB_LCD_DrawLine(50, 50, 50, 150);
  RGB_LCD_DrawLine(300, 50, 300, 150);
  RGB_LCD_DrawLine(50, 150, 300, 150);
  HAL_Delay(1000);

  /* ===== 测试7: 2D图形 - 矩形 ===== */
  RGB_LCD_SetColor(RGB_LCD_GREEN);
  RGB_LCD_DrawRect(350, 50, 150, 100);
  RGB_LCD_DrawRect(360, 60, 130, 80);

  /* 填充矩形 */
  RGB_LCD_FillRect(530, 60, 100, 80);
  HAL_Delay(1000);

  /* ===== 测试8: 2D图形 - 圆形 ===== */
  RGB_LCD_SetColor(RGB_LCD_BLUE);
  RGB_LCD_DrawCircle(100, 250, 40);
  RGB_LCD_DrawCircle(100, 250, 30);
  RGB_LCD_DrawCircle(100, 250, 20);

  /* 填充圆形 */
  RGB_LCD_FillCircle(220, 250, 35);
  HAL_Delay(1000);

  /* ===== 测试9: 2D图形 - 椭圆 ===== */
  RGB_LCD_SetColor(RGB_LCD_CYAN);
  RGB_LCD_DrawEllipse(350, 250, 60, 40);
  RGB_LCD_DrawEllipse(520, 250, 50, 30);
  HAL_Delay(1000);

  /* ===== 测试10: 彩色方块 ===== */
  RGB_LCD_SetBackColor(RGB_LCD_WHITE);
  RGB_LCD_Clear();

  RGB_LCD_SetTextFont(16);
  RGB_LCD_SetColor(RGB_LCD_BLACK);
  RGB_LCD_DisplayString(10, 10, "Color Blocks:");

  /* 绘制不同颜色的方块 */
  RGB_LCD_FillRect(20, 50, 60, 60); // 黑色已设置

  RGB_LCD_SetColor(RGB_LCD_RED);
  RGB_LCD_FillRect(100, 50, 60, 60);

  RGB_LCD_SetColor(RGB_LCD_GREEN);
  RGB_LCD_FillRect(180, 50, 60, 60);

  RGB_LCD_SetColor(RGB_LCD_BLUE);
  RGB_LCD_FillRect(260, 50, 60, 60);

  RGB_LCD_SetColor(RGB_LCD_YELLOW);
  RGB_LCD_FillRect(340, 50, 60, 60);

  RGB_LCD_SetColor(RGB_LCD_CYAN);
  RGB_LCD_FillRect(420, 50, 60, 60);

  RGB_LCD_SetColor(RGB_LCD_MAGENTA);
  RGB_LCD_FillRect(500, 50, 60, 60);

  /* 亮色系 */
  RGB_LCD_SetColor(RGB_LIGHT_RED);
  RGB_LCD_FillRect(20, 130, 60, 60);

  RGB_LCD_SetColor(RGB_LIGHT_GREEN);
  RGB_LCD_FillRect(100, 130, 60, 60);

  RGB_LCD_SetColor(RGB_LIGHT_BLUE);
  RGB_LCD_FillRect(180, 130, 60, 60);

  HAL_Delay(2000);

  /* ===== 测试11: 渐变效果（绘制多条水平线） ===== */
  RGB_LCD_SetBackColor(RGB_LCD_WHITE);
  RGB_LCD_Clear();

  RGB_LCD_SetTextFont(16);
  RGB_LCD_SetColor(RGB_LCD_BLACK);
  RGB_LCD_DisplayString(10, 10, "Gradient Lines:");

  /* 绘制颜色渐变的水平线 */
  for (uint16_t i = 0; i < 200; i += 5) {
    if (i < 50) {
      RGB_LCD_SetColor(RGB_LCD_RED);
    } else if (i < 100) {
      RGB_LCD_SetColor(RGB_LCD_YELLOW);
    } else if (i < 150) {
      RGB_LCD_SetColor(RGB_LCD_GREEN);
    } else {
      RGB_LCD_SetColor(RGB_LCD_BLUE);
    }
    RGB_LCD_DrawLine(50, 50 + i, 400, 50 + i);
  }
  HAL_Delay(2000);

  /* ===== 测试12: 几何图案组合 ===== */
  RGB_LCD_SetBackColor(RGB_LCD_WHITE);
  RGB_LCD_Clear();

  RGB_LCD_SetTextFont(20);
  RGB_LCD_SetColor(RGB_LCD_BLACK);
  RGB_LCD_DisplayString(10, 10, "Combination:");

  /* 绘制嵌套的图形 */
  RGB_LCD_SetColor(RGB_LCD_RED);
  RGB_LCD_DrawRect(50, 60, 200, 150);

  RGB_LCD_SetColor(RGB_LCD_GREEN);
  RGB_LCD_DrawCircle(150, 135, 50);

  RGB_LCD_SetColor(RGB_LCD_BLUE);
  RGB_LCD_FillRect(320, 60, 100, 100);
  RGB_LCD_DrawLine(320, 60, 420, 160);
  RGB_LCD_DrawLine(320, 160, 420, 60);

  HAL_Delay(2000);

  /* ===== 测试完成 ===== */
  RGB_LCD_SetBackColor(RGB_LCD_WHITE);
  RGB_LCD_Clear();

  RGB_LCD_SetColor(RGB_LCD_GREEN);
  RGB_LCD_SetTextFont(24);
  RGB_LCD_DisplayText(200, 200, "测试完成");
  RGB_LCD_DisplayString(200, 240, "Test Finished");

  HAL_Delay(3000);

  /* 恢复到初始状态 */
  RGB_LCD_SetBackColor(RGB_LCD_BLACK);
  RGB_LCD_SetColor(RGB_LCD_WHITE);
  RGB_LCD_Clear();
}
#endif
