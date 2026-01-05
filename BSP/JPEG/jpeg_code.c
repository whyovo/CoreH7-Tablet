/**
 ******************************************************************************
 * @file    jpeg_code.c
 * @author  菜菜why（B站：菜菜whyy）
 * @brief   硬件JPEG编解码驱动实现文件
 ******************************************************************************
 * @attention
 *
 * 功能说明：
 * ---------------------------------------------------------------
 * 1. 使用STM32H7硬件JPEG编解码器
 * 2. 该驱动移植于ST官方STM32H743I-EVAL板卡例程
 * 3. 支持JPEG解码和编码操作
 * 4. 编解码操作需要一定的缓存空间，否则会导致失败
 *
 * 重要说明：
 * ---------------------------------------------------------------
 * 1. 硬件JPEG不能解码渐进式(Progressive)的图片！！！
 * 2. JPG图片必须保存为基准式(Baseline)格式
 * 3. 编码时支持RGB565和YCbCr输入格式
 * 4. 编码质量和采样方式会影响输出文件大小和质量
 *
 * 依赖条件：
 * ---------------------------------------------------------------
 * 1. JPEG硬件已初始化配置完成
 * 2. DMA2D硬件已初始化配置完成
 * 3. SDRAM显存区域已分配
 *
 ******************************************************************************
 */

#include "jpeg_code.h"
#include "lcd_rgb.h"

#ifdef JPEG_ENABLE
extern JPEG_HandleTypeDef hjpeg;   /*!< JPEG硬件句柄 */
extern DMA2D_HandleTypeDef hdma2d; /*!< DMA2D硬件句柄 */

/*******************************************************************************
 *                              宏定义
 ******************************************************************************/
#define CHUNK_SIZE_IN ((uint32_t)(64 * 1024)) /*!< 单次解码输入数据最大长度 */
#define CHUNK_SIZE_OUT                                                         \
  ((uint32_t)(64 * 1024)) /*!< 单次解码输出数据最大长度            \
                           */
#define CHUNK_SIZE_ENCODE_IN                                                   \
  ((uint32_t)(64 * 1024)) /*!< 编码输入缓冲区大小                     \
                           */
#define CHUNK_SIZE_ENCODE_OUT                                                  \
  ((uint32_t)(64 * 1024)) /*!< 编码输出缓冲区大小                     \
                           */

/*******************************************************************************
 *                              全局变量定义
 ******************************************************************************/
__IO uint8_t Jpeg_HWOperationState = 0; /*!< 操作状态标志: 0=开始, 1=完成 */
__IO uint8_t Jpeg_IsEncoding = 0; /*!< 编码模式标志: 0=解码, 1=编码 */

uint32_t FrameBufferAddress; /*!< 缓冲区地址 */
uint32_t JPEGSourceAddress;  /*!< JPG图片源地址 */
uint32_t Input_frameSize;    /*!< JPG图片总大小 */
uint32_t Input_frameIndex;   /*!< 当前输入数据索引偏移 */
uint32_t Output_frameSize;   /*!< 输出数据总大小 */

/*******************************************************************************
 *                           解码相关函数实现
 ******************************************************************************/

/**
 * @brief  启动JPEG硬件解码(DMA模式)
 * @param  SourceAddress: JPG图片源地址
 * @param  FrameSize: JPG图片总大小(字节)
 * @param  DestAddress: 解码输出缓冲区地址
 * @retval 无
 * @note   此函数为异步调用，解码过程通过中断回调完成
 * @note   需要调用JPEG_Decode_WaitingforEnd()等待解码完成
 */
void JPEG_Decode_DMA(uint32_t SourceAddress, uint32_t FrameSize,
                     uint32_t DestAddress) {
  JPEGSourceAddress = SourceAddress; // 保存JPG图片源地址
  FrameBufferAddress = DestAddress;  // 保存解码输出缓冲区地址
  Input_frameSize = FrameSize;       // 保存JPG图片总大小

  Input_frameIndex = 0;                 // 重置数据索引
  Output_frameSize = 0;                 // 重置输出大小
  Jpeg_HWOperationState = JPEG_OpStart; // 设置解码开始标志
  Jpeg_IsEncoding = 0;                  // 设置为解码模式

  // 启动JPEG硬件DMA解码
  HAL_JPEG_Decode_DMA(&hjpeg, (uint8_t *)JPEGSourceAddress, CHUNK_SIZE_IN,
                      (uint8_t *)FrameBufferAddress, CHUNK_SIZE_OUT);
}

/**
 * @brief  等待JPEG硬件解码完成
 * @retval JPEG_OpComplete: 解码完成标志
 * @note   该函数为阻塞式调用，会一直等待至解码完成
 */
