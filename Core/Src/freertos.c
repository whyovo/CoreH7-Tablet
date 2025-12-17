/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "init.h"
#include "lv_demos.h"
#include "lvgl.h"
#include "queue.h"

// #include "lv_flash_font.h"
#include "lv_font_qspi.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "lv_port_fs.h"
#include "lv_port_jpeg.h"
#include "semphr.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
/* configTOTAL_HEAP_SIZE 在 FreeRTOSConfig.h 中设置为 288000 字节 */
uint8_t ucHeap[configTOTAL_HEAP_SIZE] __attribute__((section(".ram_d2_data")));

SemaphoreHandle_t lvglMutex;
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void StartTouchTask(void *argument);
TaskHandle_t touchTaskHandle;
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void configureTimerForRunTimeStats(void);
unsigned long getRunTimeCounterValue(void);
void vApplicationTickHook(void);
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName);

/* USER CODE BEGIN 1 */

/* Functions needed when configGENERATE_RUN_TIME_STATS is on */
__weak void configureTimerForRunTimeStats(void)
{
  /* 使用 SysTick 作为运行时统计的时基
     SysTick 已经配置好了，直接利用它即可 */
}

__weak unsigned long getRunTimeCounterValue(void)
{
  /* 返回当前的系统 Tick 计数
     这样每个 Tick (1ms) 增加一次 */
  return xTaskGetTickCount();
}
/* USER CODE END 1 */

/* USER CODE BEGIN 3 */
void vApplicationTickHook(void)
{
  /* This function will be called by each tick interrupt if
  configUSE_TICK_HOOK is set to 1 in FreeRTOSConfig.h. User code can be
  added here, but the tick hook is called from an interrupt context, so
  code must not attempt to block, and only the interrupt safe FreeRTOS API
  functions can be used (those that end in FromISR()). */
  lv_tick_inc(1);
}
/* USER CODE END 3 */

