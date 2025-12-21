#include "reader_task.h"
#include "reader_ui.h"
#include "config.h"

// 作用: Reader 任务逻辑与状态机

static void Reader_App_Init(void)
{
    DEBUG_INFO("Reader App Init");
    // 初始化 UI，UI 内部会调用 Core 进行文件扫描
    Reader_UI_Create();
}

static void Reader_App_Exit(void)
{
    DEBUG_INFO("Reader App Exit");
    Reader_UI_Delete();
}

static void Reader_Task_Entry(void *params)
{
    DEBUG_INFO("Reader Task Running");
    while (1)
    {
        // 业务逻辑主要由 UI 事件驱动
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

App_Descriptor_t ReaderApp = {
    .name = "Reader",
    .app_init = Reader_App_Init,
    .app_exit = Reader_App_Exit,
    .task_entry = Reader_Task_Entry,
    .stack_size = 2048 
};
