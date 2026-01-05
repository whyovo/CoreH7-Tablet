/**
 ******************************************************************************
 * @file    dcmi_ov5640.c
 * @author  菜菜why(B站:菜菜whyy)
 * @brief   OV5640 摄像头 DCMI 驱动实现文件
 ******************************************************************************
 * @attention
 *
 * 实现说明：
 * 1. 通过 SCCB (类似I2C) 接口配置 OV5640 摄像头参数
 * 2. 使用 DCMI 接口采集摄像头数据，支持连续模式和快照模式
 * 3. DMA 循环传输摄像头数据到指定缓冲区
 * 4. 支持图像裁剪、缩放、特效等功能
 * 5. 内置自动对焦（AF）模块，需下载固件到片内 MCU
 *
 * 数据流程：
 * OV5640 摄像头 <- DCMI 采集 <- DMA 传输 -> 缓冲区 -> 应用层处理
 *
 ******************************************************************************
 */

#include "dcmi_ov5640.h"
#include "dcmi_ov5640_cfg.h"
#include "ov5640_lcd_rgb.h"

#ifdef OV5640_ENABLE

extern DCMI_HandleTypeDef hdcmi;
extern DMA_HandleTypeDef hdma_dcmi;

volatile uint8_t OV5640_FrameState = 0;
volatile uint8_t OV5640_FPS;
// 变焦等级变量声明
extern volatile uint8_t OV5640_ZoomLevel;
/*******************************************************************************
 *                              GPIO 初始化
 ******************************************************************************/

/**
 * @brief  初始化 OV5640 的控制引脚 (PWDN)
 * @note   PWDN 引脚用于控制摄像头电源，高电平进入掉电模式，低电平正常工作
 * @retval None
 */
void DCMI_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	GPIO_OV5640_PWDN_CLK_ENABLE; /*!< 使能 PWDN 引脚的 GPIO 时钟 */

	/* 初始化 PWDN 引脚 */
	OV5640_PWDN_ON; /*!< 高电平，进入掉电模式 */

	GPIO_InitStruct.Pin = OV5640_PWDN_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;	 /*!< 推挽输出模式 */
	GPIO_InitStruct.Pull = GPIO_PULLUP;			 /*!< 上拉 */
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; /*!< 低速 */
	HAL_GPIO_Init(OV5640_PWDN_PORT, &GPIO_InitStruct);
}

/**
 * @brief  初始SCCB、DCMI、DMA以及配置OV5640
 * @param  output_width:  期望输出的图像宽度
 * @param  output_height: 期望输出的图像高度
 * @retval OV5640_Success: 初始化成功，OV5640_Error: 初始化失败
 */
int8_t DCMI_OV5640_Init(void)
{
	uint16_t Device_ID;

	SCCB_GPIO_Config();
	DCMI_GPIO_Init();
	OV5640_Reset();
	Device_ID = OV5640_ReadID();

	if (Device_ID == 0x5640)
	{
		DEBUG_INFO("OV5640 OK, ID:0x%X", Device_ID);
		DEBUG_INFO("Crop Size: %d x %d", OV5640_CROP_WIDTH, OV5640_CROP_HEIGHT);

		OV5640_Config();
		OV5640_Set_Framesize(OV5640_Width, OV5640_Height); // 设置 ISP 输出大小

		// 使用宏定义的宽高进行裁剪
		OV5640_DCMI_Crop(OV5640_CROP_WIDTH, OV5640_CROP_HEIGHT, OV5640_Width, OV5640_Height);

		return OV5640_Success;
	}
	else
	{
		DEBUG_ERROR("OV5640 ERROR! ID:%X", Device_ID);
		return OV5640_Error;
	}
}

