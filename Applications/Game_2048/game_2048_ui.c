#include "game_2048_ui.h"
#include "game_2048_core.h"
#include "launcher_task.h" // 为了引用 LauncherApp
#include "system_task.h"
#include <stdio.h>
#include <string.h>

static lv_obj_t *ui_root = NULL;
static lv_obj_t *ui_grid_cont = NULL;
static lv_obj_t *ui_score_label = NULL;
static lv_obj_t *ui_tiles[4][4];

// 颜色配置
static uint32_t get_tile_color(uint16_t val)
{
    switch (val)
    {
    case 0:
        return 0xCDC1B4; // 空格颜色
    case 2:
        return 0xEEE4DA;
    case 4:
        return 0xEDE0C8;
    case 8:
        return 0xF2B179;
    case 16:
        return 0xF59563;
    case 32:
        return 0xF67C5F;
    case 64:
        return 0xF65E3B;
    case 128:
        return 0xEDCF72;
    case 256:
        return 0xEDCC61;
    case 512:
        return 0xEDC850;
    case 1024:
        return 0xEDC53F;
    case 2048:
        return 0xEDC22E;
    default:
        return 0x3C3A32;
    }
}

static uint32_t get_text_color(uint16_t val)
{
    return (val <= 4) ? 0x776E65 : 0xF9F6F2;
}

// 刷新界面
void Game2048_UI_Update(void)
{
    if (!ui_root)
        return;

    // 更新分数
    lv_label_set_text_fmt(ui_score_label, "SCORE\n%d", Game2048_Core_GetScore());

    // 更新格子
    for (int y = 0; y < 4; y++)
    {
        for (int x = 0; x < 4; x++)
        {
            uint16_t val = Game2048_Core_GetTile(x, y);
            lv_obj_t *tile = ui_tiles[y][x];

            lv_obj_set_style_bg_color(tile, lv_color_hex(get_tile_color(val)), 0);

            lv_obj_t *label = lv_obj_get_child(tile, 0);
            if (val > 0)
            {
                lv_label_set_text_fmt(label, "%d", val);
                lv_obj_set_style_text_color(label, lv_color_hex(get_text_color(val)), 0);
            }
            else
            {
                lv_label_set_text(label, "");
            }
        }
    }
}

// 弹窗事件处理
static void game_over_event_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    const char *txt = lv_msgbox_get_active_btn_text(obj);

    if (txt)
    {
        if (strcmp(txt, "Restart") == 0)
        {
            Game2048_Core_Reset();
            Game2048_UI_Update();
        }
        else if (strcmp(txt, "Quit") == 0)
        {
            Switch_To_App(&LauncherApp);
        }
        lv_msgbox_close(obj);
    }
}