uint8_t JPEG_Decode_WaitingforEnd(void) {
  // 等待解码完成
  while (Jpeg_HWOperationState == JPEG_OpStart)
    ;

  return JPEG_OpComplete;
}

/**
 * @brief  通过DMA2D将YCbCr数据转换并输出为RGB
 * @param  x: 图片显示起始水平坐标(0~479)
 * @param  y: 图片显示起始垂直坐标(0~271)
 * @param  pSrc: JPEG解码后得到的YCbCr数据缓冲区地址
 * @param  pDst: LTDC当前显存地址
 * @retval 无
 * @note   DMA2D会根据JPEG图片的色度采样格式自动选择转换模式
 */
void DMA2D_CopyBuffer(uint16_t x, uint16_t y, uint32_t *pSrc, uint32_t *pDst) {
  uint32_t cssMode = DMA2D_CSS_420;
  uint32_t inputLineOffset = 0;
  uint32_t destination = 0;
  JPEG_ConfTypeDef JPEG_Info;

  // 获取JPEG图片信息
  HAL_JPEG_GetInfo(&hjpeg, &JPEG_Info);

  // 根据色度采样格式设置DMA2D转换参数
  // 如果采样格式为 YCbCr 4:2:0
  if (JPEG_Info.ChromaSubsampling == JPEG_420_SUBSAMPLING) {
    cssMode = DMA2D_CSS_420;
    inputLineOffset = JPEG_Info.ImageWidth % 16;
    if (inputLineOffset != 0) {
      inputLineOffset = 16 - inputLineOffset;
    }
  }
  // 如果采样格式为 YCbCr 4:4:4
  else if (JPEG_Info.ChromaSubsampling == JPEG_444_SUBSAMPLING) {
    cssMode = DMA2D_NO_CSS;

    inputLineOffset = JPEG_Info.ImageWidth % 8;
    if (inputLineOffset != 0) {
      inputLineOffset = 8 - inputLineOffset;
    }
  }
  // 如果采样格式为 YCbCr 4:2:2
  else if (JPEG_Info.ChromaSubsampling == JPEG_422_SUBSAMPLING) {
    cssMode = DMA2D_CSS_422;

    inputLineOffset = JPEG_Info.ImageWidth % 16;
    if (inputLineOffset != 0) {
      inputLineOffset = 16 - inputLineOffset;
    }
  }

  // DMA2D模式配置
  hdma2d.Init.Mode = DMA2D_M2M_PFC; // 存储器到存储器模式(带PFC转换)
  hdma2d.Init.ColorMode = DMA2D_OUTPUT_RGB565; // 输出颜色格式RGB565
  hdma2d.Init.OutputOffset = RGB_LCD_Width - JPEG_Info.ImageWidth; // 输出行偏移
  hdma2d.Init.AlphaInverted = DMA2D_REGULAR_ALPHA; // 正常透明通道
  hdma2d.Init.RedBlueSwap = DMA2D_RB_REGULAR;      // 不交换R和B颜色通道

  hdma2d.XferCpltCallback = NULL; // DMA2D完成回调(不使用)

  // DMA2D前景层配置(LTDC Layer0)
  hdma2d.LayerCfg[1].AlphaMode = DMA2D_REPLACE_ALPHA; // 透明度模式：替换
  hdma2d.LayerCfg[1].InputAlpha = 0xFF; // 恒定透明度(255=不透明)
  hdma2d.LayerCfg[1].InputColorMode = DMA2D_INPUT_YCBCR; // 输入颜色格式YCBCR
  hdma2d.LayerCfg[1].ChromaSubSampling = cssMode;   // YCBCR色度采样格式
  hdma2d.LayerCfg[1].InputOffset = inputLineOffset; // 输入行偏移
  hdma2d.LayerCfg[1].AlphaInverted = DMA2D_REGULAR_ALPHA; // 正常透明通道
  hdma2d.LayerCfg[1].RedBlueSwap = DMA2D_RB_REGULAR; // 不交换R和B颜色通道

  hdma2d.Instance = DMA2D;

  // 初始化并配置DMA2D
  HAL_DMA2D_Init(&hdma2d);
  HAL_DMA2D_ConfigLayer(&hdma2d, 1);

  // 计算输出显存地址偏移
  destination = (uint32_t)pDst + ((y * RGB_LCD_Width) + x) * 2;

  // 启动DMA2D YCbCr→RGB转换
  HAL_DMA2D_Start(&hdma2d, (uint32_t)pSrc, destination, JPEG_Info.ImageWidth,
                  JPEG_Info.ImageHeight);

  // 轮询等待DMA2D转换完成
  HAL_DMA2D_PollForTransfer(&hdma2d, 25);
}

/*******************************************************************************
 *                           编码相关函数实现
 ******************************************************************************/

