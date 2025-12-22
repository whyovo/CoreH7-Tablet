#include "reader_ui.h"
#include "reader_core.h"
#include "launcher_task.h"
#include "system_task.h"
#include "lvgl.h"
#include "lv_font_qspi.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef READER_BUFFER_SIZE
#undef READER_BUFFER_SIZE
#endif
#define READER_BUFFER_SIZE (512)

static lv_obj_t *ui_root = NULL;

// 页面容器
static lv_obj_t *ui_list_page = NULL;
static lv_obj_t *ui_read_page = NULL;

// 列表页组件
static lv_obj_t *ui_file_list = NULL;

// 阅读页组件
static lv_obj_t *ui_title_label = NULL;
static lv_obj_t *ui_content_label = NULL;
static lv_obj_t *ui_text_scroller = NULL;
static lv_obj_t *ui_progress_label = NULL; // 进度显示

// 状态变量
static char *text_buffer = NULL;
static char current_filename[128];
static uint32_t current_offset = 0;
static uint32_t total_file_size = 0;

// --- 辅助函数：加载当前页 ---
static void load_current_page(void)
{
    if (!text_buffer)
        return;

    // 读取文件块
    int len = Reader_Core_ReadFile(current_filename, current_offset, text_buffer, READER_BUFFER_SIZE);

    if (len > 0)
    {
        lv_label_set_text(ui_content_label, text_buffer);
    }
    else
    {
        lv_label_set_text(ui_content_label, "End of file or Read Error.");
    }

    // 更新进度显示
    if (total_file_size > 0)
    {
        // 计算当前是第几“块”
        int current_page_idx = (current_offset / READER_BUFFER_SIZE) + 1;
        int total_pages = (total_file_size + READER_BUFFER_SIZE - 1) / READER_BUFFER_SIZE;
        int percent = (current_offset * 100) / total_file_size;

        // 显示百分比和页码块信息
        lv_label_set_text_fmt(ui_progress_label, "%d%% (Page %d/%d)",
                              percent, current_page_idx, total_pages);
    }

    // 滚回顶部
    lv_obj_scroll_to_y(ui_text_scroller, 0, LV_ANIM_OFF);
}

// --- 事件回调 ---

static void prev_page_cb(lv_event_t *e)
{
    (void)e;
    if (current_offset >= READER_BUFFER_SIZE)
    {
        // 简单的回退逻辑：回退一个缓冲区大小
        current_offset -= READER_BUFFER_SIZE;
    }
    else
    {
        current_offset = 0;
    }
    load_current_page();
}

static void next_page_cb(lv_event_t *e)
{
    (void)e;
    if (current_offset + READER_BUFFER_SIZE < total_file_size)
    {
        current_offset += READER_BUFFER_SIZE;
        load_current_page();
    }
    else
    {
        lv_obj_t *msg = lv_msgbox_create(NULL, "Info", "Already at the last page.", NULL, true);
        lv_obj_center(msg);
    }
}

static void exit_btn_cb(lv_event_t *e)
{
    (void)e;
    Switch_To_App(&LauncherApp);
}

static void back_to_list_cb(lv_event_t *e)
{
    (void)e;
    lv_obj_add_flag(ui_read_page, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ui_list_page, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(ui_content_label, "");

    // 重置状态
    current_offset = 0;
    total_file_size = 0;
}

static void file_btn_click_handler(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *label = lv_obj_get_child(btn, 0);
    const char *filename = lv_label_get_text(label);

    // 保存文件名
    strncpy(current_filename, filename, sizeof(current_filename) - 1);

    // 获取文件大小
    total_file_size = Reader_Core_GetFileSize(current_filename);
    current_offset = 0;

    // 切换界面
    lv_obj_add_flag(ui_list_page, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ui_read_page, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(ui_title_label, current_filename);

    // 分配内存 (如果之前分配的大小不同，这里最好重新分配，或者确保 READER_BUFFER_SIZE 不变)
    if (text_buffer == NULL)
    {
        text_buffer = lv_mem_alloc(READER_BUFFER_SIZE + 1); // +1 for safety null terminator
    }

    // 加载第一页
    if (text_buffer)
    {
        load_current_page();
    }
    else
    {
        lv_label_set_text(ui_content_label, "Memory allocation failed.");
    }
}

static void on_file_found_add_btn(const char *filename, void *user_data)
{
    (void)user_data;
    lv_obj_t *btn = lv_btn_create(ui_file_list);
    lv_obj_set_width(btn, lv_pct(95));
    lv_obj_set_height(btn, 60);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x444444), 0);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, filename);

    lv_font_t *font = lv_font_qspi_get_by_size(24);
    if (font == NULL)
        font = &lv_font_montserrat_24;

    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_center(label);

    lv_obj_add_event_cb(btn, file_btn_click_handler, LV_EVENT_CLICKED, NULL);
}

