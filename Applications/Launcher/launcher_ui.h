#ifndef LAUNCHER_UI_H
#define LAUNCHER_UI_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "lvgl.h"

    /**
     * @brief 创建 Launcher 主界面 UI
     * 包含 3x2 的应用图标矩阵
     */
    void Launcher_UI_Create(void);

    /**
     * @brief 彻底销毁 Launcher UI 对象
     * 用于切换应用时的内存释放
     */
    void Launcher_UI_Delete(void);

#ifdef __cplusplus
}
#endif

#endif /* LAUNCHER_UI_H */