/* USER CODE BEGIN 4 */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName)
{
  /* Run time stack overflow checking is performed if
  configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2. This hook function is
  called if a stack overflow is detected. */
}
/* USER CODE END 4 */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  lvglMutex = xSemaphoreCreateMutex(); // 创建互斥量
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* Create touch event queue */

  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* creation of touchTask */
  xTaskCreate(StartTouchTask, "touchTask", 4096, NULL, tskIDLE_PRIORITY + 2,&touchTaskHandle);
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for (;;)
  {
#ifdef LED_ENABLE
    LED_Toggle_All();
#endif

    /* 打印任务统计信息 */
//     {
//       static char pcWriteBuffer[1024];
//       DEBUG_INFO("===== FreeRTOS Task Statistics =====");
//       vTaskList(pcWriteBuffer);
//       DEBUG_INFO("%s", pcWriteBuffer);

// #if (configGENERATE_RUN_TIME_STATS == 1)
//       DEBUG_INFO("===== Runtime Statistics =====");
//       vTaskGetRunTimeStats(pcWriteBuffer);
//       DEBUG_INFO("%s", pcWriteBuffer);
// #endif

//       DEBUG_INFO("===== Heap Info =====");
//       DEBUG_INFO("Free Heap: %u bytes", xPortGetFreeHeapSize());
//       DEBUG_INFO("Min Free Heap: %u bytes", xPortGetMinimumEverFreeHeapSize());
//     }

    osDelay(3000);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void __attribute__((section(".dtcm_text"))) StartTouchTask(void *argument);
TaskHandle_t touchTaskHandle;
// static void my_event_handler(lv_event_t *e)
// {
//   lv_obj_t *obj = lv_event_get_target(e);
//   lv_obj_t *label = (lv_obj_t *)lv_event_get_user_data(e);
//   lv_label_set_text(label, "Clicked!");
//   lv_obj_set_style_opa(obj, LV_OPA_50, LV_PART_MAIN);
// }
/* 日志回调函数 */
static void lvgl_log_print_cb(const char *buf)
{
  if (buf != NULL)
  {
    DEBUG_INFO("[LVGL] %s", buf);
  }
}
void StartTouchTask(void *argument)
{

  /* LVGL初始化 */
  lv_init();
  // DEBUG_INFO("2");
  /* 初始化显示 */
  lv_port_disp_init();
  // DEBUG_INFO("3");
  /* 初始化输入设备 */
  lv_port_indev_init();
  /* 初始化文件系统 */
  lv_port_fs_init();
  /* 初始化 JPEG 解码器 */
  lv_port_jpeg_init(); // 添加初始化
  // 初始化QSPI字库
  lv_font_qspi_init();
  /* 注册 LVGL 日志回调到 DEBUG_INFO */
  lv_log_register_print_cb(lvgl_log_print_cb);

  // DEBUG_INFO("4");
  /* ===== UI 创建阶段 - 禁用显示更新 ===== */
  disp_disable_update(); /* 禁用显示刷新 */
  // DEBUG_INFO("5");
  /* 加载官方Widget Demo */
  // lv_demo_widgets();
  // lv_demo_benchmark();
  // DEBUG_INFO("6");

  /* ===== 创建中文输入界面 - 屏幕高度 480px ===== */
  lv_obj_t *scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, lv_color_white(), LV_PART_MAIN);

  // // 获取字体
  // lv_font_t *font_24 = lv_font_qspi_get_by_size(24);
  // lv_font_t *font_16 = lv_font_qspi_get_by_size(16);

  // /* ===== 顶部区域：标题 ===== */
  // lv_obj_t *title = lv_label_create(scr);
  // lv_label_set_text(title, "中文输入演示");
  // lv_obj_set_style_text_font(title, font_24, LV_PART_MAIN);
  // lv_obj_set_style_text_color(title, lv_color_hex(0x0066CC), LV_PART_MAIN);
  // lv_obj_set_pos(title, 20, 8);

  // /* ===== 输入框区域：文本框 ===== */
  // lv_obj_t *textarea = lv_textarea_create(scr);
  // lv_obj_set_size(textarea, 760, 70);
  // lv_obj_set_pos(textarea, 20, 40);
  // lv_obj_set_style_text_font(textarea, font_24, LV_PART_MAIN);
  // lv_obj_set_style_bg_color(textarea, lv_color_hex(0xF5F5F5), LV_PART_MAIN);
  // lv_obj_set_style_border_color(textarea, lv_color_hex(0xCCCCCC), LV_PART_MAIN);
  // lv_obj_set_style_border_width(textarea, 2, LV_PART_MAIN);
  // lv_textarea_set_placeholder_text(textarea, "点击输入文字");
  // lv_textarea_set_one_line(textarea, false);

  // /* ===== 中间区域：候选字显示 ===== */
  // lv_obj_t *candidate_label = lv_label_create(scr);
  // lv_label_set_text(candidate_label, "候选字: 字库支持中文");
  // lv_obj_set_style_text_font(candidate_label, font_16, LV_PART_MAIN);
  // lv_obj_set_style_text_color(candidate_label, lv_color_hex(0xFF6600),
  //                             LV_PART_MAIN);
  // lv_obj_set_pos(candidate_label, 20, 115);

  // /* ===== 创建拼音输入法 ===== */
  // lv_obj_t *ime_pinyin = lv_ime_pinyin_create(scr);
  // /*设置候选字组件的字体，否则显示为空白方格 */
  // lv_obj_set_style_text_font(ime_pinyin, font_24, LV_PART_MAIN);
  // /* 设置候选字栏的位置和样式 */
  // lv_obj_set_pos(ime_pinyin, 20, 135);
  // lv_obj_set_size(ime_pinyin, 760, 40);
  // lv_obj_set_style_border_width(ime_pinyin, 0,
  //                               LV_PART_MAIN); // 去除边框让它看起来更自然
  // lv_obj_set_style_bg_color(ime_pinyin, lv_color_hex(0xF0F0F0),
  //                           LV_PART_MAIN); // 浅灰背景区分
  // /* 创建标准键盘 */
  // lv_obj_t *keyboard = lv_keyboard_create(scr);
  // lv_obj_set_size(keyboard, 760, 300);
  // lv_obj_set_pos(keyboard, 0, 0); // 改为正确的Y坐标

  // /* 绑定拼音输入法到键盘 */
  // lv_ime_pinyin_set_keyboard(ime_pinyin, keyboard);

  // /* 绑定文本框到键盘 (这是正确的方式) */
  // lv_keyboard_set_textarea(keyboard, textarea);

  // /* 键盘样式 */
  // lv_obj_set_style_bg_color(keyboard, lv_color_hex(0xE8E8E8), LV_PART_MAIN);
  // lv_obj_set_style_border_color(keyboard, lv_color_hex(0x999999), LV_PART_MAIN);
  // lv_obj_set_style_border_width(keyboard, 1, LV_PART_MAIN);

  // /* 按钮样式 */
  // lv_obj_set_style_bg_color(keyboard, lv_color_hex(0xFFFFFF), LV_PART_ITEMS);
  // lv_obj_set_style_border_width(keyboard, 1, LV_PART_ITEMS);
  // lv_obj_set_style_border_color(keyboard, lv_color_hex(0xCCCCCC),
  //                               LV_PART_ITEMS);
  // lv_obj_set_style_text_font(keyboard, font_16, LV_PART_ITEMS);
  // lv_obj_set_style_pad_all(keyboard, 4, LV_PART_ITEMS);

  /* ===== 显示图片 1.jpg（靠左） ===== */
  /* 左边实例 */
  lv_obj_t *img_left = lv_img_create(scr);
  lv_img_set_src(img_left, "S:/1.jpg");
  lv_obj_align(img_left, LV_ALIGN_LEFT_MID, 0, 0);

  // /* 右边实例 */
  lv_obj_t *img_right = lv_img_create(scr);
  lv_img_set_src(img_right, "S:/2.jpg");
  lv_obj_align(img_right, LV_ALIGN_RIGHT_MID, 0, 0);



   /* ===== 启用显示更新 ===== */
   disp_enable_update();
   
   DEBUG_INFO("lvgl ready");
   /* 主循环 */
   for (;;)
   {
     /* 获取互斥量 - 等待无限期 */
     if (xSemaphoreTake(lvglMutex, portMAX_DELAY) == pdTRUE)
     {
       /* 处理 LVGL 任务 */
       lv_task_handler();
       Touch_Scan();
       /* 释放互斥量 */
       xSemaphoreGive(lvglMutex);
     }

     vTaskDelay(pdMS_TO_TICKS(16));
   }
}


// void vApplicationTickHook(void) {  }
/* USER CODE END Application */

