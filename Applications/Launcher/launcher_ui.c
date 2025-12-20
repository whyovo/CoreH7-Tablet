#include "launcher_ui.h"
#include "system_task.h"
#include "img_settings.h"
#include "img_osu.h"
#include "img_file.h"
#include "img_camera.h"
#include "img_2048.h"
#include "game_2048_task.h"
#include "file_task.h"

// 静态变量
static lv_obj_t *ui_main_cont = NULL;
static lv_obj_t *ui_bg_img = NULL;

/**
 * @brief 图标点击事件回调
 */
static void icon_click_cb(lv_event_t *e)
{
    // 获取绑定在该对象上的 App 描述符
    App_Descriptor_t *app = (App_Descriptor_t *)lv_event_get_user_data(e);
    if (app != NULL)
        // 调用系统接口切换应用
        Switch_To_App(app);
}

void Launcher_UI_Create(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

    // 确保之前的 UI 已清理
    if (ui_main_cont != NULL)
        Launcher_UI_Delete();

    // 1. 创建背景图片 (如果不存在才创建)
    if (ui_bg_img == NULL)
    {
        ui_bg_img = lv_img_create(scr);
        lv_img_set_src(ui_bg_img, "S:/sys/background/1.jpg");
        lv_obj_set_size(ui_bg_img, 800, 480);
        lv_obj_center(ui_bg_img);
        // 确保背景在最底层
        lv_obj_move_background(ui_bg_img);
    }

    // 2. 创建主容器 (透明，用于承载内容)
    ui_main_cont = lv_obj_create(scr);
    lv_obj_set_size(ui_main_cont, 800, 480);
    lv_obj_set_style_bg_opa(ui_main_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ui_main_cont, 0, 0);
    lv_obj_set_style_pad_all(ui_main_cont, 0, 0);
    lv_obj_clear_flag(ui_main_cont, LV_OBJ_FLAG_SCROLLABLE);
    // 强制关闭滚动条（右侧那根线）
    lv_obj_set_scrollbar_mode(ui_main_cont, LV_SCROLLBAR_MODE_OFF);

    // --- 3. 顶部状态栏 ---
    lv_obj_t *status_bar = lv_obj_create(ui_main_cont);
    lv_obj_set_size(status_bar, 800, 40);
    lv_obj_set_align(status_bar, LV_ALIGN_TOP_MID);
    lv_obj_set_style_bg_color(status_bar, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(status_bar, LV_OPA_30, 0);
    lv_obj_set_style_border_width(status_bar, 0, 0);
    lv_obj_set_style_radius(status_bar, 0, 0);
    lv_obj_set_style_pad_hor(status_bar, 20, 0);
    // 关闭状态栏滚动属性
    lv_obj_clear_flag(status_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(status_bar, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *status_icons = lv_label_create(status_bar);
    // 添加了 SD 卡图标
    lv_label_set_text(status_icons, LV_SYMBOL_SD_CARD "  " LV_SYMBOL_WIFI "  " LV_SYMBOL_BATTERY_FULL);
    lv_obj_set_style_text_color(status_icons, lv_color_white(), 0);
    // 字体大小改为 20
    lv_obj_set_style_text_font(status_icons, &lv_font_montserrat_20, 0);
    lv_obj_align(status_icons, LV_ALIGN_RIGHT_MID, 0, 0);

    // --- 4. 图标区域布局 ---
    lv_obj_t *icon_grid = lv_obj_create(ui_main_cont);
    lv_obj_set_size(icon_grid, 800, 440);
    lv_obj_align(icon_grid, LV_ALIGN_BOTTOM_MID, 0, 0);
    // 样式彻底透明化
    lv_obj_set_style_bg_opa(icon_grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(icon_grid, 0, 0);
    lv_obj_set_scrollbar_mode(icon_grid, LV_SCROLLBAR_MODE_OFF);

    // 设置左对齐布局
    lv_obj_set_flex_flow(icon_grid, LV_FLEX_FLOW_ROW_WRAP);
    // 改为 START，配合 pad_column 实现手动间距
    lv_obj_set_flex_align(icon_grid, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    // 间距微调：
    // 屏幕 800 - (左右内边距 35*2) = 730 可用。
    // 4个图标 160*4 = 640。
    // 剩余 90 像素分配给 3 个间隙，每个间隙 30。
    lv_obj_set_style_pad_all(icon_grid, 35, 0);
    lv_obj_set_style_pad_column(icon_grid, 30, 0);
    lv_obj_set_style_pad_row(icon_grid, 20, 0);

    struct
    {
        const lv_img_dsc_t *img;
        const char *name;
        App_Descriptor_t *app_desc;
    } apps[] = {
        {&img_camera, "Camera", NULL},
        {&img_osu, "Osu!", NULL},
        {&img_file, "Files", &FileBrowserApp},
        {&img_2048, "2048", &Game2048App},
        {&img_settings, "Settings", NULL},
        {NULL, NULL, NULL}};

    for (int i = 0; apps[i].img != NULL; i++)
    {
        lv_obj_t *item = lv_obj_create(icon_grid);
        lv_obj_set_size(item, 160, 185);
        lv_obj_set_flex_flow(item, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(item, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        // 清除 Item 默认样式
        lv_obj_set_style_bg_opa(item, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(item, 0, 0);
        lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(item, LV_SCROLLBAR_MODE_OFF);

        // 点击缩放效果（256是1:1）
        lv_obj_set_style_transform_zoom(item, 240, LV_STATE_PRESSED);
        // 1. 图标
        lv_obj_t *img = lv_img_create(item);
        lv_img_set_src(img, apps[i].img);
        // 2. 文字

        lv_obj_t *label = lv_label_create(item);
        lv_label_set_text(label, apps[i].name);
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0); // 与状态栏统一
        lv_obj_set_style_pad_top(label, 8, 0);
        // 事件绑定到 item 容器上，增加点击面积
        lv_obj_add_flag(item, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(item, icon_click_cb, LV_EVENT_CLICKED, (void *)apps[i].app_desc);
    }
}

void Launcher_UI_Delete(void)
{
    // 删除主内容容器（图标等）
    if (ui_main_cont != NULL)
    {
        lv_obj_del(ui_main_cont);
        ui_main_cont = NULL;
    }

    /*
    if (ui_bg_img != NULL)
    {
        lv_obj_del(ui_bg_img);
        ui_bg_img = NULL;
    }
    */
}
