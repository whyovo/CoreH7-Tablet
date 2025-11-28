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
#include "cmsis_os.h"
#include "main.h"
#include "task.h"


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
SemaphoreHandle_t lvglMutex;
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
    .name = "defaultTask",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void StartTouchTask(void *argument);
TaskHandle_t touchTaskHandle;
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void vApplicationTickHook(void);

/* USER CODE BEGIN 3 */
void vApplicationTickHook(void) {
  /* This function will be called by each tick interrupt if
  configUSE_TICK_HOOK is set to 1 in FreeRTOSConfig.h. User code can be
  added here, but the tick hook is called from an interrupt context, so
  code must not attempt to block, and only the interrupt safe FreeRTOS API
  functions can be used (those that end in FromISR()). */
  lv_tick_inc(1);
}
/* USER CODE END 3 */

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
  defaultTaskHandle =
      osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* creation of touchTask */
  xTaskCreate(StartTouchTask, "touchTask", 1024, NULL, tskIDLE_PRIORITY + 2,
              &touchTaskHandle);
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
void StartDefaultTask(void *argument) {
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for (;;) {
    // #ifdef LED_ENABLE
    //     LED_Toggle_All();
    // #endif

    osDelay(1000);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void __attribute__((section(".dtcm_text"))) StartTouchTask(void *argument);
TaskHandle_t touchTaskHandle;

void StartTouchTask(void *argument) {

  /* LVGL初始化 */
  lv_init();
  // DEBUG_INFO("2");
  /* 初始化显示 */
  lv_port_disp_init();
  // DEBUG_INFO("3");
  /* 初始化输入设备 */
  lv_port_indev_init();
  // 初始化QSPI字库
  lv_font_qspi_init();
  // DEBUG_INFO("4");
  /* ===== UI 创建阶段 - 禁用显示更新 ===== */
  disp_disable_update(); /* 禁用显示刷新 */
  // DEBUG_INFO("5");
  /* 加载官方Widget Demo */
  // lv_demo_widgets();
  // lv_demo_benchmark();
  // DEBUG_INFO("6");
  /* ===== 启用显示更新 ===== */
  disp_enable_update();
  DEBUG_INFO("lvgl ready");
  // 获取24号字体
  lv_font_t *font_24 = lv_font_qspi_get_by_size(24);
  if (font_24 == NULL) {
    LV_LOG_ERROR("获取字体失败");
    return;
  }

  // 获取默认屏幕
  lv_obj_t *scr = lv_scr_act();

  // 清屏背景色
  lv_obj_set_style_bg_color(scr, lv_color_white(), LV_PART_MAIN);

  // ===== 示例1: 显示纯汉字 =====
  lv_obj_t *label1 = lv_label_create(scr);
  lv_label_set_text(label1, "你好世界");
  lv_obj_set_style_text_font(label1, font_24, LV_PART_MAIN);
  lv_obj_set_style_text_color(label1, lv_color_black(), LV_PART_MAIN);
  lv_obj_set_pos(label1, 10, 10);

  // ===== 示例2: 显示汉字+数字混合 =====
  lv_obj_t *label2 = lv_label_create(scr);
  lv_label_set_text(label2, "temp温度: 25℃");
  lv_obj_set_style_text_font(label2, font_24, LV_PART_MAIN);
  lv_obj_set_style_text_color(label2, lv_color_hex(0xFF0000), LV_PART_MAIN);
  lv_obj_set_pos(label2, 10, 50);

  // ===== 示例3: 创建按钮带汉字标签 =====
  lv_obj_t *btn = lv_btn_create(scr);
  lv_obj_set_pos(btn, 10, 180);
  lv_obj_set_size(btn, 200, 50);
  lv_obj_set_style_bg_color(btn, lv_color_hex(0x007BFF), LV_PART_MAIN);

  lv_obj_t *btn_label = lv_label_create(btn);
  lv_label_set_text(btn_label, "开始游戏");
  lv_obj_set_style_text_font(btn_label, font_24, LV_PART_MAIN);
  lv_obj_set_style_text_color(btn_label, lv_color_white(), LV_PART_MAIN);
  lv_obj_center(btn_label);
  /* 主循环 */
  for (;;) {
    /* 获取互斥量 - 等待无限期 */
    if (xSemaphoreTake(lvglMutex, portMAX_DELAY) == pdTRUE) {
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
