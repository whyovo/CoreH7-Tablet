/**
 ******************************************************************************
 * @file    dcmi_ov5640.h
 * @author  菜菜why(B站:菜菜whyy)
 * @brief   OV5640 摄像头 DCMI 驱动头文件
 ******************************************************************************
 * @attention
 *
 * 使用方法:
 * - 调用 DCMI_OV5640_Init() 初始化摄像头和 DCMI 接口
 * - 调用 OV5640_DMA_Transmit_Continuous() 启动连续采集模式
 * - 通过 OV5640_FrameState 标志监控帧传输完成
 * - 调用 OV5640_DCMI_Stop() 停止采集
 *
 *  * OV5640 引脚连接:
 * - PWDN -> GPIO    (电源关闭，高电平关闭)
 * - XVCLK -> 24MHz 时钟
 * - SCL/SDA -> I2C 接口 (配置接口)
 * - PCLK/VSYNC/HSYNC/D[9:0] -> DCMI 接口 (数据接口)
 *
 ******************************************************************************
 */

#ifndef __DCMI_OV5640_H
#define __DCMI_OV5640_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "config.h"

#ifdef OV5640_ENABLE

#include "dcmi_sccb.h"
/* ============================================================ */
/*                    摄像头输出目标强制选择                      */
/* ============================================================ */
/**
 * @brief 定义此宏，强制摄像头适配 SPI 屏幕分辨率
 * @note  如果不定义此宏，且开启了 LCD_RGB_ENABLE，摄像头默认适配 RGB 屏 (800x480)
 * @note  当你同时开启 RGB 和 SPI 屏，但想在 SPI 屏上看摄像头时，请取消下方注释
 */
// #define OV5640_USE_SPI_CAM

/* ============================================================ */
/*                    是否开启高帧数选择  （可能出现稳定性问题）  */
/* ============================================================ */
/**
 * @brief 定义此宏,开启高帧数模式
 */
// #define OV5640_USE_HIGH_FPS

/* ============================================================================ */
/*                        分辨率自动适配逻辑 (核心解耦点)                        */
/* ============================================================================ */

/* 1. 最高优先级：用户强制指定使用 SPI 屏幕分辨率 */
#if defined(OV5640_USE_SPI_CAM) && defined(LCD_SPI_ENABLE)
#include "SPI/lcd_spi.h"
#define OV5640_CROP_WIDTH LCD_Width
#define OV5640_CROP_HEIGHT LCD_Height
#define OV5640_Width 440
#define OV5640_Height 330

/* 2. 其次匹配 RGB 屏幕 (默认策略) */
#elif defined(LCD_RGB_ENABLE)
#include "LTDC/lcd_rgb.h"
/* 使用 RGB 屏的分辨率宏 */
#define OV5640_CROP_WIDTH RGB_LCD_Width
#define OV5640_CROP_HEIGHT RGB_LCD_Height
#define OV5640_Width 880
#define OV5640_Height 495

/* 3. 最后匹配 SPI 屏幕 (无 RGB 时的策略) */
#elif defined(LCD_SPI_ENABLE)
#include "SPI/lcd_spi.h"
/* 使用 SPI 屏的分辨率宏 */
#define OV5640_CROP_WIDTH LCD_Width
#define OV5640_CROP_HEIGHT LCD_Height
#define OV5640_Width 440
#define OV5640_Height 330

/* 4. 默认值 (防止报错) */
#else
#define OV5640_CROP_WIDTH 320
#define OV5640_CROP_HEIGHT 240
#define OV5640_Width 880
#define OV5640_Height 495
#endif



    /* ============================================================================ */
    /*                              变量与宏定义                                   */
    /* ============================================================================ */

    /* DCMI 状态标志，当数据帧传输完成时，会被 HAL_DCMI_FrameEventCallback() 中断回调函数置 1 */
    extern volatile uint8_t OV5640_FrameState;
    extern volatile uint8_t OV5640_FPS;

/* 像素格式定义 */
#define Pixformat_RGB565 0 /*!< RGB565 格式 */
#define Pixformat_JPEG 1   /*!< JPEG 格式 */
#define Pixformat_GRAY 2   /*!< 灰度格式 */

/* 自动对焦状态 */
#define OV5640_AF_Focusing 2 /*!< 正在对焦 */
#define OV5640_AF_End 1      /*!< 对焦完成 */

/* 操作结果状态 */
#define OV5640_Success 0 /*!< 操作成功 */
#define OV5640_Error -1  /*!< 操作失败 */

