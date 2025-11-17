/**
 ******************************************************************************
 * @file    dmic.h
 * @author  菜菜why(B站:菜菜whyy)
 * @brief   INMP441 数字麦克风 I2S DMA 驱动头文件
 ******************************************************************************
 * @attention
 *
 * 使用方法:
 * - 使用 I2S 主机接收模式 + DMA 双缓冲实现连续音频采集
 * - 调用 DMIC_Init() 初始化并启动 DMA 接收
 * - 在 DMA 半传输/全传输中断回调中处理音频数据
 *
 * 使用示例:
    void DMIC_DataReadyCallback(int32_t *pData, uint32_t size, uint8_t isHalfBuffer)
    {
        // pData: 指向就绪数据的指针
        // size: 数据长度（采样点数）
        // isHalfBuffer: 1=前半缓冲就绪, 0=后半缓冲就绪

        // 进行FFT、音量检测等处理
        for (uint32_t i = 0; i < size; i++) {
            // 处理 pData[i]
        }
    }
 *
 * INMP441 引脚连接:
 * - SCK  -> I2S_CK   (时钟)
 * - WS   -> I2S_WS   (字选择/左右声道)
 * - SD   -> I2S_SDI  (数据输入)
 * - L/R  -> GND      (左声道) 或 VCC (右声道)
 * - VDD  -> 3.3V
 * - GND  -> GND
 *
 * WAV录制使用示例（注意：默认保存sd卡，一定要开启sdmmc和fatfs！）:
 *   // 开始录制到"0:my_recording.wav"
 *   DMIC_StartRecord("0:my_recording.wav");
 *
 *   // ... 录音过程 ...延时
 *
 *   // 停止录制
 *   DMIC_StopRecord();
 *
 ******************************************************************************
 */

#ifndef DMIC_H
#define DMIC_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "init.h"
#include <stdint.h>

/*******************************************************************************
 *                              I2S 外设配置
 ******************************************************************************/
#define DMIC_I2S_INSTANCE SPI1 /*!< I2S外设实例（使用SPI1作为I2S） */
#define DMIC_I2S_HANDLE hi2s1  /*!< I2S句柄（在i2s.c中定义） */

/*******************************************************************************
 *                              引脚配置
 ******************************************************************************/
#define DMIC_WS_PORT GPIOG       /*!< WS引脚端口 */
#define DMIC_WS_PIN GPIO_PIN_10  /*!< WS引脚（字选择） */
#define DMIC_WS_AF GPIO_AF5_SPI1 /*!< WS复用功能 */

#define DMIC_SDI_PORT GPIOG       /*!< SDI引脚端口 */
#define DMIC_SDI_PIN GPIO_PIN_9   /*!< SDI引脚（数据输入） */
#define DMIC_SDI_AF GPIO_AF5_SPI1 /*!< SDI复用功能 */

#define DMIC_CK_PORT GPIOG       /*!< CK引脚端口 */
#define DMIC_CK_PIN GPIO_PIN_11  /*!< CK引脚（时钟） */
#define DMIC_CK_AF GPIO_AF5_SPI1 /*!< CK复用功能 */

/*******************************************************************************
 *                              DMA 配置
 ******************************************************************************/
#define DMIC_DMA_INSTANCE DMA1_Stream0 /*!< DMA流实例 */

/*******************************************************************************
 *                              音频参数配置
 ******************************************************************************/
#define DMIC_BUFFER_SIZE 2048   /*!< DMA缓冲区大小（采样点数，必须是2的幂次） */
#define DMIC_SAMPLE_RATE 32000  /*!< 采样率（Hz） */
#define DMIC_CHANNELS 1         /*!< 声道数（单声道） */
#define DMIC_BITS_PER_SAMPLE 16 /*!< 每个采样的位数（16bit） */

/*******************************************************************************
 *                              WAV录制配置
 ******************************************************************************/
