/**
 * @file oled.c
 * @author 菜菜why（B站：菜菜whyy）
 *  【汉字】取模方式：阴码、逆向、逐行式、C51格式
 * 【单色图标】
 *  1.取模参数：阴码、逆向、逐行式、C51格式
 *  2.数据格式：1位/像素 (黑白两色)
 */

#include "oled.h"

#ifdef OLED_I2C_ENABLE


#include <string.h>

/* 显存缓冲区：8页 * 128列 */
#if defined(__GNUC__)
__attribute__((aligned(32))) uint8_t OLED_GRAM[8][128];
#else
__align(32) uint8_t OLED_GRAM[8][128];
#endif

/* 初始化命令序列 (SSD1306 标准) */
static const uint8_t OLED_INIT_CMD[] = {
    0xAE,         // Display Off
    0x20, 0x00,   // Set Memory Addressing Mode -> Horizontal
    0x21, 0, 127, // Set Column Address
    0x22, 0, 7,   // Set Page Address
    0x00, 0x10, 0x40, 0xB0, 0x81, 0xFF, 0xA1, 0xA6, 0xA8, 0x3F,
    0xC8, 0xD3, 0x00, 0xD5, 0x80, 0xD8, 0x05, 0xD9, 0xF1, 0xDA, 0x12,
    0xD8, 0x30, 0x8D, 0x14, 0xAF // Display On
};

/* =================================================================================
 *                               底层接口实现 (Backend)
 * ================================================================================= */

#ifdef OLED_USE_HARD_I2C
/* ---------------- 硬件 I2C 实现 ---------------- */

/**
 * @brief  向 OLED 发送单字节命令 (硬件 I2C)
 * @param  cmd: 命令字节
 */
static void OLED_Write_Cmd(uint8_t cmd)
{
    HAL_I2C_Mem_Write(&OLED_HARD_HANDLE, 0x78, 0x00, I2C_MEMADD_SIZE_8BIT, &cmd, 1, 100);
}

/**
 * @brief  初始化 OLED (硬件 I2C)
 * @note   使用阻塞模式发送初始化序列，并清空屏幕
 */
void OLED_Init(void)
{
    HAL_Delay(200);
    // 阻塞发送初始化命令
    HAL_I2C_Mem_Write(&OLED_HARD_HANDLE, 0x78, 0x00, I2C_MEMADD_SIZE_8BIT, (uint8_t *)OLED_INIT_CMD, sizeof(OLED_INIT_CMD), 100);
    OLED_Clear();
}

/**
 * @brief  将显存刷新到屏幕 (硬件 I2C DMA)
 * @note   非阻塞传输，使用前会检查 I2C 状态并清理 D-Cache
 */
void OLED_Refresh(void)
{
    while (HAL_I2C_GetState(&OLED_HARD_HANDLE) != HAL_I2C_STATE_READY)
        ;

    // 重置指针
    uint8_t ptr_reset[] = {0x21, 0, 127, 0x22, 0, 7};
    HAL_I2C_Mem_Write(&OLED_HARD_HANDLE, 0x78, 0x00, I2C_MEMADD_SIZE_8BIT, ptr_reset, sizeof(ptr_reset), 100);

    // DMA 传输显存
    SCB_CleanDCache_by_Addr((uint32_t *)OLED_GRAM, sizeof(OLED_GRAM));
    HAL_I2C_Mem_Write_DMA(&OLED_HARD_HANDLE, 0x78, 0x40, I2C_MEMADD_SIZE_8BIT, (uint8_t *)OLED_GRAM, 128 * 8);
}

#elif defined(OLED_USE_SOFT_I2C)
/* ---------------- 软件 I2C 实现 ---------------- */
#include "soft_i2c.h"

static SOFT_I2C_Handle s_h;

/**
 * @brief  软件 I2C 批量写数据
 * @param  control: 控制字节 (0x00 命令 / 0x40 数据)
 * @param  buf: 数据缓冲区
 * @param  len: 长度
 */
static void OLED_Write_Bytes(uint8_t control, const uint8_t *buf, uint16_t len)
{
    SOFT_I2C_Start(&s_h);
    SOFT_I2C_WriteByte(&s_h, 0x78);
    SOFT_I2C_WaitAck(&s_h);
    SOFT_I2C_WriteByte(&s_h, control);
    SOFT_I2C_WaitAck(&s_h);
    for (uint16_t i = 0; i < len; ++i)
    {
        SOFT_I2C_WriteByte(&s_h, buf[i]);
        SOFT_I2C_WaitAck(&s_h);
    }
    SOFT_I2C_Stop(&s_h);
}

/**
 * @brief  向 OLED 发送单字节命令 (软件 I2C)
 */
