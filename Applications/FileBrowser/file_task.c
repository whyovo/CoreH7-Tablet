#include "file_task.h"
#include "file_ui.h"
#include "file_logic.h"
#include "config.h"

static void FileBrowser_App_Init(void)
{
    DEBUG_INFO("FileBrowser Init");
    FileLogic_Init();
    FileBrowser_UI_Create();
}

static void FileBrowser_App_Exit(void)
{
    DEBUG_INFO("FileBrowser Exit");
    FileBrowser_UI_Delete();
}

static void FileBrowser_Task_Entry(void *params)
{
    DEBUG_INFO("FileBrowser Task Running");
    while (1)
    {
        // 逻辑主要由 UI 事件驱动
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

App_Descriptor_t FileBrowserApp = {
    .name = "Files",
    .app_init = FileBrowser_App_Init,
    .app_exit = FileBrowser_App_Exit,
    .task_entry = FileBrowser_Task_Entry,
    .stack_size = 2048 
};