/**
 * @brief  启动DMA传输，连续模式
 * @param  DMA_Buffer: DMA将要传输的地址，即用于存储摄像头数据的存储区地址
 * @param  DMA_BufferSize: 传输的数据大小，32位宽
 * @note   1. 开启连续模式之后，会一直进行传输，除非挂起或者停止DCMI
 *         2. OV5640使用RGB565模式时，1个像素点需要2个字节来存储
 *         3. 因为DMA配置传输数据为32位宽，计算 DMA_BufferSize 时，需要除以4，例如：
 *            要获取 240*240分辨率 的图像，需要传输 240*240*2 = 115200 字节的数据，
 *            则 DMA_BufferSize = 115200 / 4 = 28800 。
 * @retval None
 */
void OV5640_DMA_Transmit_Continuous(uint32_t DMA_Buffer, uint32_t DMA_BufferSize)
{
	hdma_dcmi.Init.Mode = DMA_CIRCULAR; /*!< 循环模式 */
	HAL_DMA_Init(&hdma_dcmi);			/*!< 初始化 DMA */

	/* 启动 DCMI DMA 采集（连续模式）*/
	HAL_DCMI_Start_DMA(&hdcmi, DCMI_MODE_CONTINUOUS, (uint32_t)DMA_Buffer, DMA_BufferSize);
}

/**
 * @brief  启动 DMA 快照模式（单帧采集）
 * @param  DMA_Buffer: DMA 缓冲区起始地址
 * @param  DMA_BufferSize: 传输大小（32位宽，需除以4）
 * @note   传输一帧数据后自动停止，需调用 OV5640_DCMI_Resume() 恢复
 * @retval None
 */
void OV5640_DMA_Transmit_Snapshot(uint32_t DMA_Buffer, uint32_t DMA_BufferSize)
{
	hdma_dcmi.Init.Mode = DMA_NORMAL; /*!< 单次模式 */
	HAL_DMA_Init(&hdma_dcmi);		  /*!< 初始化 DMA */

	/* 启动 DCMI DMA 采集（快照模式）*/
	HAL_DCMI_Start_DMA(&hdcmi, DCMI_MODE_SNAPSHOT, (uint32_t)DMA_Buffer, DMA_BufferSize);
}

/**
 * @brief  挂起DCMI，停止捕获数据
 * @note   1. 开启连续模式之后，再调用该函数，会停止捕获DCMI的数据
 *         2. 可以调用 OV5640_DCMI_Resume() 恢复DCMI
 *         3. 强制操作寄存器，确保 DMA 和 DCMI 真正停止
 * @retval None
 */
void OV5640_DCMI_Suspend(void)
{
	// 1. 强制关闭 DCMI 捕获 (停止产生 DMA 请求)
	// 直接操作寄存器，绕过 HAL 库的状态检查
	hdcmi.Instance->CR &= ~(DCMI_CR_CAPTURE);

	// 2. 等待 DCMI 硬件位清零 (DCMI 会在当前帧结束后停止)
	// while(hdcmi.Instance->CR & DCMI_CR_CAPTURE);

	// 3. 强制关闭 DMA Stream (确保总线完全释放，防止 DMA 抢占 SD 卡读写)
	__HAL_DMA_DISABLE(&hdma_dcmi);
}

/**
 * @brief  恢复DCMI，开始捕获数据
 * @note   1. 当DCMI被挂起时，可以调用该函数恢复
 *         2. 恢复 DMA 和 DCMI 捕获
 * @retval None
 */
void OV5640_DCMI_Resume(void)
{
	// 1. 重新使能 DCMI 外设
	hdcmi.Instance->CR |= DCMI_CR_ENABLE;

#if defined(LCD_RGB_ENABLE)
	// 2. 调用 RGB 屏启动函数
	// 该函数内部必须包含：配置 DMA 地址/长度、使能 DMA、设置 DCMI_CR_CAPTURE
	OV5640_RGB_Start();
#else
	// 2. 恢复 DMA Stream
	__HAL_DMA_ENABLE(&hdma_dcmi);

	// 3. 恢复 DCMI 捕获
	hdcmi.Instance->CR |= DCMI_CR_CAPTURE;
#endif
}