#define DMIC_MAX_FILENAME_LEN 256 /*!< 最大文件名长度 */
#define DMIC_TIMESTAMP_BUF_LEN 32 /*!< 时间戳缓冲区长度 */

    /*******************************************************************************
     *                              导出类型
     ******************************************************************************/

    /**
     * @brief  麦克风状态枚举
     */
    typedef enum
    {
        DMIC_STATE_RESET = 0, /*!< 未初始化状态 */
        DMIC_STATE_READY,     /*!< 就绪状态（已初始化但未启动） */
        DMIC_STATE_RECORDING, /*!< 录音中（DMA运行中） */
        DMIC_STATE_ERROR      /*!< 错误状态 */
    } DMIC_State;

    /**
     * @brief  WAV录制状态枚举
     */
    typedef enum
    {
        DMIC_RECORD_IDLE = 0, /*!< 未录制 */
        DMIC_RECORD_RUNNING,  /*!< 录制中 */
        DMIC_RECORD_ERROR     /*!< 录制错误 */
    } DMIC_RecordState;

    /**
     * @brief  数据就绪回调函数原型
     * @param  pData: 指向就绪数据的指针
     * @param  size: 数据长度（采样点数）
     * @param  isHalfBuffer: 1=前半缓冲就绪, 0=后半缓冲就绪
     * @note   在DMA中断中调用，应快速处理或复制数据
     */
    void DMIC_DataReadyCallback(int32_t *pData, uint32_t size, uint8_t isHalfBuffer);

    /**
     * @brief  数据就绪回调函数原型（类型别名）
     * @param  pData: 指向就绪数据的指针
     * @param  size: 数据长度（采样点数）
     * @param  isHalfBuffer: 1=前半缓冲就绪, 0=后半缓冲就绪
     * @note   在DMA中断中调用，应快速处理或复制数据
     */
    typedef void (*DMIC_DataReadyCallback_t)(int32_t *pData, uint32_t size, uint8_t isHalfBuffer);

    /*******************************************************************************
     *                              导出函数
     ******************************************************************************/

    /**
     * @brief  初始化数字麦克风（I2S + DMA双缓冲）
     * @note   会自动启动DMA接收，开始连续采集
     * @retval HAL_StatusTypeDef: HAL_OK=成功, 其他=失败
     */
    HAL_StatusTypeDef DMIC_Init(void);

    /**
     * @brief  启动麦克风录音（启动DMA接收）
     * @retval HAL_StatusTypeDef: HAL_OK=成功, 其他=失败
     */
    HAL_StatusTypeDef DMIC_Start(void);

    /**
     * @brief  停止麦克风录音（停止DMA接收）
     * @retval HAL_StatusTypeDef: HAL_OK=成功, 其他=失败
     */
    HAL_StatusTypeDef DMIC_Stop(void);

    /**
     * @brief  获取当前状态
     * @retval DMIC_State: 当前状态
     */
    DMIC_State DMIC_GetState(void);

    /**
     * @brief  注册数据就绪回调函数
     * @param  callback: 回调函数指针
     * @retval None
     */
    void DMIC_RegisterCallback(DMIC_DataReadyCallback_t callback);

    /**
     * @brief  取消回调注册
     * @retval None
     */
    void DMIC_UnregisterCallback(void);

    /**
     * @brief  DMA传输完成回调（内部使用，由HAL库调用）
     * @note   用户不应直接调用此函数
     * @retval None
     */
    void DMIC_DMA_TransferComplete_Callback(void);

    /**
     * @brief  DMA半传输完成回调（内部使用，由HAL库调用）
     * @note   用户不应直接调用此函数
     * @retval None
     */
    void DMIC_DMA_HalfTransfer_Callback(void);

    /*******************************************************************************
     *                          WAV 文件录制函数
     ******************************************************************************/

    /**
     * @brief  启动WAV文件录制
     * @param  filename: 文件名（例："0:recording.wav"），NULL则使用时间戳命名
     * @note   自动处理同名文件重复，后面添加递增数字（例：recording_1.wav）
     * @retval HAL_StatusTypeDef: HAL_OK=成功, 其他=失败
     */
    HAL_StatusTypeDef DMIC_StartRecord(const char *filename);

    /**
     * @brief  停止WAV文件录制
     * @note   会自动补齐WAV文件头信息（文件大小等）
     * @retval HAL_StatusTypeDef: HAL_OK=成功, 其他=失败
     */
    HAL_StatusTypeDef DMIC_StopRecord(void);

    /**
     * @brief  获取当前录制状态
     * @retval DMIC_RecordState: 录制状态
     */
    DMIC_RecordState DMIC_GetRecordState(void);

    /**
     * @brief  获取已录制的音频数据大小（字节）
     * @retval 音频数据大小
     */
    uint32_t DMIC_GetRecordedSize(void);

    /**
     * @brief  获取最后一次录制的文件名
     * @return 文件名指针（指向内部缓冲区）
     */
    const char *DMIC_GetLastRecordFile(void);

#ifdef __cplusplus
}
#endif

#endif // DMIC_H
