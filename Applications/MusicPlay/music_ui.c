#include "music_ui.h"
#include "lv_demo_music.h"
#include "launcher_task.h"
#include "system_task.h"

static lv_obj_t *ui_exit_btn = NULL; // 保存退出按钮指针

// 退出按钮回调
static void exit_btn_event_cb(lv_event_t *e)
{
    // 切换回 Launcher
    Switch_To_App(&LauncherApp);
}

void Music_UI_Create(void)
{
    // 1. 创建官方 Demo 界面
    lv_demo_music();

    // 2. 创建右上角退出按钮 (覆盖在 Demo 之上)
    ui_exit_btn = lv_btn_create(lv_scr_act()); // 赋值给全局变量
    lv_obj_set_size(ui_exit_btn, 50, 50);
    lv_obj_align(ui_exit_btn, LV_ALIGN_TOP_RIGHT, -10, 10);

    // 样式美化：半透明黑色圆形
    lv_obj_set_style_bg_color(ui_exit_btn, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(ui_exit_btn, LV_OPA_50, 0);
    lv_obj_set_style_radius(ui_exit_btn, LV_RADIUS_CIRCLE, 0);

    // 添加 "X" 图标
    lv_obj_t *label = lv_label_create(ui_exit_btn);
    lv_label_set_text(label, LV_SYMBOL_CLOSE);
    lv_obj_center(label);

    // 绑定事件
    lv_obj_add_event_cb(ui_exit_btn, exit_btn_event_cb, LV_EVENT_CLICKED, NULL);
}

void Music_UI_Delete(void)
{
    // 1. 清理退出按钮
    if (ui_exit_btn != NULL)
    {
        lv_obj_del(ui_exit_btn);
        ui_exit_btn = NULL;
    }

    // 2. 清理 Demo 资源
    lv_demo_music_close();
}
