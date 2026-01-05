#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "semphr.h"
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "lv_port_fs.h"
#include "lv_port_jpeg.h"
#include "lv_font_qspi.h"
#include "config.h"

// 定义应用描述符结构
typedef struct
{
    const char *name;
    void (*app_init)(void);    // UI 创建
    void (*app_exit)(void);    // UI 销毁与内存清理
    TaskFunction_t task_entry; // 任务入口函数
    uint32_t stack_size;       // 该应用需要的栈大小
} App_Descriptor_t;

// 全局变量
SemaphoreHandle_t lvglMutex;
// 文件系统互斥锁
SemaphoreHandle_t fsMutex;

TaskHandle_t currentAppTaskHandle = NULL;
static App_Descriptor_t *currentAppDescriptor = NULL; // 记录当前 App 的描述符，以便调用 exit

// 引用外部定义的 LauncherApp
extern App_Descriptor_t LauncherApp;
// 引用外部触摸扫描函数
extern void Touch_Scan(void);

// 日志回调
static void lvgl_log_print_cb(const char *buf)
{
    if (buf != NULL)
    {
        DEBUG_INFO("[LVGL] %s", buf);
    }
}

// 切换应用逻辑
void Switch_To_App(App_Descriptor_t *new_app)
{
    // 1. 如果有正在运行的 App，先执行它的退出清理逻辑
    if (currentAppDescriptor != NULL)
    {
        // 调用退出回调 (清理 UI、释放资源)
        if (currentAppDescriptor->app_exit)
        {
            currentAppDescriptor->app_exit();
        }
    }

    // 2. 清理旧任务
    if (currentAppTaskHandle != NULL)
    {
        vTaskDelete(currentAppTaskHandle);
        currentAppTaskHandle = NULL;
    }

    // 3. 更新当前 App 描述符
    currentAppDescriptor = new_app;

    // 4. 执行新 App 的 UI 初始化 (需要在 LVGL 锁保护下调用，或者在 Display 任务中调用)
    if (new_app->app_init)
    {
        new_app->app_init();
    }

    // 5. 动态创建新任务
    if (new_app->task_entry)
    {
        xTaskCreate(new_app->task_entry, new_app->name, new_app->stack_size, NULL, tskIDLE_PRIORITY + 1, &currentAppTaskHandle);
    }
}

// 系统显示任务
void StartSystemDisplayTask(void *argument)
{
    /* LVGL初始化 */
    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();
    lv_port_fs_init();
    lv_port_jpeg_init();
    lv_font_qspi_init();
    lv_log_register_print_cb(lvgl_log_print_cb);

    /* 创建互斥量 */
    lvglMutex = xSemaphoreCreateMutex();
    fsMutex = xSemaphoreCreateMutex(); // 创建 FS 锁

    /* 启动默认应用 (Launcher) */
    // 获取锁以确保安全，虽然此时任务还没开始调度
    xSemaphoreTake(lvglMutex, portMAX_DELAY);

    // 显式初始化 currentAppDescriptor 为 NULL，确保第一次切换正常
    currentAppDescriptor = NULL;
    Switch_To_App(&LauncherApp);

    xSemaphoreGive(lvglMutex);

    DEBUG_INFO("System Display Task Ready");

    /* 主循环 */
    for (;;)
    {
        if (xSemaphoreTake(lvglMutex, portMAX_DELAY) == pdTRUE)
        {
            // 使用 fsMutex 保护 lv_task_handler，因为 LVGL 可能会读取 SD 卡加载图片
            if (xSemaphoreTake(fsMutex, portMAX_DELAY) == pdTRUE)
            {
                lv_task_handler();
                xSemaphoreGive(fsMutex);
            }
            xSemaphoreGive(lvglMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(16));
    }
}

// 硬件触摸扫描任务
void StartHardwareTouchTask(void *argument)
{
    for (;;)
    {
        Touch_Scan();
        vTaskDelay(pdMS_TO_TICKS(10)); // 100Hz 采样
    }
}