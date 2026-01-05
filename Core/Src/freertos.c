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
#include "lvgl.h"
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

// 移除 lvglMutex 定义，它现在在 system_task.c 中
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
// 声明外部任务函数
extern void StartSystemDisplayTask(void *argument);
extern void StartHardwareTouchTask(void *argument);

TaskHandle_t systemDisplayTaskHandle;
TaskHandle_t hardwareTouchTaskHandle;
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
  
  // 创建系统显示任务 (LVGL)
  xTaskCreate(StartSystemDisplayTask, "SysDispTask", 4096, NULL, tskIDLE_PRIORITY + 2, &systemDisplayTaskHandle);

  // 创建硬件触摸任务
  xTaskCreate(StartHardwareTouchTask, "HwTouchTask", 1024, NULL, tskIDLE_PRIORITY + 3, &hardwareTouchTaskHandle);
  
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


/* USER CODE END Application */

