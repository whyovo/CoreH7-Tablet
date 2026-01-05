#include "music_game_task.h"
#include "music_game_ui.h"
#include "music_game_core.h" // 引入 Core
#include "config.h"

/**
 * @brief MusicGame 初始化
 */
static void MusicGame_App_Init(void)
{
    DEBUG_INFO("MusicGame App Init...");

    // 1. 初始化核心逻辑 (清空列表)
    MusicGame_Core_Init();

    // 2. 扫描歌曲 (这是一个耗时操作，必须在这里调用！)
    MusicGame_Core_ScanSongs();

    // 3. 创建 UI (此时 Core 里已经有数据了)
    MusicGame_UI_Create();
}

/**
 * @brief MusicGame 退出清理
 */
static void MusicGame_App_Exit(void)
{
    DEBUG_INFO("MusicGame App Exiting...");
    MusicGame_UI_Delete();
    // 这里释放音频资源等
}

/**
 * @brief MusicGame 任务入口
 */
static void MusicGame_Task_Entry(void *params)
{
    DEBUG_INFO("MusicGame Task Started");

    while (1)
    {
        // 游戏主循环逻辑 (例如处理音符下落计算)
        // 目前先挂起，等待 UI 事件
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* --- 定义 MusicGame 应用描述符 --- */
App_Descriptor_t MusicGameApp = {
    .name = "MusicGame",
    .app_init = MusicGame_App_Init,
    .app_exit = MusicGame_App_Exit,
    .task_entry = MusicGame_Task_Entry,
    .stack_size = 8192};