static void show_game_over_dialog(void)
{
    static const char *btns[] = {"Restart", "Quit", ""};

    // 创建消息框
    lv_obj_t *mbox = lv_msgbox_create(ui_root, "Game Over", "No more moves available!", btns, true);
    lv_obj_center(mbox);

    // 样式美化
    lv_obj_set_style_bg_color(mbox, lv_color_hex(0xFAF8EF), 0);
    lv_obj_set_style_text_color(mbox, lv_color_hex(0x776E65), 0);
    lv_obj_set_style_text_font(mbox, &lv_font_montserrat_20, 0);

    // 按钮样式
    lv_obj_t *btns_obj = lv_msgbox_get_btns(mbox);
    lv_obj_set_style_bg_color(btns_obj, lv_color_hex(0x8f7a66), LV_PART_ITEMS);

    lv_obj_add_event_cb(mbox, game_over_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
}

// 手势事件回调
static void grid_gesture_cb(lv_event_t *e)
{
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    bool moved = false;

    if (dir == LV_DIR_LEFT)
        moved = Game2048_Core_Move(DIR_LEFT);
    else if (dir == LV_DIR_RIGHT)
        moved = Game2048_Core_Move(DIR_RIGHT);
    else if (dir == LV_DIR_TOP)
        moved = Game2048_Core_Move(DIR_DOWN);
    else if (dir == LV_DIR_BOTTOM)
        moved = Game2048_Core_Move(DIR_UP);

    if (moved)
    {
        Game2048_UI_Update();
        if (Game2048_Core_IsGameOver())
        {
            show_game_over_dialog();
        }
    }
}

// 退出按钮回调
static void close_btn_cb(lv_event_t *e)
{
    // 切换回 Launcher
    Switch_To_App(&LauncherApp);
}

void Game2048_UI_Create(void)
{
    lv_obj_t *scr = lv_scr_act();

    // 1. 根容器 (全屏)
    ui_root = lv_obj_create(scr);
    lv_obj_set_size(ui_root, 800, 480);

    // 背景透明，让 Launcher 的壁纸透出来
    lv_obj_set_style_bg_opa(ui_root, LV_OPA_TRANSP, 0);

    // 去除边框和圆角，防止边缘出现线条
    lv_obj_set_style_border_width(ui_root, 0, 0);
    lv_obj_set_style_radius(ui_root, 0, 0);
    lv_obj_set_style_pad_all(ui_root, 0, 0);
    lv_obj_clear_flag(ui_root, LV_OBJ_FLAG_SCROLLABLE);

    // 2. 顶部栏
    // 分数框
    lv_obj_t *score_box = lv_obj_create(ui_root);
    lv_obj_set_size(score_box, 120, 60);
    lv_obj_align(score_box, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_bg_color(score_box, lv_color_hex(0xBBADA0), 0);
    lv_obj_set_style_radius(score_box, 5, 0);
    lv_obj_clear_flag(score_box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(score_box, 0, 0);

    ui_score_label = lv_label_create(score_box);
    lv_obj_center(ui_score_label);
    lv_obj_set_style_text_align(ui_score_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(ui_score_label, lv_color_white(), 0);
    lv_label_set_text(ui_score_label, "SCORE\n0");

    // 退出按钮 (右上角 X)
    lv_obj_t *close_btn = lv_btn_create(ui_root);
    lv_obj_set_size(close_btn, 50, 50);
    lv_obj_align(close_btn, LV_ALIGN_TOP_RIGHT, -20, 20);
    lv_obj_set_style_bg_color(close_btn, lv_color_hex(0x8f7a66), 0);
    lv_obj_set_style_shadow_width(close_btn, 0, 0); // 去除按钮阴影
    lv_obj_add_event_cb(close_btn, close_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *close_label = lv_label_create(close_btn);
    lv_label_set_text(close_label, LV_SYMBOL_CLOSE);
    lv_obj_center(close_label);

    // 3. 游戏棋盘区域
    ui_grid_cont = lv_obj_create(ui_root);
    lv_obj_set_size(ui_grid_cont, 390, 390);
    lv_obj_align(ui_grid_cont, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(ui_grid_cont, lv_color_hex(0xBBADA0), 0);
    lv_obj_set_style_radius(ui_grid_cont, 20, 0);
    lv_obj_set_style_border_width(ui_grid_cont, 0, 0); // 确保无边框
    lv_obj_clear_flag(ui_grid_cont, LV_OBJ_FLAG_SCROLLABLE);

    // 启用手势识别
    lv_obj_clear_flag(ui_grid_cont, LV_OBJ_FLAG_GESTURE_BUBBLE); // 阻止冒泡
    lv_obj_add_event_cb(ui_grid_cont, grid_gesture_cb, LV_EVENT_GESTURE, NULL);

    // 创建 4x4 网格
    const int tile_size = 78;
    const int gap = 8;

    for (int y = 0; y < 4; y++)
    {
        for (int x = 0; x < 4; x++)
        {
            lv_obj_t *tile = lv_obj_create(ui_grid_cont);
            lv_obj_set_size(tile, tile_size, tile_size);
            // 绝对定位
            lv_obj_set_pos(tile, gap + x * (tile_size + gap), gap + y * (tile_size + gap));
            lv_obj_set_style_radius(tile, 5, 0);
            lv_obj_set_style_border_width(tile, 0, 0);
            lv_obj_clear_flag(tile, LV_OBJ_FLAG_SCROLLABLE);
            // 让点击/手势穿透到父容器
            lv_obj_clear_flag(tile, LV_OBJ_FLAG_CLICKABLE);

            lv_obj_t *num = lv_label_create(tile);
            lv_obj_center(num);
            lv_obj_set_style_text_font(num, &lv_font_montserrat_24, 0);

            ui_tiles[y][x] = tile;
        }
    }

    // 初始刷新
    Game2048_UI_Update();
}

void Game2048_UI_Delete(void)
{
    if (ui_root != NULL)
    {
        lv_obj_del(ui_root);
        ui_root = NULL;
    }
}