/**
 * @brief  禁止DCMI的DMA请求，停止DCMI捕获，禁止DCMI外设
 * @retval None
 */
void OV5640_DCMI_Stop(void)
{
	HAL_DCMI_Stop(&hdcmi);
}

/**
 * @brief  使用DCMI的裁剪功能，将传感器输出的图像裁剪成适应屏幕的大小
 * @param  Displey_XSize: 显示器的宽度
 * @param  Displey_YSize: 显示器的高度
 * @param  Sensor_XSize: 摄像头传感器输出图像的宽度
 * @param  Sensor_YSize: 摄像头传感器输出图像的高度
 * @note   1. 因为摄像头输出的画面比例不一定匹配显示器，所以需要裁剪
 *         2. 摄像头的输出画面比例由 OV5640_Config()配置参数决定，最终画面大小由 OV5640_Set_Framesize()决定
 *         3. DCMI的水平有效像素也必须要能被4整除！
 *         4. 函数会计算水平和垂直偏移，尽量让画面居中裁剪
 * @retval OV5640_Success: 成功，OV5640_Error: 失败
 */
int8_t OV5640_DCMI_Crop(uint16_t Displey_XSize, uint16_t Displey_YSize, uint16_t Sensor_XSize, uint16_t Sensor_YSize)
{
	uint16_t DCMI_X_Offset, DCMI_Y_Offset; // 水平和垂直偏移，垂直代表的是行数，水平代表的是像素时钟数（PCLK周期数）
	uint16_t DCMI_CAPCNT;				   // 水平有效像素，代表的是像素时钟数（PCLK周期数）
	uint16_t DCMI_VLINE;				   // 垂直有效行数

	if ((Displey_XSize >= Sensor_XSize) || (Displey_YSize >= Sensor_YSize))
	{
		//		DEBUG_INFO("实际显示的尺寸大于或等于摄像头输出的尺寸，退出DCMI裁剪\r\n");
		return OV5640_Error; // 如果实际显示的尺寸大于或等于摄像头输出的尺寸，则退出当前函数，不进行裁剪
	}
	// 在设置为RGB565格式时，水平偏移，必须是奇数，否则画面色彩不正确，
	// 因为一个有效像素是2个字节，需要2个PCLK周期，所以必须从奇数位开始，不然数据会错乱，
	// 需要注意的是，寄存器值是从0开始算起的	！
	DCMI_X_Offset = Sensor_XSize - Displey_XSize; // 实际计算过程为（Sensor_XSize - LCD_XSize）/2*2

	// 计算垂直偏移，尽量让画面居中裁剪，该值代表的是行数，
	DCMI_Y_Offset = (Sensor_YSize - Displey_YSize) / 2 - 1; // 寄存器值是从0开始算起的，所以要-1

	// 因为一个有效像素是2个字节，需要2个PCLK周期，所以要乘2
	// 最终得到的寄存器值，必须要能被4整除！
	DCMI_CAPCNT = Displey_XSize * 2 - 1; // 寄存器值是从0开始算起的，所以要-1

	DCMI_VLINE = Displey_YSize - 1; // 垂直有效行数

	//	DEBUG_INFO("%d  %d  %d  %d\r\n",DCMI_X_Offset,DCMI_Y_Offset,DCMI_CAPCNT,DCMI_VLINE);

	HAL_DCMI_ConfigCrop(&hdcmi, DCMI_X_Offset, DCMI_Y_Offset, DCMI_CAPCNT, DCMI_VLINE); // 设置裁剪窗口
	HAL_DCMI_EnableCrop(&hdcmi);														// 使能裁剪

	return OV5640_Success;
}

/**
 * @brief  执行软件复位
 * @note   期间有多个延时操作
 * @retval None
 */