static void OLED_Write_Cmd(uint8_t cmd)
{
    OLED_Write_Bytes(0x00, &cmd, 1);
}

/**
 * @brief  初始化 OLED (软件 I2C)
 */
void OLED_Init(void)
{
    SOFT_I2C_ConfigHandle(&s_h, OLED_SOFT_SCL_PORT, OLED_SOFT_SCL_PIN,
                          OLED_SOFT_SDA_PORT, OLED_SOFT_SDA_PIN, OLED_SOFT_DELAY_US);
    HAL_Delay(200);
    OLED_Write_Bytes(0x00, OLED_INIT_CMD, sizeof(OLED_INIT_CMD));
    OLED_Clear();
}

/**
 * @brief  将显存刷新到屏幕 (软件 I2C 阻塞逐页刷新)
 */
void OLED_Refresh(void)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        OLED_Write_Cmd(0xB0 + i);
        OLED_Write_Cmd(0x00);
        OLED_Write_Cmd(0x10);
        OLED_Write_Bytes(0x40, &OLED_GRAM[i][0], 128);
    }
}

#endif

/* =================================================================================
 *                               通用功能实现 (Common)
 * ================================================================================= */

/**
 * @brief  清空显存缓冲区（不刷新屏幕）
 */
void OLED_ClearRAM(void)
{
    memset(OLED_GRAM, 0, sizeof(OLED_GRAM));
}

/**
 * @brief  清屏（将显存所有字节写为 0 并刷新）
 */
void OLED_Clear(void)
{
    OLED_ClearRAM();
    OLED_Refresh();
}

/**
 * @brief  开启显示
 */
void OLED_Display_On(void)
{
    OLED_Write_Cmd(0X8D);
    OLED_Write_Cmd(0X14);
    OLED_Write_Cmd(0XAF);
}

/**
 * @brief  关闭显示
 */
void OLED_Display_Off(void)
{
    OLED_Write_Cmd(0X8D);
    OLED_Write_Cmd(0X10);
    OLED_Write_Cmd(0XAE);
}

/**
 * @brief  全屏点亮 (测试用)
 */
void OLED_On(void)
{
    memset(OLED_GRAM, 0xFF, sizeof(OLED_GRAM));
    OLED_Refresh();
}

/**
 * @brief  在显存中画一个点
 * @param  x: 列坐标 (0~127)
 * @param  y: 行坐标 (0~63)
 * @param  t: 颜色 (1: 亮, 0: 灭)
 */
void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t t)
{
    uint8_t page = y / 8;
    uint8_t bit = y % 8;
    if (x > 127 || page > 7)
        return;

    if (t)
        OLED_GRAM[page][x] |= (1 << bit);
    else
        OLED_GRAM[page][x] &= ~(1 << bit);
}

/**
 * @brief  内部函数：计算 m^n
 */
static uint32_t oled_pow(uint8_t m, uint8_t n)
{
    uint32_t result = 1;
    while (n--)
        result *= m;
    return result;
}

/**
 * @brief  显示单个 ASCII 字符
 * @param  x: 起始列
 * @param  y: 起始页 (0~7)
 * @param  chr: 字符
 * @param  Char_Size: 字体大小 (16: 8x16, 其他: 6x8)
 */
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t Char_Size)
{
    unsigned char c = chr - ' ';
    uint8_t i;
    if (x > 127)
        return;

    if (Char_Size == 16)
    {
        for (i = 0; i < 8; i++)
        {
            if (x + i > 127)
                break;
            OLED_GRAM[y][x + i] = F8X16[c * 16 + i];
            OLED_GRAM[y + 1][x + i] = F8X16[c * 16 + i + 8];
        }
    }
    else
    {
        for (i = 0; i < 6; i++)
        {
            if (x + i > 127)
                break;
            OLED_GRAM[y][x + i] = F6x8[c][i];
        }
    }
}

/**
 * @brief  显示整数（按固定宽度显示）
 * @param  x: 起始列
 * @param  y: 起始页
 * @param  num: 数值
 * @param  len: 位数
 * @param  size2: 字体高度
 */
void OLED_ShowNum(uint8_t x, uint8_t y, unsigned int num, uint8_t len, uint8_t size2)
{
    uint8_t t, temp;
    uint8_t enshow = 0;
    for (t = 0; t < len; t++)
    {
        temp = (num / oled_pow(10, len - t - 1)) % 10;
        if (enshow == 0 && t < (len - 1))
        {
            if (temp == 0)
            {
                OLED_ShowChar(x + (size2 / 2) * t, y, ' ', size2);
                continue;
            }
            else
                enshow = 1;
        }
        OLED_ShowChar(x + (size2 / 2) * t, y, temp + '0', size2);
    }
}

