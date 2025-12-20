#include "file_ui.h"
#include "file_logic.h"
#include "launcher_task.h"
#include "system_task.h"
#include "lv_font_qspi.h"

static lv_obj_t *ui_root = NULL;
static lv_obj_t *ui_path_label = NULL;
static lv_obj_t *ui_file_list = NULL;
static lv_obj_t *ui_paste_btn = NULL;

// 异步退出
static void switch_to_launcher_async(void *data)
{
    Switch_To_App(&LauncherApp);
}

// 退出按钮回调
static void close_btn_cb(lv_event_t *e)
{
    lv_async_call(switch_to_launcher_async, NULL);
}

// 返回上一级回调
static void back_btn_cb(lv_event_t *e)
{
    if (FileLogic_GoUp())
    {
        FileBrowser_UI_Update();
    }
}

// 粘贴按钮回调
static void paste_btn_cb(lv_event_t *e)
{
    if (FileLogic_Paste())
    {
        lv_obj_t *mbox = lv_msgbox_create(NULL, "提示", "粘贴成功", NULL, true);
        lv_obj_center(mbox);
        lv_obj_set_style_text_font(mbox, &lv_font_montserrat_20, 0);
        FileBrowser_UI_Update();
    }
    else
    {
        lv_obj_t *mbox = lv_msgbox_create(NULL, "错误", "粘贴失败", NULL, true);
        lv_obj_center(mbox);
        lv_obj_set_style_text_font(mbox, &lv_font_montserrat_20, 0);
    }
    FileLogic_ClearClipboard();
    lv_obj_add_flag(ui_paste_btn, LV_OBJ_FLAG_HIDDEN);
}

// 文件操作弹窗回调
static void file_action_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    // 修改：使用索引判断更安全 (0:复制, 1:删除, 2:取消)
    uint16_t btn_id = lv_msgbox_get_active_btn(obj);
    const char *filename = (const char *)lv_event_get_user_data(e);

    if (btn_id == 0) // 复制
    {
        FileLogic_SetClipboard(filename);
        lv_obj_clear_flag(ui_paste_btn, LV_OBJ_FLAG_HIDDEN);
    }
    else if (btn_id == 1) // 删除
    {
        if (FileLogic_Delete(filename))
        {
            FileBrowser_UI_Update();
        }
        else
        {
            // 提示删除失败
        }
    }

    // 无论是 复制、删除 还是 取消(id=2)，最后都关闭弹窗
    lv_msgbox_close(obj);
}

// 列表项点击回调
static void list_item_cb(lv_event_t *e)
{
    FileInfo_t *info = (FileInfo_t *)lv_event_get_user_data(e);

    if (info->type == FILE_TYPE_DIR)
    {
        // 进入文件夹
        if (FileLogic_EnterDir(info->name))
        {
            FileBrowser_UI_Update();
        }
    }
    else
    {
        // 文件：弹出操作菜单
        static const char *btns[] = {"复制", "删除", "取消", ""};
        char title[100];
        snprintf(title, sizeof(title), "操作: %s", info->name);

        lv_obj_t *mbox = lv_msgbox_create(ui_root, title, "请选择操作", btns, true);
        lv_obj_center(mbox);

        lv_font_t *font = lv_font_qspi_get_by_size(24); // 使用 24号字体
        if (!font)
            font = &lv_font_montserrat_24;

        lv_obj_set_style_text_font(mbox, font, 0);
        lv_obj_set_style_text_font(lv_msgbox_get_btns(mbox), font, 0);

        lv_obj_add_event_cb(mbox, file_action_cb, LV_EVENT_VALUE_CHANGED, (void *)info->name);
    }
}