void OV5640_Reset(void)
{
	HAL_Delay(30); // 等待模块上电稳定，最少5ms，然后拉低PWDN

	OV5640_PWDN_OFF; // PWDN 引脚输出低电平，不开启掉电模式，摄像头正常工作，此时摄像头模块的白色LED会点亮

	// 根据OV5640的上电时序，PWDN拉低之后，要等待1ms再去拉高RESET
	// 因此加入延时，等待硬件复位完成并稳定下来
	HAL_Delay(5);

	// 复位完成之后，要>=20ms方可执行SCCB配置
	HAL_Delay(20);

	SCCB_WriteReg_16Bit(0x3103, 0x11); // 根据手册的建议，复位之前，直接将时钟输入引脚的时钟作为主时钟
	SCCB_WriteReg_16Bit(0x3008, 0x82); // 执行一次软复位
	HAL_Delay(5);					   // 延时5ms
}

/**
 * @brief  读取 OV5640 的器件ID
 * @retval 器件ID
 */
uint16_t OV5640_ReadID(void)
{
	uint8_t PID_H, PID_L; // ID变量

	PID_H = SCCB_ReadReg_16Bit(OV5640_ChipID_H); // 读取ID高字节
	PID_L = SCCB_ReadReg_16Bit(OV5640_ChipID_L); // 读取ID低字节

	return (PID_H << 8) | PID_L; // 返回完整的器件ID
}

/**
 * @brief  配置 OV5640 各个寄存器参数
 * @note   参数定义在 dcmi_ov5640_cfg.h
 * @retval None
 */
void OV5640_Config(void)
{
	uint32_t i; // 计数变量

	//	uint8_t	Verify_Reg; // 读取配置，用于调试

	for (i = 0; i < (sizeof(OV5640_INIT_Config) / 4); i++)
	{
		SCCB_WriteReg_16Bit(OV5640_INIT_Config[i][0], OV5640_INIT_Config[i][1]); // 写入配置

		HAL_Delay(1);

		//		Verify_Reg = SCCB_ReadReg_16Bit(OV5640_INIT_Config[i][0]);	// 读取配置，用于调试

		//		if( OV5640_INIT_Config[i][1] != Verify_Reg )	// 配置不成功
		//		{
		//			DEBUG_INFO("出错位置：%d\r\n",i);	// 打印出错位置
		//			DEBUG_INFO("0x%x-0x%x-0x%x\r\n",OV5640_INIT_Config[i][0],OV5640_INIT_Config[i][1],Verify_Reg);
		//		}
	}
}

/**
 * @brief  设置输出的像素格式
 * @param  pixformat: 像素格式，可选择 Pixformat_RGB565、Pixformat_GRAY、Pixformat_JPEG
 * @retval None
 */
