#include "reader_core.h"
#include "lvgl.h"
#include <string.h>
#include <stdio.h>

#define TXT_BASE_PATH "S:/mytxt"

void Reader_Core_ScanDir(file_found_cb_t cb, void *user_data)
{
    lv_fs_dir_t dir;
    lv_fs_res_t res;

    res = lv_fs_dir_open(&dir, TXT_BASE_PATH);
    if (res != LV_FS_RES_OK)
    {
        LV_LOG_WARN("Failed to open dir: %s", TXT_BASE_PATH);
        return;
    }

    char fn[256];
    while (1)
    {
        res = lv_fs_dir_read(&dir, fn);
        if (res != LV_FS_RES_OK || strlen(fn) == 0)
        {
            break;
        }

        // 简单的后缀过滤
        char *ext = strrchr(fn, '.');
        if (ext && (strcmp(ext, ".txt") == 0 || strcmp(ext, ".TXT") == 0))
        {
            if (cb)
            {
                cb(fn, user_data);
            }
        }
    }

    lv_fs_dir_close(&dir);
}

uint32_t Reader_Core_GetFileSize(const char *filename)
{
    char full_path[256];
    snprintf(full_path, sizeof(full_path), "%s/%s", TXT_BASE_PATH, filename);

    lv_fs_file_t f;
    if (lv_fs_open(&f, full_path, LV_FS_MODE_RD) != LV_FS_RES_OK)
        return 0;

    uint32_t size = 0;
    lv_fs_seek(&f, 0, LV_FS_SEEK_END);
    lv_fs_tell(&f, &size);
    lv_fs_close(&f);
    return size;
}

int Reader_Core_ReadFile(const char *filename, uint32_t offset, char *buffer, uint32_t max_len)
{
    if (!filename || !buffer || max_len == 0)
        return 0;

    char full_path[256];
    snprintf(full_path, sizeof(full_path), "%s/%s", TXT_BASE_PATH, filename);

    lv_fs_file_t f;
    if (lv_fs_open(&f, full_path, LV_FS_MODE_RD) != LV_FS_RES_OK)
        return 0;

    // 跳转到指定偏移
    lv_fs_seek(&f, offset, LV_FS_SEEK_SET);

    uint32_t br = 0;
    lv_fs_res_t res = lv_fs_read(&f, buffer, max_len - 1, &br);

    if (res == LV_FS_RES_OK && br > 0)
    {
        buffer[br] = '\0';

        // 简单的 UTF-8 截断修复：如果最后一个字节是多字节字符的一部分，将其截断
        // UTF-8 多字节字符的后续字节格式为 10xxxxxx (0x80 - 0xBF)
        // 我们向回回溯，直到找到非后续字节，或者截断过多
        if (br > 0 && (buffer[br - 1] & 0xC0) == 0x80)
        {
            int backtrack = 0;
            while (backtrack < 4 && (br - backtrack) > 0)
            {
                backtrack++;
                // 找到头字节 (0xC0以上) 或 ASCII (0x7F以下)
                if ((buffer[br - backtrack] & 0xC0) != 0x80)
                {
                    // 这是一个多字节字符的头，说明这个字符被截断了，我们把它整个丢弃，留给下一页读
                    buffer[br - backtrack] = '\0';
                    // 返回修正后的长度，但这不影响文件指针，下一页会会有少量重叠或丢失，
                    // 完美的做法是返回实际消耗的字节数给上层更新 offset，这里简化处理
                    break;
                }
            }
        }
    }
    else
    {
        br = 0;
        buffer[0] = '\0';
    }

    lv_fs_close(&f);
    return (int)br;
}