/**
 * @brief  获取编码后的JPEG数据大小
 * @retval 编码后JPEG数据的大小(字节)
 * @note   必须在编码完成后调用此函数
 */
uint32_t JPEG_GetEncodedSize(void) { return Output_frameSize; }

/**
 * @brief  使用jpeg_utils进行RGB565到YCbCr的转换并编码
 */
int JPEG_Encode_RGB565(uint32_t ImageWidth, uint32_t ImageHeight,
                       uint32_t RGBSourceAddress, uint32_t YCbCrBufferAddress,
                       uint32_t JPEGDestAddress, JPEG_QualityTypeDef Quality,
                       JPEG_EncodeSubsamplingTypeDef ChromaSubsampling) {
  JPEG_ConfTypeDef JPEG_Conf;
  JPEG_RGBToYCbCr_Convert_Function pRGBToYCbCr_Convert_Function;
  uint32_t MCU_TotalNb;
  uint32_t MCU_BlockIndex = 0;
  uint32_t convertedDataCount;
  uint8_t QualityFactor = 75;
  uint32_t dataCount;

  // 初始化颜色转换表
  my_JPEG_InitColorTables();

  // 根据质量等级设置质量因子
  switch (Quality) {
  case JPEG_Quality_Low:
    QualityFactor = 50;
    break;
  case JPEG_Quality_Medium:
    QualityFactor = 75;
    break;
  case JPEG_Quality_High:
    QualityFactor = 90;
    break;
  case JPEG_Quality_VeryHigh:
    QualityFactor = 95;
    break;
  default:
    QualityFactor = 75;
    break;
  }

  // 设置JPEG编码配置
  JPEG_Conf.ImageWidth = ImageWidth;
  JPEG_Conf.ImageHeight = ImageHeight;
  JPEG_Conf.ColorSpace = JPEG_YCBCR_COLORSPACE;
  
  // 转换采样格式
  switch (ChromaSubsampling) {
  case JPEG_Encode_420:
    JPEG_Conf.ChromaSubsampling = JPEG_420_SUBSAMPLING;
    break;
  case JPEG_Encode_422:
    JPEG_Conf.ChromaSubsampling = JPEG_422_SUBSAMPLING;
    break;
  case JPEG_Encode_444:
    JPEG_Conf.ChromaSubsampling = JPEG_444_SUBSAMPLING;
    break;
  default:
    JPEG_Conf.ChromaSubsampling = JPEG_420_SUBSAMPLING;
    break;
  }
  
  JPEG_Conf.ImageQuality = QualityFactor;

  // 获取RGB到YCbCr转换函数
  if (my_JPEG_GetEncodeColorConvertFunc(&JPEG_Conf, &pRGBToYCbCr_Convert_Function,
                                      &MCU_TotalNb) != HAL_OK) {
    DEBUG_ERROR("获取转换函数失败");
    return -1;
  }

  // 计算每次处理的数据量 (按MCU对齐)
  // 对于RGB565: 每像素2字节
  uint32_t bytesPerPixel = 2;
  
  // 配置JPEG编码器
  HAL_JPEG_ConfigEncoding(&hjpeg, &JPEG_Conf);

  // 初始化编码参数
  FrameBufferAddress = JPEGDestAddress;
  Output_frameSize = 0;
  Jpeg_HWOperationState = JPEG_OpStart;
  Jpeg_IsEncoding = 1;

  // 将整个RGB图像转换为YCbCr MCU块
  // 输入: RGB565数据, 输出: YCbCr MCU块
  dataCount = ImageWidth * ImageHeight * bytesPerPixel;
  
  MCU_BlockIndex = pRGBToYCbCr_Convert_Function(
      (uint8_t *)RGBSourceAddress,      // RGB输入
      (uint8_t *)YCbCrBufferAddress,    // YCbCr输出
      0,                                 // 起始块索引
      dataCount,                         // 输入数据量
      &convertedDataCount);              // 转换后的数据量

  if (MCU_BlockIndex == 0) {
    DEBUG_ERROR("RGB转YCbCr失败");
    return -1;
  }

  // 计算YCbCr数据大小
  uint32_t ycbcrSize = convertedDataCount;
  
  // 保存相关参数供回调函数使用
  JPEGSourceAddress = YCbCrBufferAddress;
  Input_frameSize = ycbcrSize;
  Input_frameIndex = 0;

// 刷新D-Cache，确保DMA能读取到正确的YCbCr数据
  SCB_CleanDCache_by_Addr((uint32_t*)YCbCrBufferAddress, ycbcrSize + 32);

  // 启动JPEG硬件DMA编码
  HAL_JPEG_Encode_DMA(&hjpeg, (uint8_t *)YCbCrBufferAddress,
                      (ycbcrSize > CHUNK_SIZE_ENCODE_IN) ? CHUNK_SIZE_ENCODE_IN : ycbcrSize,
                      (uint8_t *)JPEGDestAddress, CHUNK_SIZE_ENCODE_OUT);

  // 等待编码完成
  while (Jpeg_HWOperationState == JPEG_OpStart)
    ;

  return 0;
}
/*******************************************************************************
 *                              HAL回调函数实现
 ******************************************************************************/

