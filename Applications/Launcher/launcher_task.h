#ifndef LAUNCHER_TASK_H
#define LAUNCHER_TASK_H

#include "system_task.h"

// 声明 Launcher 应用描述符，供系统调度使用
extern App_Descriptor_t LauncherApp;

// 函数声明
void Launcher_App_Init(void);
void Launcher_App_Exit(void);
void Launcher_Task_Entry(void *params);

#endif /* LAUNCHER_TASK_H */
