#include "file_logic.h"
#include <string.h>
#include <stdio.h>
#include "lvgl.h"
#include "ff.h"
#include "config.h"

static char current_path[MAX_PATH_LEN];
static FileInfo_t file_list[MAX_FILES_PER_DIR];
static char clipboard_path[MAX_PATH_LEN];
static bool clipboard_valid = false;

void FileLogic_Init(void)
{
    strcpy(current_path, "S:");
    clipboard_valid = false;
}

const char *FileLogic_GetPath(void)
{
    return current_path;
}

// 路径拼接辅助函数
static void path_append(char *path, const char *sub)
{
    int len = strlen(path);
    if (path[len - 1] != '/' && path[len - 1] != ':')
    {
        strcat(path, "/");
    }
    strcat(path, sub);
}

// 路径回退辅助函数
static void path_up(char *path)
{
    char *last_slash = strrchr(path, '/');
    if (last_slash)
    {
        *last_slash = '\0'; // 截断
        // 如果回退后是 "S:" 这种形式，保持原样，或者根据具体FS驱动调整
        if (strlen(path) == 2 && path[1] == ':')
        {
            // 已经是根目录
        }
    }
    else
    {
        // 可能是 "S:" 这种根目录，不做处理
    }
}

bool FileLogic_EnterDir(const char *dir_name)
{
    // 1. 构造目标路径
    char next_path[MAX_PATH_LEN];
    strcpy(next_path, current_path);
    path_append(next_path, dir_name);

    // 2. 验证路径是否有效 (尝试打开目录)
    // 这一步非常重要，防止进入无效路径导致列表为空且无法返回
    char temp_path[MAX_PATH_LEN];
    strcpy(temp_path, next_path);

    // 确保路径以 '/' 结尾，因为某些 FS 驱动对目录打开有此要求
    // (与 FileLogic_GetFileList 中的逻辑保持一致)
    int len = strlen(temp_path);
    if (len > 0 && temp_path[len - 1] != '/' && temp_path[len - 1] != ':')
    {
        strcat(temp_path, "/");
    }

    lv_fs_dir_t dir;
    if (lv_fs_dir_open(&dir, temp_path) == LV_FS_RES_OK)
    {
        // 打开成功，说明是有效目录
        lv_fs_dir_close(&dir);
        strcpy(current_path, next_path);
        return true;
    }

    return false;
}

bool FileLogic_GoUp(void)
{
    // 简单判断是否根目录
    if (strlen(current_path) <= 3)
        return false; // "S:/" or "S:"
    path_up(current_path);
    return true;
}

FileInfo_t *FileLogic_GetFileList(uint16_t *count)
{
    *count = 0;
    lv_fs_dir_t dir;
    lv_fs_res_t res;

    // 确保路径以 / 结尾
    char temp_path[MAX_PATH_LEN];
    strcpy(temp_path, current_path);
    if (temp_path[strlen(temp_path) - 1] != '/' && temp_path[strlen(temp_path) - 1] != ':')
    {
        strcat(temp_path, "/");
    }

    res = lv_fs_dir_open(&dir, temp_path);
    if (res != LV_FS_RES_OK)
        return NULL;

    char fn[64];
    while (1)
    {
        res = lv_fs_dir_read(&dir, fn);
        if (res != LV_FS_RES_OK || fn[0] == '\0')
            break;

        if (fn[0] == '.')
            continue; // 跳过 . 和 ..

        if (*count < MAX_FILES_PER_DIR)
        {
            strncpy(file_list[*count].name, fn, 63);

            char sub_path[MAX_PATH_LEN];
            strcpy(sub_path, temp_path);
            strcat(sub_path, fn);

            lv_fs_dir_t test_dir;

            // 1. 尝试直接打开
            if (lv_fs_dir_open(&test_dir, sub_path) == LV_FS_RES_OK)
            {
                file_list[*count].type = FILE_TYPE_DIR;
                lv_fs_dir_close(&test_dir);
            }
            else
            {
                // 2. 如果失败，尝试追加 '/' 后再打开 (某些 FatFS 驱动需要)
                char sub_path_slash[MAX_PATH_LEN];
                strcpy(sub_path_slash, sub_path);
                strcat(sub_path_slash, "/");

                if (lv_fs_dir_open(&test_dir, sub_path_slash) == LV_FS_RES_OK)
                {
                    file_list[*count].type = FILE_TYPE_DIR;
                    lv_fs_dir_close(&test_dir);
                }
                else
                {
                    file_list[*count].type = FILE_TYPE_FILE;
                }
            }

            (*count)++;
        }
    }

    lv_fs_dir_close(&dir);
    return file_list;
}

