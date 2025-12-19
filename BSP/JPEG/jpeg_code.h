/**
 ******************************************************************************
 * @file    jpeg_code.h
 * @author  菜菜why（B站：菜菜whyy）
 * @brief   硬件JPEG编解码驱动头文件
 ******************************************************************************
 * @attention
 *
 * 功能说明：
 * ---------------------------------------------------------------
 * 1. 定义JPEG硬件编解码驱动的接口和存储区域划分
 * 2. 基于STM32H7硬件JPEG编解码器
 * 3. 支持JPEG解码和编码功能
 *
 * 存储区域划分：
 * ---------------------------------------------------------------
 * LCD_FRAME_BUFFER          -> LTDC显存基地址
 * JPEG_OUTPUT_DATA_BUFFER   -> JPEG解码输出缓冲区(2MB)
 * JPEG_ENCODE_OUTPUT_BUFFER -> JPEG编码输出缓冲区(2MB)
 * File_BUFFER               -> 文件数据缓冲区(用于SD卡/SPI Flash)
 *
 * 重要说明：
 * ---------------------------------------------------------------
 * 1. 硬件JPEG不能解码渐进式(Progressive)的图片！！！
 * 2. JPG图片必须保存为基准式(Baseline)格式
 * 3. 当前配置为单层显示，如需双层显示需重新计算地址
 * 4. JPEG解码输出为YCbCr格式，需通过DMA2D转换为RGB显示
 * 5. JPEG编码输入支持RGB565和YCbCr格式
 * 6. 所有缓冲区地址基于SDRAM内存映射
 *
 ******************************************************************************
 */

#ifndef __JPEG_CODE_H
#define __JPEG_CODE_H

#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
 *                              状态标志定义
 ******************************************************************************/

/**
 * @brief JPEG硬件操作完成标志
 * @note  值为1时表示操作过程已完成
 */
#define JPEG_OpComplete 1

/**
 * @brief JPEG硬件操作开始标志
 * @note  值为0时表示操作正在进行中或尚未完成
 */
#define JPEG_OpStart 0

  /**
   * @brief JPEG编码质量等级定义
   */
  typedef enum
  {
    JPEG_Quality_Low = 0,     /*!< 低质量 (质量因子 ~50) */
    JPEG_Quality_Medium = 1,  /*!< 中等质量 (质量因子 ~75) */
    JPEG_Quality_High = 2,    /*!< 高质量 (质量因子 ~90) */
    JPEG_Quality_VeryHigh = 3 /*!< 超高质量 (质量因子 ~95) */
  } JPEG_QualityTypeDef;

  /**
   * @brief JPEG编码色度采样格式定义
   */
  typedef enum
  {
    JPEG_Encode_420 = 0, /*!< YCbCr 4:2:0采样 */
    JPEG_Encode_422 = 1, /*!< YCbCr 4:2:2采样 */
    JPEG_Encode_444 = 2  /*!< YCbCr 4:4:4采样 */
  } JPEG_EncodeSubsamplingTypeDef;

#include "config.h"
#include "my_jpeg_utils.h"

#ifdef JPEG_ENABLE
/*******************************************************************************
 *                          显存区域地址定义
 ******************************************************************************/
/**
 * @brief LTDC屏幕显存基地址
 * @note  单层显示模式下的显存地址
 */
#define LCD_FRAME_BUFFER RGB_LCD_MemoryAdd

/**
 * @brief JPEG硬件解码输出缓冲区地址
 * @note  大小: 1.5MB (0x180000)
 * @note  在DMA2D中将YCbCr转换为RGB后输出到显存
 */
#define JPEG_OUTPUT_DATA_BUFFER (0xC0400000)

/**
 * @brief JPEG硬件编码输出缓冲区地址
 * @note  大小: 1MB (0x100000)
 * @note  位于解码缓冲区之后，用于存储编码后的JPEG数据
 */
#define JPEG_ENCODE_OUTPUT_BUFFER (JPEG_OUTPUT_DATA_BUFFER + 0x180000)