void OV5640_Set_Pixformat(uint8_t pixformat)
{
	uint8_t OV5640_Reg; // 寄存器的值

	if (pixformat == Pixformat_JPEG)
	{
		SCCB_WriteReg_16Bit(OV5640_FORMAT_CONTROL, 0x30);	  //	设置数据接口输出的格式
		SCCB_WriteReg_16Bit(OV5640_FORMAT_CONTROL_MUX, 0x00); // 设置ISP的格式

		SCCB_WriteReg_16Bit(OV5640_JPEG_MODE_SELECT, 0x02); // JPEG 模式2

		SCCB_WriteReg_16Bit(OV5640_JPEG_VFIFO_CTRL00, 0xA0); // JPEG 固定行数

		SCCB_WriteReg_16Bit(OV5640_JPEG_VFIFO_HSIZE_H, OV5640_Width >> 8);		// JPEG输出水平尺寸,高字节
		SCCB_WriteReg_16Bit(OV5640_JPEG_VFIFO_HSIZE_L, (uint8_t)OV5640_Width);	// JPEG输出水平尺寸,低字节
		SCCB_WriteReg_16Bit(OV5640_JPEG_VFIFO_VSIZE_H, OV5640_Height >> 8);		// JPEG输出垂直尺寸,低字节
		SCCB_WriteReg_16Bit(OV5640_JPEG_VFIFO_VSIZE_L, (uint8_t)OV5640_Height); // JPEG输出垂直尺寸,低字节
	}
	else if (pixformat == Pixformat_GRAY)
	{
		SCCB_WriteReg_16Bit(OV5640_FORMAT_CONTROL, 0x10);	  //	设置数据接口输出的格式
		SCCB_WriteReg_16Bit(OV5640_FORMAT_CONTROL_MUX, 0x00); // 设置ISP的格式
	}
	else // RGB565
	{
		SCCB_WriteReg_16Bit(OV5640_FORMAT_CONTROL, 0x6F);	  // 此处设置为RGB565格式，序列为 G[2:0]B[4:0], R[4:0]G[5:3]
		SCCB_WriteReg_16Bit(OV5640_FORMAT_CONTROL_MUX, 0x01); // 设置ISP的格式
	}

	OV5640_Reg = SCCB_ReadReg_16Bit(0x3821); // 读取寄存器值，Bit[5]用于是否使能JPEG模式
	SCCB_WriteReg_16Bit(0x3821, (OV5640_Reg & 0xDF) | ((pixformat == Pixformat_JPEG) ? 0x20 : 0x00));

	OV5640_Reg = SCCB_ReadReg_16Bit(0x3002); // 读取寄存器值，Bit[7]、Bit[4]和Bit[2]使能 VFIFO、JFIFO、JPG
	SCCB_WriteReg_16Bit(0x3002, (OV5640_Reg & 0xE3) | ((pixformat == Pixformat_JPEG) ? 0x00 : 0x1C));

	OV5640_Reg = SCCB_ReadReg_16Bit(0x3006); // 读取寄存器值，Bit[5]和Bit[3] 用于是否使能JPG时钟
	SCCB_WriteReg_16Bit(0x3006, (OV5640_Reg & 0xD7) | ((pixformat == Pixformat_JPEG) ? 0x28 : 0x00));
}

/**
 * @brief  设置JPEG压缩等级
 * @param  scale: 压缩等级，取值 0x01~0x3F
 * @note   数值越大，压缩就越厉害，得到的图片占用空间就越小，但相应的画质会变差，客户可自行调节
 * @retval None
 */
void OV5640_Set_JPEG_QuantizationScale(uint8_t scale)
{
	SCCB_WriteReg_16Bit(0x4407, scale); // JPEG 压缩等级
}

/**
 * @brief  设置实际输出的图像大小（缩放后）
 * @param  width: 实际输出图像的长度
 * @param  height: 实际输出图像的宽度
 * @note   1. 需要注意的是，要设置的图像长、宽需要满足初始化配置时ISP窗口的比例，不然图像会变形
 *         2. 并不是设置输出的图像分辨率越小帧率就越高，帧率只和初始化的配置（PLL、HTS和VTS）有关
 * @retval OV5640_Success: 成功，OV5640_Error: 失败
 */
int8_t OV5640_Set_Framesize(uint16_t width, uint16_t height)
{
	// OV5640的很多操作，都要加上这种对应 group 的配置
	SCCB_WriteReg_16Bit(OV5640_GroupAccess, 0X03); // 开始 group 3 的配置

	SCCB_WriteReg_16Bit(OV5640_TIMING_DVPHO_H, width >> 8); // DVPHO，设置输出水平尺寸
	SCCB_WriteReg_16Bit(OV5640_TIMING_DVPHO_L, width & 0xff);
	SCCB_WriteReg_16Bit(OV5640_TIMING_DVPVO_H, height >> 8); // DVPVO，设置输出垂直尺寸
	SCCB_WriteReg_16Bit(OV5640_TIMING_DVPVO_L, height & 0xff);

	SCCB_WriteReg_16Bit(OV5640_GroupAccess, 0X13); // 结束配置
	SCCB_WriteReg_16Bit(OV5640_GroupAccess, 0Xa3); // 启用设置

	return OV5640_Success;
}