bool FileLogic_Delete(const char *name)
{
    char full_path[MAX_PATH_LEN];
    char fat_path[MAX_PATH_LEN];

    // 1. 生成 LVGL 风格的完整路径 (例如 "S:/music/test.mp3")
    strncpy(full_path, current_path, MAX_PATH_LEN);
    path_append(full_path, name);

    // 2. 路径映射转换：S:/xxx -> 0:/xxx
    const char *p_actual = full_path;

    // 跳过 LVGL 的盘符 "S:"
    if (strncmp(p_actual, "S:", 2) == 0)
    {
        p_actual += 2;
    }

    // 格式化为 FatFS 识别的 0:/ 路径
    // 如果 p_actual 是 "/folder/file.txt"，拼接后变成 "0:/folder/file.txt"
    if (*p_actual == '/')
    {
        snprintf(fat_path, sizeof(fat_path), "0:%s", p_actual);
    }
    else
    {
        snprintf(fat_path, sizeof(fat_path), "0:/%s", p_actual);
    }

    // 3. 调用 FatFS 原生函数
    // FRESULT 为 FR_OK (0) 时表示成功
    FRESULT res = f_unlink(fat_path);

    if (res == FR_OK)
    {
        return true;
    }
    else
    {
        DEBUG_INFO("File delete error: %d", res);
        return false;
    }
}

void FileLogic_SetClipboard(const char *name)
{
    strcpy(clipboard_path, current_path);
    path_append(clipboard_path, name);
    clipboard_valid = true;
}

const char *FileLogic_GetClipboard(void)
{
    return clipboard_valid ? clipboard_path : NULL;
}

bool FileLogic_Paste(void)
{
    if (!clipboard_valid)
        return false;

    // 文件复制实现：读源 -> 写目标
    lv_fs_file_t f_src, f_dst;
    if (lv_fs_open(&f_src, clipboard_path, LV_FS_MODE_RD) != LV_FS_RES_OK)
        return false;

    // 构造目标路径 (当前路径 + 源文件名)
    char *fname = strrchr(clipboard_path, '/');
    if (!fname)
        fname = clipboard_path;
    else
        fname++; // 跳过 '/'

    char dst_path[MAX_PATH_LEN];
    strcpy(dst_path, current_path);
    path_append(dst_path, fname);

    if (lv_fs_open(&f_dst, dst_path, LV_FS_MODE_WR) != LV_FS_RES_OK)
    {
        lv_fs_close(&f_src);
        return false;
    }

    // 缓冲区复制
    uint8_t buf[512];
    uint32_t br, bw;
    bool success = true;
    while (1)
    {
        if (lv_fs_read(&f_src, buf, sizeof(buf), &br) != LV_FS_RES_OK)
        {
            success = false;
            break;
        }
        if (br == 0)
            break; // EOF

        if (lv_fs_write(&f_dst, buf, br, &bw) != LV_FS_RES_OK || bw != br)
        {
            success = false;
            break;
        }
    }

    lv_fs_close(&f_src);
    lv_fs_close(&f_dst);
    return success;
}

void FileLogic_ClearClipboard(void)
{
    clipboard_valid = false;
}