/**
 * @brief 文件数据缓冲区地址
 * @note  大小: 1MB (0x100000)
 * @note  位于编码缓冲区之后
 * @note  用于存储从SD卡或SPI Flash中读取的图片数据
 * @note  仅在使用SD卡或SPI Flash时需要
 */
#define File_BUFFER (JPEG_ENCODE_OUTPUT_BUFFER + 0x100000)

  /*******************************************************************************
   *                          导出函数声明
   ******************************************************************************/

  /* ==================== 解码相关函数 ==================== */

  /**
   * @brief  启动JPEG硬件解码(DMA模式)
   * @param  SourceAddress: JPG图片源地址
   * @param  FrameSize: JPG图片大小(字节)
   * @param  DestAddress: 解码输出缓冲区地址
   * @retval 无
   * @note   此函数为异步调用，解码过程在中断中进行
   * @note   调用此函数后，需要调用JPEG_Decode_WaitingforEnd()等待解码完成
   * @note   示例:
   *         JPEG_Decode_DMA(jpg_addr, jpg_size, JPEG_OUTPUT_DATA_BUFFER);
   *         JPEG_Decode_WaitingforEnd();
   */
  void JPEG_Decode_DMA(uint32_t SourceAddress, uint32_t FrameSize,
                       uint32_t DestAddress);

  /**
   * @brief  等待JPEG硬件解码完成
   * @retval JPEG_OpComplete: 解码完成标志
   * @note   该函数为阻塞式调用，会一直等待至解码完成
   * @note   解码期间，JPEG硬件会通过DMA传输数据和触发中断
   */
  uint8_t JPEG_Decode_WaitingforEnd(void);

  /**
   * @brief  通过DMA2D将YCbCr数据转换为RGB并输出到显存
   * @param  x: 图片显示起始水平坐标(0~479)
   * @param  y: 图片显示起始垂直坐标(0~271)
   * @param  pSrc: JPEG解码后得到的YCbCr数据缓冲区地址
   * @param  pDst: LTDC当前显存地址(通常为LCD_FRAME_BUFFER)
   * @retval 无
   * @note   DMA2D会根据JPEG图片的色度采样格式自动选择转换模式
   * @note   此函数为阻塞式调用，会一直等待DMA2D转换完成
   * @note   支持的采样格式: YCbCr 4:2:0, 4:2:2, 4:4:4
   * @note   示例:
   *         DMA2D_CopyBuffer(0, 0, (uint32_t*)JPEG_OUTPUT_DATA_BUFFER,
   *                          (uint32_t*)LCD_FRAME_BUFFER);
   */
  void DMA2D_CopyBuffer(uint16_t x, uint16_t y, uint32_t *pSrc, uint32_t *pDst);

  /* ==================== 编码相关函数 ==================== */

  /**
   * @brief  获取编码后的JPEG数据大小
   * @retval 编码后JPEG数据的大小(字节)
   * @note   必须在编码完成后调用此函数
   * @note   返回值为0表示编码失败或尚未完成
   */
  uint32_t JPEG_GetEncodedSize(void);

  /**
   * @brief  使用jpeg_utils进行RGB565到YCbCr的转换并编码
   * @param  ImageWidth: 输入图片宽度(像素)
   * @param  ImageHeight: 输入图片高度(像素)
   * @param  RGBSourceAddress: RGB565输入图片源地址
   * @param  YCbCrBufferAddress: YCbCr中间缓冲区地址
   * @param  JPEGDestAddress: JPEG编码输出缓冲区地址
   * @param  Quality: 编码质量等级
   * @param  ChromaSubsampling: 色度采样格式
   * @retval 0: 成功, -1: 失败
   */
  int JPEG_Encode_RGB565(uint32_t ImageWidth, uint32_t ImageHeight,
                         uint32_t RGBSourceAddress, uint32_t YCbCrBufferAddress,
                         uint32_t JPEGDestAddress, JPEG_QualityTypeDef Quality,
                         JPEG_EncodeSubsamplingTypeDef ChromaSubsampling);
#endif

#ifdef __cplusplus
}
#endif

#endif // __JPEG_CODE_H
