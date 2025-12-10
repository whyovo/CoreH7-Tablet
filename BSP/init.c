/**
 ******************************************************************************
 * @file    init.c
 * @author  菜菜why（B站：菜菜whyy）
 * @brief   初始化源文件，实现外设统一初始化与主循环任务调度
 ******************************************************************************
 * @attention
 *
 * 本文件实现：
 * - init_all(): 根据 init.h 中的使能开关初始化各外设模块
 * - main_while(): 周期调用各模块的非阻塞任务（如按键扫描）
 *
 * 使用示例：
 * int main(void) {
 *     HAL_Init();
 *     SystemClock_Config();
 *     MX_GPIO_Init();
 *
 *     init_all();  // 初始化所有外设
 *
 *     while (1) {
 *         main_while();  // 周期任务
 *         HAL_Delay(10); // 建议10ms调用一次
 *     }
 * }
 *
 ******************************************************************************
 */

#include "init.h"

/*******************************************************************************
 *                              初始化函数
 ******************************************************************************/

/**
 * @brief  初始化所有启用的外设
 * @note   根据 init.h 中的宏定义（LED_ENABLE / KEY_ENABLE）有条件编译
 * @note   初始化顺序：先底层驱动，再上层功能模块
 * @retval None
 */
void init_all(void) {
  /*******************************************************************************
   *                              外设初始化部分
   ******************************************************************************/

#ifdef LED_ENABLE
  LED_Init(); /* 初始化LED驱动，关闭所有LED */
#endif        /* LED_ENABLE */

#ifdef KEY_ENABLE
  KEY_Init(); /* 初始化按键驱动，读取初始电平 */
#endif        /* KEY_ENABLE */

#ifdef BUZZER_ENABLE
  BUZZER_Init(); /* 初始化蜂鸣器驱动，关闭所有蜂鸣器 */
#endif           /* BUZZER_ENABLE */

#ifdef DIGITAL_SENSOR_ENABLE
  DIGITAL_SENSOR_Init(); /* 初始化数字传感器驱动，读取初始电平 */
#endif                   /* DIGITAL_SENSOR_ENABLE */

#ifdef UI_ENCODER_ENABLE
  UI_ENCODER_Init(); /* 初始化UI编码器（轮询型） */
#endif               /* UI_ENCODER_ENABLE */

#ifdef LCD_SPI_ENABLE
  SPI_LCD_Init(); /* 初始化LCD SPI驱动 */
#endif            /* LCD_SPI_ENABLE */

#ifdef QSPI_FLASH_ENABLE
  QSPI_W25Qxx_Init(); /* 初始化QSPI Flash驱动 */
  QSPI_W25Qxx_MemoryMappedMode();
#endif

#ifdef FLASH_FONT_ENABLE
  FlashFont_Init();
#endif

#ifdef DMIC_ENABLE
  DMIC_Init();
#endif /* DMIC_ENABLE */

#ifdef OLED_HARD_ENABLE
  OLED_hard_Init();
#endif

#ifdef OLED_SOFT_ENABLE
  OLED_Soft_Init();
#endif

#ifdef SDMMC_ENABLE
  SDCard_Init();
#ifdef FATFS_ENABLE
  HAL_Delay(100);
  FatFs_Check();
#endif
#endif

#ifdef SDRAM_ENABLE
  SDRAM_Initialization_Sequence();
#endif

#ifdef LCD_RGB_ENABLE
  RGB_LCD_Init(); /* 初始化LCD RGB驱动 */
#endif            /* LCD_RGB_ENABLE */

#ifdef LCD_RGB_TOUCH_ENABLE
  Touch_Init(); /* 初始化LCD触摸驱动 */
#endif

#ifdef DSPEAKER_ENABLE
  DSPEAKER_Init(); /* 初始化LCD触摸驱动 */
#endif

#ifdef USB_DEVICE_ENABLE
  MX_USB_DEVICE_Init();
#endif

#ifdef USB_HOST_ENABLE
  MX_USB_HOST_Init();
#endif
  DEBUG_INFO("初始化完成");
  /*******************************************************************************
   *                              用户自定义初始化部分
   ******************************************************************************/

  // RGB_LCD_SetColor(0xff333333);     /* 设置画笔色，使用自定义颜色 */
  // RGB_LCD_SetBackColor(0xffB9EDF8); /* 设置背景色，使用自定义颜色 */
  // RGB_LCD_Clear();                  /* 清屏，刷背景色 */

  // RGB_LCD_SetTextFont(32);
  // RGB_LCD_DisplayText(42, 20, "电容触摸测试");
  // RGB_LCD_DisplayText(42, 70, "核心板型号：FK750M6-XBH6");
  // RGB_LCD_DisplayText(42, 120, "屏幕分辨率：800*480");

  // RGB_LCD_DisplayString(44, 170, "X1:       Y1:");
  // RGB_LCD_DisplayString(44, 220, "X2:       Y2:");
  // RGB_LCD_DisplayString(44, 270, "X3:       Y3:");
  // RGB_LCD_DisplayString(44, 320, "X4:       Y4:");
  // RGB_LCD_DisplayString(44, 370, "X5:       Y5:");
  // RGB_LCD_SetTextFont(32);
  // RGB_LCD_DisplayText(42, 20, "电容触摸测试");
  // RGB_LCD_DisplayText(42, 70, "核心板型号：FK750M6-XBH6");
  // RGB_LCD_DisplayText(42, 120, "屏幕分辨率：800*480");

  // 显示1.jpg
JPEG_App_DisplayFile("1.jpg", 0, 0);
JPEG_App_DisplayFile("2.jpg", 300, 0);
/* 下面是用于验证 printf 风格的 DEBUG_INFO 示例 */

// 创建纯红色的800x480图片并保存为3.jpg
// RGB565红色 = (255>>3)<<11 = 0xF800
// JPEG_App_CreateSolidColorImage("4.jpg", 800, 480, 0xFFE0,
// JPEG_Quality_High);

// /* 全局音乐播放器实例 */
// static AudioPlayer_t g_music_player = {0};

// /* 1. 打开 WAV 文件 */
// if (AudioPlayer_OpenFile(&g_music_player, "0:1.wav") != HAL_OK) {
//   DEBUG_ERROR("打开音乐文件失败");
//   return;
// }

// /* 2. 开始无限循环播放（loop_count=0 表示无限循环） */
// AudioPlayer_PlayWithLoop(&g_music_player, 0);



/* 打开 MP3 文件 */
// if (MP3Player_OpenFile(&g_mp3_player, "0:1.mp3") == HAL_OK)
// {
//   DEBUG_INFO("MP3 文件打开成功");

//   MP3Player_PlayWithLoop(&g_mp3_player, 0);  // 无限循环
  
// }
}

/*******************************************************************************
 *                              主循环周期任务
 ******************************************************************************/

/**
 * @brief  主循环周期任务
 * @note   在主函数 while(1) 中调用，驱动非阻塞任务
 * @note   建议调用间隔: 5~20ms
 *
 * @par    调用的任务列表：
 *         - KEY_Task(): 按键扫描（消抖、事件检测）
 *         - DIGITAL_SENSOR_Task(): 数字传感器扫描
 *         - UI_ENCODER_Poll(): UI编码器轮询（如果启用）
 *
 * @retval None
 */
void main_while(void) {
  // MX_USB_HOST_Process();
  // HAL_Delay(1);
  //  USB_printf("STM32H750虚拟串口实验\r\n");
  // LED_Blink_All(1000);
// #ifdef DSPEAKER_ENABLE
//     MP3Player_DecodeTask(&g_mp3_player);
// #endif
}
