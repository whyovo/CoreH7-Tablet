#include "music_game_ui.h"
#include "music_game_core.h" // 引入 Core
#include "launcher_task.h"
#include "system_task.h"
#include "dspeaker.h"
#include "audio_player.h"
#include "lvgl.h"
#include <stdio.h>
#include <string.h>
#include "config.h"

static lv_obj_t *ui_root = NULL;
static lv_obj_t *ui_bg_img = NULL;
static lv_obj_t *ui_info_cont = NULL;
static lv_obj_t *ui_song_roller = NULL;
static lv_obj_t *ui_settings_panel = NULL; // 设置面板句柄

// 音频播放器实例
static AudioPlayer_t bgm_player;

// 信息标签
static lv_obj_t *lbl_folder;
static lv_obj_t *lbl_title_artist;
static lv_obj_t *lbl_creator;
static lv_obj_t *lbl_stats;

// --- UI 更新逻辑 ---
static void update_ui_info(int index)
{
    SongInfo_t *s = MusicGame_Core_GetSongInfo(index);
    if (s == NULL)
    {
        DEBUG_INFO("[UI] Error: Invalid song index %d", index);
        return;
    }

    // // 1. 停止当前播放，释放文件系统资源
    // AudioPlayer_Stop(&bgm_player);
    // AudioPlayer_CloseFile(&bgm_player);

    // --- 调试打印 ---
    // DEBUG_INFO("[UI] Update Info for #%d:", index);
    // DEBUG_INFO("     Title: %s", s->title);
    // DEBUG_INFO("     Stars: %.2f, BPM: %.1f", s->stars, s->bpm);

    // 2. 进行 UI 更新和图片加载 (文件 I/O 操作)

    // 更新背景 (动态构建 bg.jpg 路径)
    char bg_path[256];
    snprintf(bg_path, sizeof(bg_path), "S:/music_game/songs/%s/bg.jpg", s->folder_name);
    lv_img_set_src(ui_bg_img, bg_path);

    // 更新左侧文本
    lv_label_set_text_fmt(lbl_folder, "Folder: %s", s->folder_name);
    lv_label_set_text_fmt(lbl_title_artist, "%s\n%s", s->title, s->artist);
    lv_label_set_text_fmt(lbl_creator, "Mapper: %s\n%s", s->creator, s->version);
    lv_label_set_text_fmt(lbl_stats,
                          "Stars: %.2f\n"
                          "BPM: %.1f\n"
                          "Length: %ds\n"
                          "Notes: %d\n"
                          "L.Notes: %d",
                          s->stars, s->bpm, s->duration, s->short_notes, s->long_notes);

    // // 3. 启动音频播放
    // // 检查音频文件是否存在
    // char lv_audio_path[256];
    // snprintf(lv_audio_path, sizeof(lv_audio_path), "S:/music_game/songs/%s/audio.wav", s->folder_name);

    // lv_fs_file_t f;
    // if (lv_fs_open(&f, lv_audio_path, LV_FS_MODE_RD) == LV_FS_RES_OK)
    // {
    //     lv_fs_close(&f);

    //     // 转换路径给 AudioPlayer (S: -> 0:)
    //     char fatfs_path[256];
    //     snprintf(fatfs_path, sizeof(fatfs_path), "0:/music_game/songs/%s/audio.wav", s->folder_name);

    //     if (AudioPlayer_OpenFile(&bgm_player, fatfs_path) == HAL_OK)
    //     {
    //         AudioPlayer_PlayWithLoop(&bgm_player, 0); // 0 = 无限循环
    //     }
    // }
    // else
    // {
    //     DEBUG_INFO("[UI] Audio not found (skip): %s", lv_audio_path);
    // }
}

// --- 事件回调：滚轮选择 ---
static void song_select_event_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED)
    {
        int idx = lv_roller_get_selected(obj);
        update_ui_info(idx);
    }
}

// --- 退出回调 ---
static void close_btn_cb(lv_event_t *e)
{
    Switch_To_App(&LauncherApp);
}

// --- 设置面板相关回调 ---

static void settings_close_cb(lv_event_t *e)
{
    if (ui_settings_panel)
    {
        lv_obj_del(ui_settings_panel);
        ui_settings_panel = NULL;
    }
}

static void volume_slider_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    int val = (int)lv_slider_get_value(slider);
    DSPEAKER_SetVolume((uint8_t)val);

    // 更新百分比标签 (User data 传入 label)
    lv_obj_t *label = (lv_obj_t *)lv_event_get_user_data(e);
    if (label)
        lv_label_set_text_fmt(label, "%d%%", val);
}