void FileBrowser_UI_Update(void)
{
    if (!ui_root)
        return;

    // 获取支持中文的字体
    lv_font_t *font_24 = lv_font_qspi_get_by_size(24);
    if (!font_24)
        font_24 = &lv_font_montserrat_24;

    // 1. 更新路径显示
    lv_label_set_text_fmt(ui_path_label, "Path: %s", FileLogic_GetPath());
    lv_obj_set_style_text_font(ui_path_label, font_24, 0);

    // 2. 清空列表
    lv_obj_clean(ui_file_list);

    // 3. 获取文件列表并填充
    uint16_t count = 0;
    FileInfo_t *files = FileLogic_GetFileList(&count);

    for (int i = 0; i < count; i++)
    {
        const char *icon = (files[i].type == FILE_TYPE_DIR) ? LV_SYMBOL_DIRECTORY : LV_SYMBOL_FILE;

        lv_obj_t *btn = lv_list_add_btn(ui_file_list, icon, files[i].name);

        // 设置字体
        lv_obj_set_style_text_font(btn, font_24, 0);

        // 增加按钮高度
        lv_obj_set_height(btn, 50);

        // 遍历按钮的子对象，找到 Label 并设置属性
        uint32_t child_cnt = lv_obj_get_child_cnt(btn);
        for (uint32_t j = 0; j < child_cnt; j++)
        {
            lv_obj_t *child = lv_obj_get_child(btn, j);
            if (lv_obj_check_type(child, &lv_label_class))
            {
                // 设置 flex_grow 让 Label 自动撑满剩余宽度
                lv_obj_set_flex_grow(child, 1);
                // 设置长文本滚动模式 (如果文件名过长，会循环滚动显示)
                lv_label_set_long_mode(child, LV_LABEL_LONG_SCROLL_CIRCULAR);
                break;
            }
        }

        lv_obj_add_event_cb(btn, list_item_cb, LV_EVENT_CLICKED, &files[i]);
    }
}

void FileBrowser_UI_Create(void)
{
    lv_obj_t *scr = lv_scr_act();
    ui_root = lv_obj_create(scr);
    lv_obj_set_size(ui_root, 800, 480);
    lv_obj_set_style_bg_color(ui_root, lv_color_hex(0x202020), 0);
    lv_obj_set_style_border_width(ui_root, 0, 0);
    lv_obj_clear_flag(ui_root, LV_OBJ_FLAG_SCROLLABLE);

    // 清除根容器的默认内边距，防止子控件位置偏移导致重叠
    lv_obj_set_style_pad_all(ui_root, 0, 0);

    // 顶部栏
    lv_obj_t *header = lv_obj_create(ui_root);
    lv_obj_set_size(header, 800, 60);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x404040), 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(header, 0, 0); // 清除 header padding 方便布局

    // 返回按钮
    lv_obj_t *btn_back = lv_btn_create(header);
    lv_obj_set_size(btn_back, 80, 40);
    lv_obj_align(btn_back, LV_ALIGN_LEFT_MID, 10, 0); // 左边留点空隙
    lv_obj_add_event_cb(btn_back, back_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, LV_SYMBOL_LEFT " Back");

    // 路径显示
    ui_path_label = lv_label_create(header);
    lv_obj_align(ui_path_label, LV_ALIGN_LEFT_MID, 110, 0);
    lv_obj_set_style_text_color(ui_path_label, lv_color_white(), 0);
    lv_label_set_text(ui_path_label, "Path: /");
    // 限制路径显示宽度，防止覆盖右侧按钮
    lv_obj_set_width(ui_path_label, 500);
    lv_label_set_long_mode(ui_path_label, LV_LABEL_LONG_DOT);

    // 粘贴按钮 (默认隐藏)
    ui_paste_btn = lv_btn_create(header);
    lv_obj_set_size(ui_paste_btn, 80, 40);
    lv_obj_align(ui_paste_btn, LV_ALIGN_RIGHT_MID, -70, 0);
    lv_obj_set_style_bg_color(ui_paste_btn, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_add_event_cb(ui_paste_btn, paste_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_paste = lv_label_create(ui_paste_btn);
    lv_label_set_text(lbl_paste, "Paste");
    if (!FileLogic_GetClipboard())
    {
        lv_obj_add_flag(ui_paste_btn, LV_OBJ_FLAG_HIDDEN);
    }

    // 关闭按钮
    lv_obj_t *btn_close = lv_btn_create(header);
    lv_obj_set_size(btn_close, 50, 40);
    lv_obj_align(btn_close, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(btn_close, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_add_event_cb(btn_close, close_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_close = lv_label_create(btn_close);
    lv_label_set_text(lbl_close, LV_SYMBOL_CLOSE);

    // 文件列表容器
    ui_file_list = lv_list_create(ui_root);
    // 调整列表大小和位置
    // 屏幕高度 480 - 顶部栏 60 = 420
    lv_obj_set_size(ui_file_list, 800, 420);
    // 紧贴底部，Y轴偏移设为0
    lv_obj_align(ui_file_list, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_obj_set_style_bg_color(ui_file_list, lv_color_hex(0x303030), 0);
    lv_obj_set_style_border_width(ui_file_list, 0, 0);
    lv_obj_set_style_radius(ui_file_list, 0, 0); // 去除圆角，贴合边缘

    // 初始刷新
    FileBrowser_UI_Update();
}

void FileBrowser_UI_Delete(void)
{
    if (ui_root)
    {
        lv_obj_del(ui_root);
        ui_root = NULL;
    }
}
