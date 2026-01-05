#ifndef SYSTEM_TASK_H
#define SYSTEM_TASK_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "lvgl.h"
#include <stdint.h>

    /**
     * @brief 应用描述符结构体
     * 用于定义一个独立的 App 模块及其资源需求
     */
    typedef struct
    {
        const char *name;          // 应用名称（用于调试和任务名）
        void (*app_init)(void);    // UI 创建函数：绘制该 App 的初始界面
        void (*app_exit)(void);    // UI 销毁函数：释放该 App 申请的动态内存、定时器等
        TaskFunction_t task_entry; // 任务入口函数：该 App 的 RTOS 逻辑主循环
        uint32_t stack_size;       // 任务栈大小 (单位: Word)
    } App_Descriptor_t;

    /* --- 全局变量声明 --- */

    // LVGL 线程安全互斥量：任何在非 GUI 任务中操作 LVGL 的代码都必须获取此锁
    extern SemaphoreHandle_t lvglMutex;

    extern SemaphoreHandle_t fsMutex; // 声明外部互斥锁
    
    // 当前正在运行的应用任务句柄
    extern TaskHandle_t currentAppTaskHandle;

    /* --- 系统任务原型 --- */

    /**
     * @brief 系统显示与 LVGL 管理任务
     * 职责：初始化 LVGL 核心、驱动、启动默认应用并轮詢 lv_task_handler
     */
    void StartSystemDisplayTask(void *argument);

    /**
     * @brief 硬件触摸扫描任务
     * 职责：以固定频率扫描物理触摸屏芯片
     */
    void StartHardwareTouchTask(void *argument);

    /* --- 应用管理接口 --- */

    /**
     * @brief 切换当前运行的应用
     * @param new_app 指向新应用的描述符结构体
     * @note 此函数会杀死当前运行的任务并启动新任务
     */
    void Switch_To_App(App_Descriptor_t *new_app);

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_TASK_H */
