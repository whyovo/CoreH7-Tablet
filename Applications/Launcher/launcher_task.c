#include "launcher_task.h"
#include "launcher_ui.h"
#include "system_task.h"
#include "config.h"      

/**
 * @brief Launcher 初始化
 */
void Launcher_App_Init(void)
{
    DEBUG_INFO("Launcher App Init...");
    Launcher_UI_Create();
}

/**
 * @brief Launcher 退出清理
 */
void Launcher_App_Exit(void)
{
    DEBUG_INFO("Launcher App Exiting...");
    // 销毁 Launcher 的 UI 对象
    Launcher_UI_Delete();
    // 这里可以释放 Launcher 申请的其他内存
}

/**
 * @brief Launcher 任务入口
 */
void Launcher_Task_Entry(void *params)
{
    DEBUG_INFO("Launcher Task Started");

    while (1)
    {
        //  Launcher 的后台逻辑
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* --- 定义 Launcher 应用描述符 --- */
// 这样 system_task.c 就可以通过 extern 引用它
App_Descriptor_t LauncherApp = {
    .name = "Launcher",
    .app_init = Launcher_App_Init,
    .app_exit = Launcher_App_Exit,
    .task_entry = Launcher_Task_Entry,
    .stack_size = 1024};