/**
 * @brief  用于设置输出的图像是否进行水平镜像
 * @param  ConfigState: 置1时，图像会水平镜像，置0时恢复正常
 * @retval OV5640_Success: 成功，OV5640_Error: 失败
 */
int8_t OV5640_Set_Horizontal_Mirror(int8_t ConfigState)
{
	uint8_t OV5640_Reg; // 寄存器的值

	OV5640_Reg = SCCB_ReadReg_16Bit(OV5640_TIMING_Mirror); // 读取寄存器值

	// Bit[2:1]用于设置是否水平镜像
	if (ConfigState == OV5640_Enable) // 如果使能镜像
	{
		OV5640_Reg |= 0X06;
	}
	else // 取消镜像
	{
		OV5640_Reg &= 0xF9;
	}
	return SCCB_WriteReg_16Bit(OV5640_TIMING_Mirror, OV5640_Reg); // 写入寄存器
}

/**
 * @brief  用于设置输出的图像是否进行垂直翻转
 * @param  ConfigState: 置1时，图像会垂直翻转，置0时恢复正常
 * @retval OV5640_Success: 成功，OV5640_Error: 失败
 */
int8_t OV5640_Set_Vertical_Flip(int8_t ConfigState)
{
	uint8_t OV5640_Reg; // 寄存器的值

	OV5640_Reg = SCCB_ReadReg_16Bit(OV5640_TIMING_Flip); // 读取寄存器值

	// Bit[2:1]用于设置是否垂直翻转
	if (ConfigState == OV5640_Enable)
	{
		OV5640_Reg |= 0X06;
	}
	else // 取消翻转
	{
		OV5640_Reg &= 0xF9;
	}
	return SCCB_WriteReg_16Bit(OV5640_TIMING_Flip, OV5640_Reg); // 写入寄存器
}

/**
 * @brief  设置亮度
 * @param  Brightness: 亮度，可设置为9个等级：4，3，2，1，0，-1，-2，-3，-4   ，数字越大亮度越高
 * @note   1. 直接使用OV5640手册给出的代码
 *         2. 亮度越高，画面就越明亮，但是会变模糊一些
 *         3. 亮度太低，噪点会增多
 * @retval None
 */
void OV5640_Set_Brightness(int8_t Brightness)
{
	Brightness = Brightness + 4;
	SCCB_WriteReg_16Bit(OV5640_GroupAccess, 0X03); // 开始 group 3 的配置

	SCCB_WriteReg_16Bit(0x5587, OV5640_Brightness_Config[Brightness][0]);
	SCCB_WriteReg_16Bit(0x5588, OV5640_Brightness_Config[Brightness][1]);

	SCCB_WriteReg_16Bit(OV5640_GroupAccess, 0X13); // 结束配置
	SCCB_WriteReg_16Bit(OV5640_GroupAccess, 0Xa3); // 启用设置
}

/**
 * @brief  设置对比度
 * @param  Contrast: 对比度，可设置为7个等级：3，2，1，0，-1，-2 ，-3
 * @note   1. 直接使用OV5640手册给出的代码
 *         2. 对比度越高，画面越清晰，黑白越加分明
 * @retval None
 */
void OV5640_Set_Contrast(int8_t Contrast)
{
	Contrast = Contrast + 3;
	SCCB_WriteReg_16Bit(OV5640_GroupAccess, 0X03); // 开始 group 3 的配置

	SCCB_WriteReg_16Bit(0x5586, OV5640_Contrast_Config[Contrast][0]);
	SCCB_WriteReg_16Bit(0x5585, OV5640_Contrast_Config[Contrast][1]);

	SCCB_WriteReg_16Bit(OV5640_GroupAccess, 0X13); // 结束配置
	SCCB_WriteReg_16Bit(OV5640_GroupAccess, 0Xa3); // 启用设置
}

