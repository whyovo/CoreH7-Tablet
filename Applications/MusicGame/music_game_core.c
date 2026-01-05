#include "music_game_core.h"
#include "lvgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "config.h"

// 私有数据
static SongInfo_t song_list[MAX_SONGS];
static int song_count = 0;

// 全局设置定义
int music_game_speed = 12; // 默认流速

// --- 缓冲读取器结构 ---
typedef struct
{
    lv_fs_file_t *f;
    uint8_t buffer[512]; // 512字节缓冲区
    uint32_t buf_pos;
    uint32_t buf_len;
    bool eof;
} FileReader_t;

// --- 内部辅助函数 ---

// 从缓冲区读取一个字符 (大幅提升速度)
static char buffered_getc(FileReader_t *fr)
{
    if (fr->buf_pos >= fr->buf_len)
    {
        if (fr->eof)
            return 0;
        lv_fs_res_t res = lv_fs_read(fr->f, fr->buffer, sizeof(fr->buffer), &fr->buf_len);
        if (res != LV_FS_RES_OK || fr->buf_len == 0)
        {
            fr->eof = true;
            return 0;
        }
        fr->buf_pos = 0;
    }
    return fr->buffer[fr->buf_pos++];
}

static char *trim_spaces(char *str)
{
    if (!str)
        return NULL;
    while (isspace((unsigned char)*str))
        str++;
    if (*str == 0)
        return str;
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        end--;
    *(end + 1) = 0;
    return str;
}

static void parse_osu_file(const char *path, SongInfo_t *info)
{
    // 1. 初始化默认值
    info->stars = 0.0f;
    info->bpm = 0.0f;
    info->duration = 0;
    info->short_notes = 0;
    info->long_notes = 0;
    strcpy(info->title, "Unknown Title");
    strcpy(info->artist, "Unknown Artist");
    strcpy(info->creator, "Unknown Creator");
    strcpy(info->version, "Normal");

    // 2. 打开文件
    lv_fs_file_t f;
    lv_fs_res_t res = lv_fs_open(&f, path, LV_FS_MODE_RD);
    if (res != LV_FS_RES_OK)
    {
        DEBUG_INFO("[Core] Failed to open .osu: %s", path);
        return;
    }

    // 初始化缓冲读取器
    FileReader_t fr;
    fr.f = &f;
    fr.buf_pos = 0;
    fr.buf_len = 0;
    fr.eof = false;

    char line[512];
    // DEBUG_INFO("[Core] Parsing: %s", path);

    while (1)
    {
        // 使用缓冲读取替代 lv_fs_read
        int line_idx = 0;
        char c;
        while (line_idx < 511)
        {
            c = buffered_getc(&fr);
            if (c == 0)
                break; // EOF
            if (c == '\n')
                break;
            line[line_idx++] = c;
        }
        line[line_idx] = '\0';

        if (line_idx == 0 && c == 0)
            break; // EOF

        char *clean_line = trim_spaces(line);
        if (clean_line[0] == '\0' || (clean_line[0] == '/' && clean_line[1] == '/'))
            continue;

        // 检测到非元数据章节立即停止 ---
        if (clean_line[0] == '[')
        {
            if (strncmp(clean_line, "[Events]", 8) == 0 ||
                strncmp(clean_line, "[TimingPoints]", 14) == 0 ||
                strncmp(clean_line, "[HitObjects]", 12) == 0)
            {
                // DEBUG_INFO("[Core] Reached data section, stopping parse.");
                break;
            }
        }

        char *colon_pos = strchr(clean_line, ':');
        if (colon_pos)
        {
            *colon_pos = '\0';
            char *key = trim_spaces(clean_line);
            char *val = trim_spaces(colon_pos + 1);

            if (strcmp(key, "Title") == 0)
                strcpy(info->title, val);
            else if (strcmp(key, "Artist") == 0)
                strcpy(info->artist, val);
            else if (strcmp(key, "Creator") == 0)
                strcpy(info->creator, val);
            else if (strcmp(key, "Version") == 0)
                strcpy(info->version, val);

            else if (strcmp(key, "Stars") == 0)
            {
                float f_val;
                if (sscanf(val, "%f", &f_val) == 1)
                    info->stars = f_val;
            }
            else if (strcmp(key, "BPM") == 0)
            {
                float f_val;
                if (sscanf(val, "%f", &f_val) == 1)
                    info->bpm = f_val;
            }
            else if (strcmp(key, "Duration") == 0)
            {
                int i_val;
                if (sscanf(val, "%d", &i_val) == 1)
                    info->duration = i_val;
            }
            else if (strcmp(key, "ShortNotes") == 0)
            {
                int i_val;
                if (sscanf(val, "%d", &i_val) == 1)
                    info->short_notes = i_val;
            }
            else if (strcmp(key, "LongNotes") == 0)
            {
                int i_val;
                if (sscanf(val, "%d", &i_val) == 1)
                    info->long_notes = i_val;
            }
        }
    }
    lv_fs_close(&f);
}

