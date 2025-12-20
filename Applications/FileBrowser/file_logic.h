#ifndef FILE_LOGIC_H
#define FILE_LOGIC_H

#include <stdint.h>
#include <stdbool.h>
#include "lvgl.h"

#define MAX_PATH_LEN 256
#define MAX_FILES_PER_DIR 128

typedef enum
{
    FILE_TYPE_DIR,
    FILE_TYPE_FILE
} FileType_t;

typedef struct
{
    char name[64];
    FileType_t type;
    uint32_t size;
} FileInfo_t;

// 初始化/重置
void FileLogic_Init(void);

// 获取当前路径
const char *FileLogic_GetPath(void);

// 进入目录
bool FileLogic_EnterDir(const char *dir_name);

// 返回上一级
bool FileLogic_GoUp(void);

// 刷新当前目录下的文件列表
// count: 输出参数，返回文件数量
FileInfo_t *FileLogic_GetFileList(uint16_t *count);

// 删除文件/文件夹
bool FileLogic_Delete(const char *name);

// 标记复制源文件
void FileLogic_SetClipboard(const char *name);

// 获取剪贴板内容
const char *FileLogic_GetClipboard(void);

// 执行粘贴操作（将剪贴板文件复制到当前目录）
bool FileLogic_Paste(void);

// 清空剪贴板
void FileLogic_ClearClipboard(void);

#endif