/**
 * @brief  显示字符串
 * @param  x: 起始列
 * @param  y: 起始页
 * @param  chr: 字符串指针
 * @param  Char_Size: 字体大小
 */
void OLED_ShowString(uint8_t x, uint8_t y, uint8_t *chr, uint8_t Char_Size)
{
    uint8_t j = 0;
    while (chr[j] != '\0')
    {
        OLED_ShowChar(x, y, chr[j], Char_Size);
        x += 8;
        if (x > 120)
        {
            x = 0;
            y += 2;
        }
        j++;
    }
}

/**
 * @brief  显示汉字
 * @param  x: 起始列
 * @param  y: 起始页
 * @param  no: 汉字索引
 */
void OLED_ShowChinese(uint8_t x, uint8_t y, uint8_t no)
{
    uint8_t i, k, byte1, byte2;
    uint8_t y0 = y * 8;
    if (x > 127 - 16 || y > 6)
        return;

    for (i = 0; i < 16; i++)
    {
        if (i < 8)
        {
            byte1 = Hzk[2 * no][i * 2];
            byte2 = Hzk[2 * no][i * 2 + 1];
        }
        else
        {
            byte1 = Hzk[2 * no + 1][(i - 8) * 2];
            byte2 = Hzk[2 * no + 1][(i - 8) * 2 + 1];
        }
        for (k = 0; k < 8; k++)
        {
            OLED_DrawPoint(x + k, y0 + i, (byte1 >> k) & 0x01);
            OLED_DrawPoint(x + 8 + k, y0 + i, (byte2 >> k) & 0x01);
        }
    }
}

/**
 * @brief  显示位图图像
 * @param  x, y: 起始位置
 * @param  w, h: 宽高
 * @param  bitmap: 数据指针
 */
void OLED_ShowImage(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *bitmap)
{
    if (!bitmap)
        return;
    uint8_t dx, dy, byte_val;
    uint8_t y0 = y * 8;
    uint8_t bytes_per_row = (w + 7) / 8;

    for (dy = 0; dy < h; dy++)
    {
        for (dx = 0; dx < w; dx++)
        {
            if (x + dx > 127 || y0 + dy > 63)
                continue;
            byte_val = bitmap[dy * bytes_per_row + dx / 8];
            OLED_DrawPoint(x + dx, y0 + dy, (byte_val >> (dx % 8)) & 0x01);
        }
    }
}

/**
 * @brief  播放 GIF 动画
 * @param  frames: 帧数组
 * @param  frame_count: 总帧数
 * @param  frame_delay_ms: 帧间隔
 * @param  loop_count: 循环次数
 */
void OLED_ShowGIF(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                  const uint8_t *frames[], uint16_t frame_count,
                  uint16_t frame_delay_ms, uint16_t loop_count)
{
    if (!frames || frame_count == 0)
        return;
    if (loop_count == 0)
        loop_count = 1;

    for (uint16_t loop = 0; loop < loop_count; ++loop)
    {
        for (uint16_t f = 0; f < frame_count; ++f)
        {
            if (frames[f])
            {
                OLED_ShowImage(x, y, w, h, frames[f]);
                OLED_Refresh();
                HAL_Delay(frame_delay_ms);
            }
        }
    }
}

/**
 * @brief  显示浮点数
 * @param  num: 浮点数值
 * @param  a: 整数部分长度
 * @param  b: 小数部分长度
 */
void OLED_ShowFloat(uint8_t x, uint8_t y, float num, uint8_t a, uint8_t b, uint8_t size)
{
    if (num < 0)
    {
        OLED_ShowChar(x, y, '-', size);
        x += size / 2;
        num = -num;
    }
    OLED_ShowNum(x, y, (unsigned int)num, a, size);
    OLED_ShowChar(x + a * (size / 2), y, '.', size);
    for (uint8_t i = 1; i <= b; i++)
    {
        num *= 10;
        OLED_ShowNum(x + (a + i) * (size / 2), y, (unsigned int)num % 10, 1, size);
    }
}

/**
 * @brief  功能测试演示
 */
void OLED_Test_Demo(void)
{
    OLED_Init();
    OLED_ClearRAM();
    #ifdef OLED_USE_HARD_I2C
    OLED_ShowString(0, 0, (uint8_t *)"HardI2C OLED", 16);
    #elif defined(OLED_USE_SOFT_I2C)
    OLED_ShowString(0, 0, (uint8_t *)"SoftI2C OLED", 16);
    #endif
    OLED_ShowChinese(0, 2, 0);
    OLED_ShowChinese(16, 2, 1);
    OLED_ShowFloat(0, 4, 3.1415, 1, 4, 16);
    OLED_Refresh();
}

#endif /* OLED_I2C_ENABLE */