/**
 * @brief  JPEG数据获取回调函数(编码和解码时都会调用)
 * @param  hjpeg: JPEG硬件句柄指针
 * @param  NbDecodedData: 本次已处理的数据长度
 * @retval 无
 * @note   当输入数据过多时，该函数会被多次调用以加载后续数据
 */
void HAL_JPEG_GetDataCallback(JPEG_HandleTypeDef *hjpeg,
                              uint32_t NbDecodedData) {
  uint32_t inDataLength;

  // 更新已处理数据长度
  Input_frameIndex += NbDecodedData;

  // 如果还有未处理的数据
  if (Input_frameIndex < Input_frameSize) {
    // 计算下一段数据的源地址
    JPEGSourceAddress = JPEGSourceAddress + NbDecodedData;

    // 根据是编码还是解码选择合适的CHUNK_SIZE
    uint32_t chunkSize;
    if (Jpeg_IsEncoding) {
      chunkSize = CHUNK_SIZE_ENCODE_IN;
    } else {
      chunkSize = CHUNK_SIZE_IN;
    }

    // 计算本次应读取的数据长度
    if ((Input_frameSize - Input_frameIndex) >= chunkSize) {
      inDataLength = chunkSize;
    } else {
      inDataLength = Input_frameSize - Input_frameIndex;
    }
  } else {
    inDataLength = 0;
  }

  // 配置下一段输入缓冲数据
  HAL_JPEG_ConfigInputBuffer(hjpeg, (uint8_t *)JPEGSourceAddress, inDataLength);
}

/**
 * @brief  JPEG输出数据就绪回调函数
 * @param  hjpeg: JPEG硬件句柄指针
 * @param  pDataOut: 输出缓冲区地址指针
 * @param  OutDataLength: 本次输出数据长度
 * @retval 无
 * @note   当输出数据达到CHUNK_SIZE时，该函数被调用以更新输出缓冲地址
 */
void HAL_JPEG_DataReadyCallback(JPEG_HandleTypeDef *hjpeg, uint8_t *pDataOut,
                                uint32_t OutDataLength) {
  // 更新输出缓冲区地址和大小
  FrameBufferAddress += OutDataLength;
  Output_frameSize += OutDataLength;

  // 根据是编码还是解码选择合适的CHUNK_SIZE
  uint32_t chunkSize;
  if (Jpeg_IsEncoding) {
    chunkSize = CHUNK_SIZE_ENCODE_OUT;
  } else {
    chunkSize = CHUNK_SIZE_OUT;
  }

  // 配置下一段输出缓冲区
  HAL_JPEG_ConfigOutputBuffer(hjpeg, (uint8_t *)FrameBufferAddress, chunkSize);
}

/**
 * @brief  JPEG硬件错误回调函数
 * @param  hjpeg: JPEG硬件句柄指针
 * @retval 无
 * @note   当JPEG操作过程中发生错误时触发
 */
void HAL_JPEG_ErrorCallback(JPEG_HandleTypeDef *hjpeg) {
  // 设置操作完成标志以退出等待循环
  Jpeg_HWOperationState = JPEG_OpComplete;

  // 判断错误类型
  if (HAL_JPEG_GetError(hjpeg) == HAL_JPEG_ERROR_DMA) {
    // DMA数据传输错误
    DEBUG_ERROR("DMA传输错误");
  }

  // 打印错误信息
  DEBUG_ERROR("JPEG硬件错误");
}

/**
 * @brief  JPEG硬件解码完成回调函数
 * @param  hjpeg: JPEG硬件句柄指针
 * @retval 无
 * @note   当JPEG图片完全解码后，该函数被调用以标记完成状态
 */
void HAL_JPEG_DecodeCpltCallback(JPEG_HandleTypeDef *hjpeg) {
  // 设置操作完成标志
  Jpeg_HWOperationState = JPEG_OpComplete;
}

/**
 * @brief  JPEG硬件编码完成回调函数
 * @param  hjpeg: JPEG硬件句柄指针
 * @retval 无
 * @note   当JPEG图片完全编码后，该函数被调用以标记完成状态
 */
void HAL_JPEG_EncodeCpltCallback(JPEG_HandleTypeDef *hjpeg) {
  // 设置操作完成标志
  Jpeg_HWOperationState = JPEG_OpComplete;
}

#endif