// --- UI 创建 ---

void Reader_UI_Create(void)
{
    lv_obj_t *scr = lv_scr_act();

    ui_root = lv_obj_create(scr);
    lv_obj_set_size(ui_root, 800, 480);
    lv_obj_set_style_bg_color(ui_root, lv_color_hex(0x222222), 0);
    lv_obj_set_style_border_width(ui_root, 0, 0);
    lv_obj_set_style_pad_all(ui_root, 0, 0);

    // ================= 列表页 (保持不变) =================
    ui_list_page = lv_obj_create(ui_root);
    lv_obj_set_size(ui_list_page, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(ui_list_page, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ui_list_page, 0, 0);
    lv_obj_clear_flag(ui_list_page, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *list_title = lv_label_create(ui_list_page);
    lv_label_set_text(list_title, "My Books (S:/mytxt)");
    lv_font_t *font_24 = lv_font_qspi_get_by_size(24);
    lv_obj_set_style_text_font(list_title, font_24, 0);
    lv_obj_set_style_text_color(list_title, lv_color_white(), 0);
    lv_obj_align(list_title, LV_ALIGN_TOP_MID, 0, 20);

    lv_obj_t *exit_btn = lv_btn_create(ui_list_page);
    lv_obj_set_size(exit_btn, 60, 60);
    lv_obj_align(exit_btn, LV_ALIGN_TOP_RIGHT, -20, 10);
    lv_obj_set_style_bg_color(exit_btn, lv_color_hex(0xcc4444), 0);
    lv_obj_add_event_cb(exit_btn, exit_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *exit_lbl = lv_label_create(exit_btn);
    lv_label_set_text(exit_lbl, LV_SYMBOL_POWER);
    lv_obj_center(exit_lbl);

    ui_file_list = lv_obj_create(ui_list_page);
    lv_obj_set_size(ui_file_list, 760, 350);
    lv_obj_align(ui_file_list, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_flex_flow(ui_file_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(ui_file_list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_color(ui_file_list, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_width(ui_file_list, 0, 0);
    lv_obj_set_style_pad_row(ui_file_list, 10, 0);

    Reader_Core_ScanDir(on_file_found_add_btn, NULL);

    // ================= 阅读页=================
    ui_read_page = lv_obj_create(ui_root);
    lv_obj_set_size(ui_read_page, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(ui_read_page, lv_color_hex(0xF5F5DC), 0);
    lv_obj_set_style_border_width(ui_read_page, 0, 0);
    lv_obj_add_flag(ui_read_page, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ui_read_page, LV_OBJ_FLAG_SCROLLABLE); // 禁用页面滚动

    // 1. 顶部栏
    lv_obj_t *top_bar = lv_obj_create(ui_read_page);
    lv_obj_set_size(top_bar, lv_pct(100), 50);
    lv_obj_align(top_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(top_bar, lv_color_hex(0xDDCBA0), 0);
    lv_obj_set_style_border_width(top_bar, 0, 0);
    lv_obj_clear_flag(top_bar, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *back_btn = lv_btn_create(top_bar);
    lv_obj_set_size(back_btn, 80, 40);
    lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x8B4513), 0);
    lv_obj_add_event_cb(back_btn, back_to_list_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, LV_SYMBOL_LEFT " Back");
    lv_obj_center(back_lbl);

    ui_title_label = lv_label_create(top_bar);
    lv_label_set_text(ui_title_label, "Filename.txt");
    lv_obj_set_style_text_color(ui_title_label, lv_color_black(), 0);
    lv_obj_align(ui_title_label, LV_ALIGN_CENTER, 0, 0);

    // 给标题设置支持中文的字体
    lv_font_t *font_title = lv_font_qspi_get_by_size(24);
    if (font_title)
        lv_obj_set_style_text_font(ui_title_label, font_title, 0);

    // 2. 底部控制栏 (新增)
    lv_obj_t *bottom_bar = lv_obj_create(ui_read_page);
    lv_obj_set_size(bottom_bar, lv_pct(100), 60);
    lv_obj_align(bottom_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(bottom_bar, lv_color_hex(0xDDCBA0), 0);
    lv_obj_set_style_border_width(bottom_bar, 0, 0);
    lv_obj_clear_flag(bottom_bar, LV_OBJ_FLAG_SCROLLABLE);

    // 上一页按钮
    lv_obj_t *prev_btn = lv_btn_create(bottom_bar);
    lv_obj_set_size(prev_btn, 100, 40);
    lv_obj_align(prev_btn, LV_ALIGN_LEFT_MID, 20, 0);
    lv_obj_set_style_bg_color(prev_btn, lv_color_hex(0x8B4513), 0);
    lv_obj_add_event_cb(prev_btn, prev_page_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *prev_lbl = lv_label_create(prev_btn);
    lv_label_set_text(prev_lbl, "Prev");
    lv_obj_center(prev_lbl);

    // 下一页按钮
    lv_obj_t *next_btn = lv_btn_create(bottom_bar);
    lv_obj_set_size(next_btn, 100, 40);
    lv_obj_align(next_btn, LV_ALIGN_RIGHT_MID, -20, 0);
    lv_obj_set_style_bg_color(next_btn, lv_color_hex(0x8B4513), 0);
    lv_obj_add_event_cb(next_btn, next_page_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *next_lbl = lv_label_create(next_btn);
    lv_label_set_text(next_lbl, "Next");
    lv_obj_center(next_lbl);

    // 进度文本
    ui_progress_label = lv_label_create(bottom_bar);
    lv_label_set_text(ui_progress_label, "0%");
    lv_obj_set_style_text_color(ui_progress_label, lv_color_black(), 0);
    lv_obj_align(ui_progress_label, LV_ALIGN_CENTER, 0, 0);

    // 3. 文本滚动区域
    ui_text_scroller = lv_obj_create(ui_read_page);
    lv_obj_set_size(ui_text_scroller, 780, 320);             // 高度减小
    lv_obj_align(ui_text_scroller, LV_ALIGN_TOP_MID, 0, 55); // 位于顶部栏下方
    lv_obj_set_style_bg_opa(ui_text_scroller, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ui_text_scroller, 0, 0);

    // 禁用滚动
    lv_obj_set_scroll_dir(ui_text_scroller, LV_DIR_VER);

    ui_content_label = lv_label_create(ui_text_scroller);
    // 使用百分比宽度，减去一点边距，确保不会触发水平滚动
    lv_obj_set_width(ui_content_label, lv_pct(95));
    lv_label_set_long_mode(ui_content_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(ui_content_label, lv_color_black(), 0);

    lv_font_t *font_content = lv_font_qspi_get_by_size(24);
    if (font_content)
        lv_obj_set_style_text_font(ui_content_label, font_content, 0);

    lv_label_set_text(ui_content_label, "");
}

void Reader_UI_Delete(void)
{
    if (ui_root != NULL)
    {
        lv_obj_del(ui_root);
        ui_root = NULL;
        ui_list_page = NULL;
        ui_read_page = NULL;
    }

    if (text_buffer != NULL)
    {
        lv_mem_free(text_buffer);
        text_buffer = NULL;
    }
}