static void speed_slider_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    int val = (int)lv_slider_get_value(slider);
    music_game_speed = val;

    // 更新数值标签
    lv_obj_t *label = (lv_obj_t *)lv_event_get_user_data(e);
    if (label)
        lv_label_set_text_fmt(label, "%d", val);
}

// --- 创建设置面板 ---
static void create_settings_panel(void)
{
    if (ui_settings_panel)
        return; // 防止重复创建

    // 1. 半透明遮罩背景 (作为模态窗口)
    ui_settings_panel = lv_obj_create(ui_root);
    lv_obj_set_size(ui_settings_panel, 400, 320);
    lv_obj_center(ui_settings_panel);
    lv_obj_set_style_bg_color(ui_settings_panel, lv_color_hex(0x202020), 0);
    lv_obj_set_style_bg_opa(ui_settings_panel, LV_OPA_90, 0);
    lv_obj_set_style_border_color(ui_settings_panel, lv_color_hex(0x808080), 0);
    lv_obj_set_style_border_width(ui_settings_panel, 2, 0);
    lv_obj_set_flex_flow(ui_settings_panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(ui_settings_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(ui_settings_panel, 20, 0);
    lv_obj_set_style_pad_gap(ui_settings_panel, 15, 0);

    // 标题
    lv_obj_t *title = lv_label_create(ui_settings_panel);
    lv_label_set_text(title, "SETTINGS");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);

    // --- 音量控制 ---
    lv_obj_t *vol_cont = lv_obj_create(ui_settings_panel);
    lv_obj_set_size(vol_cont, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(vol_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(vol_cont, 0, 0);
    lv_obj_set_flex_flow(vol_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(vol_cont, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(vol_cont, 0, 0);

    lv_obj_t *vol_lbl = lv_label_create(vol_cont);
    lv_label_set_text(vol_lbl, "Volume");
    lv_obj_set_style_text_color(vol_lbl, lv_color_white(), 0);

    lv_obj_t *vol_val_lbl = lv_label_create(vol_cont);
    lv_obj_set_style_text_color(vol_val_lbl, lv_color_white(), 0);
    lv_obj_set_width(vol_val_lbl, 50);
    lv_obj_set_style_text_align(vol_val_lbl, LV_TEXT_ALIGN_RIGHT, 0);
    lv_label_set_text_fmt(vol_val_lbl, "%d%%", DSPEAKER_GetVolume());

    lv_obj_t *vol_slider = lv_slider_create(ui_settings_panel);
    lv_obj_set_width(vol_slider, lv_pct(100));
    lv_slider_set_range(vol_slider, 0, 100);
    lv_slider_set_value(vol_slider, DSPEAKER_GetVolume(), LV_ANIM_OFF);
    lv_obj_add_event_cb(vol_slider, volume_slider_cb, LV_EVENT_VALUE_CHANGED, vol_val_lbl);

    // --- 流速控制 ---
    lv_obj_t *speed_cont = lv_obj_create(ui_settings_panel);
    lv_obj_set_size(speed_cont, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(speed_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(speed_cont, 0, 0);
    lv_obj_set_flex_flow(speed_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(speed_cont, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(speed_cont, 0, 0);

    lv_obj_t *speed_lbl = lv_label_create(speed_cont);
    lv_label_set_text(speed_lbl, "Note Speed");
    lv_obj_set_style_text_color(speed_lbl, lv_color_white(), 0);

    lv_obj_t *speed_val_lbl = lv_label_create(speed_cont);
    lv_obj_set_style_text_color(speed_val_lbl, lv_color_white(), 0);
    lv_obj_set_width(speed_val_lbl, 50);
    lv_obj_set_style_text_align(speed_val_lbl, LV_TEXT_ALIGN_RIGHT, 0);
    lv_label_set_text_fmt(speed_val_lbl, "%d", music_game_speed);

    lv_obj_t *speed_slider = lv_slider_create(ui_settings_panel);
    lv_obj_set_width(speed_slider, lv_pct(100));
    lv_slider_set_range(speed_slider, 1, 30); // 假设速度范围 1-30
    lv_slider_set_value(speed_slider, music_game_speed, LV_ANIM_OFF);
    lv_obj_add_event_cb(speed_slider, speed_slider_cb, LV_EVENT_VALUE_CHANGED, speed_val_lbl);

    // --- 关闭按钮 ---
    lv_obj_t *close_btn = lv_btn_create(ui_settings_panel);
    lv_obj_set_size(close_btn, 100, 40);
    lv_obj_set_style_bg_color(close_btn, lv_color_hex(0xFF4040), 0);
    lv_obj_add_event_cb(close_btn, settings_close_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_lbl = lv_label_create(close_btn);
    lv_label_set_text(btn_lbl, "Close");
    lv_obj_center(btn_lbl);
}

// --- 设置按钮回调 (暂空) ---
static void settings_btn_cb(lv_event_t *e)
{
    DEBUG_INFO("[UI] Settings clicked");
    create_settings_panel();
}

// --- 开始按钮回调 (暂空) ---
static void start_btn_cb(lv_event_t *e)
{
    DEBUG_INFO("[UI] Start Game clicked");
    // TODO: Start game logic
}

void MusicGame_UI_Create(void)
{
    int count = MusicGame_Core_GetSongCount();

    memset(&bgm_player, 0, sizeof(bgm_player));

    lv_obj_t *scr = lv_scr_act();
    ui_root = lv_obj_create(scr);
    lv_obj_set_size(ui_root, 800, 480);
    lv_obj_set_style_bg_color(ui_root, lv_color_black(), 0);
    lv_obj_set_style_pad_all(ui_root, 0, 0);
    lv_obj_set_style_border_width(ui_root, 0, 0);
    lv_obj_clear_flag(ui_root, LV_OBJ_FLAG_SCROLLABLE);

    // 2. 背景层
    ui_bg_img = lv_img_create(ui_root);
    lv_obj_set_size(ui_bg_img, 800, 480);
    lv_obj_center(ui_bg_img);
    lv_obj_set_style_bg_color(ui_bg_img, lv_color_black(), 0); // 默认黑底

    // 3. 左侧信息面板 (半透明黑底)
    ui_info_cont = lv_obj_create(ui_root);
    lv_obj_set_size(ui_info_cont, 300, 480);
    lv_obj_align(ui_info_cont, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(ui_info_cont, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(ui_info_cont, LV_OPA_70, 0); // 70% 不透明度
    lv_obj_set_style_border_width(ui_info_cont, 0, 0);
    lv_obj_set_style_radius(ui_info_cont, 0, 0);
    lv_obj_set_flex_flow(ui_info_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(ui_info_cont, 20, 0);
    lv_obj_set_style_pad_gap(ui_info_cont, 15, 0);

    // --- 左侧内容 ---
    // 文件夹名
    lbl_folder = lv_label_create(ui_info_cont);
    lv_obj_set_width(lbl_folder, lv_pct(100));
    lv_label_set_long_mode(lbl_folder, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_font(lbl_folder, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_folder, lv_color_hex(0xAAAAAA), 0);

    // 标题和艺术家 (大字)
    lbl_title_artist = lv_label_create(ui_info_cont);
    lv_obj_set_width(lbl_title_artist, lv_pct(100));
    lv_obj_set_style_text_font(lbl_title_artist, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_title_artist, lv_color_white(), 0);

    // 作者和难度
    lbl_creator = lv_label_create(ui_info_cont);
    lv_obj_set_width(lbl_creator, lv_pct(100));
    lv_obj_set_style_text_font(lbl_creator, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_creator, lv_color_hex(0xDDDDDD), 0);

    // 分割线
    lv_obj_t *line = lv_obj_create(ui_info_cont);
    lv_obj_set_size(line, lv_pct(100), 2);
    lv_obj_set_style_bg_color(line, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(line, LV_OPA_50, 0);

    // 详细数据 (Stars, BPM...)
    lbl_stats = lv_label_create(ui_info_cont);
    lv_obj_set_width(lbl_stats, lv_pct(100));
    lv_obj_set_style_text_font(lbl_stats, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_stats, lv_color_hex(0xFFD700), 0); // 金色
    lv_obj_set_style_text_line_space(lbl_stats, 10, 0);

    // 4. 右侧歌曲选择滚轮 (Roller)
    ui_song_roller = lv_roller_create(ui_root);
    lv_obj_set_size(ui_song_roller, 480, 360);
    lv_obj_align(ui_song_roller, LV_ALIGN_RIGHT_MID, 0, 0);

    // 构建选项字符串
    static char roller_opts[MAX_SONGS * 128];
    roller_opts[0] = '\0';

    for (int i = 0; i < count; i++)
    {
        SongInfo_t *s = MusicGame_Core_GetSongInfo(i);
        if (s)
        {
            if (strlen(roller_opts) + strlen(s->folder_name) + 2 < sizeof(roller_opts))
            {
                strcat(roller_opts, s->folder_name);
                if (i < count - 1)
                    strcat(roller_opts, "\n");
            }
        }
    }
    lv_roller_set_options(ui_song_roller, roller_opts, LV_ROLLER_MODE_NORMAL);

    // 样式
    lv_obj_set_style_bg_color(ui_song_roller, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(ui_song_roller, LV_OPA_50, 0);
    lv_obj_set_style_text_color(ui_song_roller, lv_color_white(), 0);
    lv_obj_set_style_text_font(ui_song_roller, &lv_font_montserrat_12, 0);
    lv_obj_set_style_bg_color(ui_song_roller, lv_color_hex(0xFF0080), LV_PART_SELECTED);
    lv_obj_set_style_text_font(ui_song_roller, &lv_font_montserrat_14, LV_PART_SELECTED);

    lv_obj_add_event_cb(ui_song_roller, song_select_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // 5. 退出按钮 (右上角)
    lv_obj_t *close_btn = lv_btn_create(ui_root);
    lv_obj_set_size(close_btn, 50, 50);
    lv_obj_align(close_btn, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_bg_color(close_btn, lv_color_hex(0xFF4040), 0);
    lv_obj_add_event_cb(close_btn, close_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *close_label = lv_label_create(close_btn);
    lv_obj_set_style_text_font(close_label, &lv_font_montserrat_24, 0);
    lv_label_set_text(close_label, LV_SYMBOL_CLOSE);
    lv_obj_center(close_label);

    // 6. 设置按钮 (左上角)
    lv_obj_t *settings_btn = lv_btn_create(ui_root);
    lv_obj_set_size(settings_btn, 50, 50);
    lv_obj_align(settings_btn, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(settings_btn, lv_color_hex(0x808080), 0);
    lv_obj_set_style_bg_opa(settings_btn, LV_OPA_50, 0);
    lv_obj_add_event_cb(settings_btn, settings_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *settings_label = lv_label_create(settings_btn);
    lv_obj_set_style_text_font(settings_label, &lv_font_montserrat_24, 0);
    lv_label_set_text(settings_label, LV_SYMBOL_SETTINGS);
    lv_obj_center(settings_label);

    // 7. 开始按钮 (右下角)
    lv_obj_t *start_btn = lv_btn_create(ui_root);
    lv_obj_set_size(start_btn, 160, 60);
    lv_obj_align(start_btn, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_set_style_bg_color(start_btn, lv_color_hex(0x00D000), 0); // 绿色
    lv_obj_set_style_bg_opa(start_btn, LV_OPA_90, 0);
    lv_obj_set_style_shadow_width(start_btn, 20, 0);
    lv_obj_set_style_shadow_color(start_btn, lv_color_hex(0x00FF00), 0);
    lv_obj_add_event_cb(start_btn, start_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *start_label = lv_label_create(start_btn);
    lv_label_set_text(start_label, LV_SYMBOL_PLAY " PLAY");
    lv_obj_set_style_text_font(start_label, &lv_font_montserrat_20, 0);
    lv_obj_center(start_label);

    // 初始化显示
    if (count > 0)
    {
        lv_roller_set_selected(ui_song_roller, 0, LV_ANIM_OFF);
        update_ui_info(0);
    }
    else
    {
        lv_label_set_text(lbl_title_artist, "No Songs Found");
    }
}

void MusicGame_UI_Delete(void)
{
    if (ui_root != NULL)
    {
        // 停止播放并释放资源
        AudioPlayer_Stop(&bgm_player);
        AudioPlayer_CloseFile(&bgm_player);
        DSPEAKER_Stop();

        lv_obj_del(ui_root);
        ui_root = NULL;

        // 1. 重置所有静态指针，防止悬空引用
        ui_bg_img = NULL;
        ui_info_cont = NULL;
        ui_song_roller = NULL;
        ui_settings_panel = NULL; // 清除设置面板指针
        lbl_folder = NULL;
        lbl_title_artist = NULL;
        lbl_creator = NULL;
        lbl_stats = NULL;

        // 2.清理图片缓存，释放背景图占用的 RAM
        lv_img_cache_invalidate_src(NULL);
    }
}