// --- 公开 API 实现 ---

void MusicGame_Core_Init(void)
{
    song_count = 0;
    memset(song_list, 0, sizeof(song_list));
}

void MusicGame_Core_ScanSongs(void)
{
    song_count = 0;
    lv_fs_dir_t dir;
    lv_fs_res_t res = lv_fs_dir_open(&dir, "S:/music_game/songs");

    if (res != LV_FS_RES_OK)
    {
        DEBUG_INFO("[Core] Error: Cannot open S:/music_game/songs");
        return;
    }

    char fn[256];
    while (song_count < MAX_SONGS)
    {
        res = lv_fs_dir_read(&dir, fn);
        if (res != LV_FS_RES_OK || fn[0] == '\0')
            break;
        if (strcmp(fn, ".") == 0 || strcmp(fn, "..") == 0)
            continue;

        char *fn_ptr = fn;
        if (fn_ptr[0] == '/')
            fn_ptr++;

        // DEBUG_INFO("[Core] Checking: %s", fn_ptr);
        strcpy(song_list[song_count].folder_name, fn_ptr);

        char sub_path[256];
        snprintf(sub_path, sizeof(sub_path), "S:/music_game/songs/%s", fn_ptr);

        lv_fs_dir_t sub_dir;
        if (lv_fs_dir_open(&sub_dir, sub_path) == LV_FS_RES_OK)
        {
            char sub_fn[256];
            bool found_osu = false;

            while (1)
            {
                res = lv_fs_dir_read(&sub_dir, sub_fn);
                if (res != LV_FS_RES_OK || sub_fn[0] == '\0')
                    break;

                char *sub_fn_ptr = sub_fn;
                if (sub_fn_ptr[0] == '/')
                    sub_fn_ptr++;
                int len = strlen(sub_fn_ptr);

                if (len > 4)
                {
                    if (strcmp(&sub_fn_ptr[len - 4], ".osu") == 0)
                    {
                        // 1. 仅保存文件名，节省内存
                        strncpy(song_list[song_count].osu_filename, sub_fn_ptr, sizeof(song_list[song_count].osu_filename) - 1);
                        
                        // 2. 临时构建完整路径用于解析 (解析完即丢弃)
                        char temp_path[256];
                        snprintf(temp_path, sizeof(temp_path), "%s/%s", sub_path, sub_fn_ptr);
                        
                        parse_osu_file(temp_path, &song_list[song_count]);
                        found_osu = true;
                    }
                    
                }
            }
            lv_fs_dir_close(&sub_dir);

            if (found_osu)
            {
                // 打印关键信息确认解析结果
                // DEBUG_INFO("  -> Added: %s (Stars: %.2f)", song_list[song_count].title, song_list[song_count].stars);
                song_count++;
            }
        }
    }
    lv_fs_dir_close(&dir);
    // DEBUG_INFO("[Core] Scan finished. Total: %d", song_count);
}

int MusicGame_Core_GetSongCount(void) { return song_count; }
SongInfo_t *MusicGame_Core_GetSongInfo(int index)
{
    if (index >= 0 && index < song_count)
        return &song_list[index];
    return NULL;
}