/* 使能/禁止 */
#define OV5640_Enable 1  /*!< 使能 */
#define OV5640_Disable 0 /*!< 禁止 */

/* OV5640 特效模式 */
#define OV5640_Effect_Normal 0   /*!< 正常模式 */
#define OV5640_Effect_Negative 1 /*!< 负片模式 */
#define OV5640_Effect_BW 2       /*!< 黑白模式 */
#define OV5640_Effect_Solarize 3 /*!< 正负片叠加模式 */

    /* ============================================================================ */
    /*                              常用寄存器定义                                 */
    /* ============================================================================ */

#define OV5640_ChipID_H 0x300A
#define OV5640_ChipID_L 0x300B
#define OV5640_FORMAT_CONTROL 0x4300
#define OV5640_FORMAT_CONTROL_MUX 0x501F
#define OV5640_JPEG_MODE_SELECT 0x4713
#define OV5640_JPEG_VFIFO_CTRL00 0x4600
#define OV5640_JPEG_VFIFO_HSIZE_H 0x4602
#define OV5640_JPEG_VFIFO_HSIZE_L 0x4603
#define OV5640_JPEG_VFIFO_VSIZE_H 0x4604
#define OV5640_JPEG_VFIFO_VSIZE_L 0x4605
#define OV5640_GroupAccess 0X3212
#define OV5640_TIMING_DVPHO_H 0x3808
#define OV5640_TIMING_DVPHO_L 0x3809
#define OV5640_TIMING_DVPVO_H 0x380A
#define OV5640_TIMING_DVPVO_L 0x380B
#define OV5640_TIMING_Flip 0x3820
#define OV5640_TIMING_Mirror 0x3821
#define OV5640_AF_CMD_MAIN 0x3022
#define OV5640_AF_CMD_ACK 0x3023
#define OV5640_AF_FW_STATUS 0x3029

    /* ============================================================================ */
    /*                              导出函数声明                                   */
    /* ============================================================================ */

    /**
     * @brief  初始化 DCMI 和 OV5640 摄像头
     * @retval OV5640_Success: 初始化成功，OV5640_Error: 初始化失败
     * @note   会依次初始化 SCCB、DCMI、DMA 以及配置 OV5640 参数。
     *         会自动根据 config.h 中启用的屏幕类型(RGB/SPI)设置裁剪窗口大小。
     */
    int8_t DCMI_OV5640_Init(void);

    /**
     * @brief  启动 DMA 连续传输模式
     * @param  DMA_Buffer: DMA 将要传输的地址
     * @param  DMA_BufferSize: 传输的数据大小(32位宽)
     * @note   OV5640 RGB565 模式下：1个像素=2字节，需要除以4；
     *         例如 240*240 分辨率：DMA_BufferSize = (240*240*2) / 4 = 28800
     */
    void OV5640_DMA_Transmit_Continuous(uint32_t DMA_Buffer, uint32_t DMA_BufferSize);

    /**
     * @brief  启动 DMA 快照模式(采集一帧后停止)
     * @param  DMA_Buffer: DMA 将要传输的地址
     * @param  DMA_BufferSize: 传输的数据大小(32位宽)
     * @note   传输完成后需调用 OV5640_DCMI_Resume() 恢复 DCMI
     */
    void OV5640_DMA_Transmit_Snapshot(uint32_t DMA_Buffer, uint32_t DMA_BufferSize);

    /**
     * @brief  挂起 DCMI，停止捕获数据
     * @retval None
     */
    void OV5640_DCMI_Suspend(void);

    /**
     * @brief  恢复 DCMI，开始捕获数据
     * @retval None
     */
    void OV5640_DCMI_Resume(void);

    /**
     * @brief  停止 DCMI 捕获
     * @retval None
     */
    void OV5640_DCMI_Stop(void);

    /**
     * @brief  使用 DCMI 裁剪功能调整输出图像大小
     * @param  Displey_XSize: 显示屏宽度
     * @param  Displey_YSize: 显示屏高度
     * @param  Sensor_XSize: 传感器输出宽度
     * @param  Sensor_YSize: 传感器输出高度
     * @retval OV5640_Success: 成功，OV5640_Error: 失败
     * @note   DCMI 水平有效像素必须能被4整除
     */
    int8_t OV5640_DCMI_Crop(uint16_t Displey_XSize, uint16_t Displey_YSize,
                            uint16_t Sensor_XSize, uint16_t Sensor_YSize);

    /**
     * @brief  执行 OV5640 软件复位
     * @retval None
     */
    void OV5640_Reset(void);

    /**
     * @brief  读取 OV5640 器件 ID
     * @retval 器件 ID (16位)
     */
    uint16_t OV5640_ReadID(void);

    /**
     * @brief  配置 OV5640 各项参数
     * @retval None
     */
    void OV5640_Config(void);

    /**
     * @brief  设置图像输出格式
     * @param  pixformat: Pixformat_RGB565/Pixformat_JPEG/Pixformat_GRAY
     * @retval None
     */
    void OV5640_Set_Pixformat(uint8_t pixformat);

    /**
     * @brief  设置 JPEG 压缩等级
     * @param  scale: 压缩等级 (0x01~0x3F)，值越大压缩越厉害
     * @retval None
     */
    void OV5640_Set_JPEG_QuantizationScale(uint8_t scale);

    /**
     * @brief  设置实际输出的图像大小
     * @param  width: 图像宽度(像素)
     * @param  height: 图像高度(像素)
     * @retval OV5640_Success: 成功，OV5640_Error: 失败
     */
    int8_t OV5640_Set_Framesize(uint16_t width, uint16_t height);

    /**
     * @brief  设置水平镜像
     * @param  ConfigState: OV5640_Enable 或 OV5640_Disable
     * @retval OV5640_Success: 成功，OV5640_Error: 失败
     */
    int8_t OV5640_Set_Horizontal_Mirror(int8_t ConfigState);

    /**
     * @brief  设置垂直翻转
     * @param  ConfigState: OV5640_Enable 或 OV5640_Disable
     * @retval OV5640_Success: 成功，OV5640_Error: 失败
     */
    int8_t OV5640_Set_Vertical_Flip(int8_t ConfigState);

    /**
     * @brief  设置亮度
     * @param  Brightness: 亮度等级 (-4 ~ +4)
     * @retval None
     */
    void OV5640_Set_Brightness(int8_t Brightness);

    /**
     * @brief  设置对比度
     * @param  Contrast: 对比度等级 (-3 ~ +3)
     * @retval None
     */
    void OV5640_Set_Contrast(int8_t Contrast);

    /**
     * @brief  设置特效模式
     * @param  effect_Mode: OV5640_Effect_Normal/Negative/BW/Solarize
     * @retval None
     */
    void OV5640_Set_Effect(uint8_t effect_Mode);

    /**
     * @brief  下载自动对焦固件到 OV5640
     * @retval OV5640_Success: 成功，OV5640_Error: 失败
     * @note   OV5640 片内无 Flash，每次上电都需要重新下载固件
     */
    int8_t OV5640_AF_Download_Firmware(void);

    /**
     * @brief  查询对焦状态
     * @retval OV5640_AF_End: 对焦完成，OV5640_AF_Focusing: 正在对焦
     */
    int8_t OV5640_AF_QueryStatus(void);

    /**
     * @brief  触发持续自动对焦
     * @retval None
     * @note   当检测到画面不在焦点时会一直对焦，无需用户干预
     */
    void OV5640_AF_Trigger_Constant(void);

    /**
     * @brief  触发单次自动对焦
     * @retval None
     * @note   对焦过程持续约 500ms，可通过 OV5640_AF_QueryStatus() 查询进度
     */
    void OV5640_AF_Trigger_Single(void);

    /**
     * @brief  释放对焦马达，镜头回到初始位置
     * @retval None
     */
    void OV5640_AF_Release(void);
    /**
     * @brief  设置数码变焦等级
     * @param  zoom_percent: 0~100 (0为最广角，100为最大变焦/1:1裁剪)
     * @note   基于改变 ISP 输入窗口大小实现
     */
    void OV5640_Set_Zoom(uint8_t zoom_percent);
    /* ============================================================================ */
    /*                              引脚配置宏定义                                 */
    /* ============================================================================ */

#define OV5640_PWDN_PIN GPIO_PIN_7
#define OV5640_PWDN_PORT GPIOG
#define GPIO_OV5640_PWDN_CLK_ENABLE __HAL_RCC_GPIOG_CLK_ENABLE()

#define OV5640_PWDN_OFF HAL_GPIO_WritePin(OV5640_PWDN_PORT, OV5640_PWDN_PIN, GPIO_PIN_RESET)
#define OV5640_PWDN_ON HAL_GPIO_WritePin(OV5640_PWDN_PORT, OV5640_PWDN_PIN, GPIO_PIN_SET)

#endif /* OV5640_ENABLE */

#ifdef __cplusplus
}
#endif

#endif //__DCMI_OV5640_H