/**
 * @brief  用于设置OV5640的特效，正常、负片、黑白、正负片叠加模式
 * @param  effect_Mode: 特效模式，可选择参数 OV5640_Effect_Normal、OV5640_Effect_Negative、OV5640_Effect_BW、OV5640_Effect_Solarize
 * @note   这里仅列举了4个模式，更多特效模式可以参考手册进行配置
 * @retval None
 */
void OV5640_Set_Effect(uint8_t effect_Mode)
{
	SCCB_WriteReg_16Bit(OV5640_GroupAccess, 0X03); // 开始 group 3 的配置

	SCCB_WriteReg_16Bit(0x5580, OV5640_Effect_Config[effect_Mode][0]);
	SCCB_WriteReg_16Bit(0x5583, OV5640_Effect_Config[effect_Mode][1]);
	SCCB_WriteReg_16Bit(0x5584, OV5640_Effect_Config[effect_Mode][2]);
	SCCB_WriteReg_16Bit(0x5003, OV5640_Effect_Config[effect_Mode][3]);

	SCCB_WriteReg_16Bit(OV5640_GroupAccess, 0X13); // 结束配置
	SCCB_WriteReg_16Bit(OV5640_GroupAccess, 0Xa3); // 启用设置
}

/**
 * @brief  将自动对焦固件写入OV5640
 * @note   因为OV5640片内没有flash，不能保存固件，因此每次上电都要写入一次
 * @retval OV5640_Success: 成功，OV5640_Error: 失败
 */
int8_t OV5640_AF_Download_Firmware(void)
{
	uint8_t AF_Status = 0;			   // 对焦状态
	uint16_t i = 0;					   // 计数变量
	uint16_t OV5640_MCU_Addr = 0x8000; // OV5640 MCU 存储器的起始地址为 0x8000，大小为4KB

	SCCB_WriteReg_16Bit(0x3000, 0x20); // Bit[5]，复位MCU，写入固件之前，需要执行此操作
									   // 开始写入固件，批量写入，提高写入速度
	SCCB_WriteBuffer_16Bit(OV5640_MCU_Addr, (uint8_t *)OV5640_AF_Firmware, sizeof(OV5640_AF_Firmware));
	SCCB_WriteReg_16Bit(0x3000, 0x00); // Bit[5]，写入完毕，写0使能MCU

	// 写入固件之后，会有个初始化的过程，因此尝试读取100次状态，根据状态进行判断
	for (i = 0; i < 100; i++)
	{
		AF_Status = SCCB_ReadReg_16Bit(OV5640_AF_FW_STATUS); // 读取状态寄存器
		if (AF_Status == 0x7E)
		{
			DEBUG_INFO("AF固件初始化中>>>\r\n");
		}
		if (AF_Status == 0x70) // 释放马达，镜头回到初始（对焦为无穷远处）位置，意味着固件写入成功
		{
			DEBUG_INFO("AF固件写入成功！\r\n");
			return OV5640_Success;
		}
	}
	// 尝试100次读取之后，还是没有读到0x70状态，说明固件没写入成功
	DEBUG_INFO("自动对焦固件写入失败！！！error！！\r\n");
	return OV5640_Error;
}

/**
 * @brief  对焦状态查询
 * @note   1. 对焦过程大概会持续500多ms
 *         2. 对焦没完成时，采集到的的图像不在焦点，会非常模糊
 * @retval OV5640_AF_End: 对焦结束， OV5640_AF_Focusing: 正在对焦
 */
