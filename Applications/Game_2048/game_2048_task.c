#include "game_2048_task.h"
#include "game_2048_ui.h"
#include "game_2048_core.h"
#include "config.h"

// 作用: RTOS任务逻辑与状态机

static void Game2048_App_Init(void)
{
    DEBUG_INFO("2048 App Init");
    Game2048_Core_Init();
    Game2048_UI_Create();
}

static void Game2048_App_Exit(void)
{
    DEBUG_INFO("2048 App Exit");
    Game2048_UI_Delete();
}

static void Game2048_Task_Entry(void *params)
{
    DEBUG_INFO("2048 Task Running");
    while (1)
    {
        // 游戏逻辑主要由 UI 事件驱动，这里只需维持任务存活
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

App_Descriptor_t Game2048App = {
    .name = "2048",
    .app_init = Game2048_App_Init,
    .app_exit = Game2048_App_Exit,
    .task_entry = Game2048_Task_Entry,
    .stack_size = 1024};