int8_t OV5640_AF_QueryStatus(void)
{
	uint8_t AF_Status = 0; // 对焦状态

	AF_Status = SCCB_ReadReg_16Bit(OV5640_AF_FW_STATUS); // 读取状态寄存器
	DEBUG_INFO("AF_Status:0x%x\r\n", AF_Status);

	// 单次对焦模式	下，返回 0x10，持续对焦模式下，返回0x20
	if ((AF_Status == 0x10) || (AF_Status == 0x20))
	{
		return OV5640_AF_End; // 返回 对焦结束 标志
	}
	else
	{
		return OV5640_AF_Focusing; // 返回 正在对焦 标志
	}
}

/**
 * @brief  持续触发对焦，当OV5640检测到当前画面不在焦点时，会一直触发对焦，无需用户干预
 * @note   1.可以调用 OV5640_AF_QueryStatus() 函数查询对焦状态
 *         2.可以调用 OV5640_AF_Release() 退出持续对焦模式
 *         3.对焦过程大概会持续500多ms
 *         4.有时环境光线太暗，OV5640会反复的进行对焦，用户可根据实际情况切换到单次对焦模式
 * @retval None
 */
void OV5640_AF_Trigger_Constant(void)
{
	SCCB_WriteReg_16Bit(0x3022, 0x04); //	持续对焦
}

/**
 * @brief  触发一次自动对焦
 * @note   对焦过程大概会持续500多ms，用户可以调用 OV5640_AF_QueryStatus() 函数查询对焦状态
 * @retval None
 */
void OV5640_AF_Trigger_Single(void)
{
	SCCB_WriteReg_16Bit(OV5640_AF_CMD_MAIN, 0x03); // 触发一次自动对焦
}

/**
 * @brief  释放马达，镜头回到初始（对焦为无穷远处）位置
 * @retval None
 */
void OV5640_AF_Release(void)
{
	SCCB_WriteReg_16Bit(OV5640_AF_CMD_MAIN, 0x08); // 对焦释放指令
}

/**
 * @brief  帧回调函数，每传输一帧数据，会进入该中断服务函数
 * @note   每次传输完一帧，对相应的标志位进行操作，并计算帧率
 * @retval None
 */
void HAL_DCMI_FrameEventCallback(DCMI_HandleTypeDef *hdcmi)
{
	static uint32_t DCMI_Tick = 0;		 // 用于保存当前的时间计数值
	static uint8_t DCMI_Frame_Count = 0; // 帧数计数

	if (HAL_GetTick() - DCMI_Tick >= 1000) // 每隔 1s 计算一次帧率
	{
		DCMI_Tick = HAL_GetTick(); // 重新获取当前时间计数值

		OV5640_FPS = DCMI_Frame_Count; // 获得fps

		DCMI_Frame_Count = 0; // 计数清0
	}
	DCMI_Frame_Count++; // 每进入一次中断（每次传输完一帧数据），计数值+1

	OV5640_FrameState = 1; // 传输完成标志位置1
}

/**
 * @brief  错误回调函数
 * @note   当发生DMA传输错误或者FIFO溢出错误就会进入
 * @retval None
 */
void HAL_DCMI_ErrorCallback(DCMI_HandleTypeDef *hdcmi)
{

	DEBUG_ERROR("DCMI Error Code: 0x%x", HAL_DCMI_GetError(hdcmi));

	if (HAL_DCMI_GetError(hdcmi) == HAL_DCMI_ERROR_OVR)
	{
		DEBUG_ERROR("FIFO Overflow! (DMA too slow or bandwidth full)");
	}
	if (HAL_DCMI_GetError(hdcmi) == HAL_DCMI_ERROR_SYNC)
	{
		DEBUG_ERROR("Synchronization Error! (Check VSYNC/HSYNC pins)");
	}
}

/**
 * @brief  设置数码变焦等级
 * @param  zoom_percent: 0~100 (0为最广角，100为最大变焦)
 */
void OV5640_Set_Zoom(uint8_t zoom_percent)
{
	if (zoom_percent > 100)
		zoom_percent = 100;
	OV5640_ZoomLevel = zoom_percent;
}

#endif
