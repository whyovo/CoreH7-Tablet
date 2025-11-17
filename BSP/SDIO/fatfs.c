/**
 ******************************************************************************
 * @file    fatfs.c
 * @author  菜菜why（B站：菜菜whyy）
 * @brief   FatFs 文件系统源文件，实现 SD 卡文件操作功能
 *          包括挂载、格式化、文件读写、目录管理等操作
 ******************************************************************************
 */
#include "fatfs.h"
#include <string.h>
#include <stdio.h>

/* 全局变量定义 */
FATFS SD_FatFs;     /* 文件系统对象 */
FRESULT MyFile_Res; /* 操作结果 */
char SDPath[4];     /* 逻辑驱动路径，FatFs_LinkDriver 会写入，例如 "0:" */

/**
 * @brief 检查并挂载 FatFs 文件系统
 * @note  若需要，这里可以在挂载失败时加入格式化逻辑（目前仅输出信息）
 */
void FatFs_Check(void)
{
    BYTE work[FF_MAX_SS]; // 用于格式化工作缓冲区

    /* 绑定驱动并挂载（使用 SD_Driver，sd_diskio 提供） */
    FATFS_LinkDriver(&SD_Driver, SDPath); /* SDPath 会被设置为 "0:" */

    MyFile_Res = f_mount(&SD_FatFs, SDPath, 1);
    if (MyFile_Res == FR_OK)
    {
        char buf[128];
        snprintf(buf, sizeof(buf), "FatFs 挂载成功: %s", SDPath);
        DEBUG_INFO(buf);
    }
    else if (MyFile_Res == FR_NO_FILESYSTEM)
    {
        char buf[128];
        DEBUG_INFO("未检测到文件系统，尝试格式化 SD 卡");

        /* 格式化 SD 卡为 FAT32 */
        MyFile_Res = f_mkfs(SDPath, FM_FAT32, 0, work, sizeof(work));

        if (MyFile_Res == FR_OK)
        {
            DEBUG_INFO("SD 卡格式化成功，重新挂载");

            /* 重新挂载 */
            MyFile_Res = f_mount(&SD_FatFs, SDPath, 1);
            if (MyFile_Res == FR_OK)
            {
                snprintf(buf, sizeof(buf), "FatFs 挂载成功: %s", SDPath);
                DEBUG_INFO(buf);
            }
            else
            {
                snprintf(buf, sizeof(buf), "格式化后挂载失败，错误代码: 0x%02X", MyFile_Res);
                DEBUG_ERROR(buf);
            }
        }
        else
        {
            snprintf(buf, sizeof(buf), "SD 卡格式化失败，错误代码: 0x%02X", MyFile_Res);
            DEBUG_ERROR(buf);
        }
    }
    else
    {
        char buf[128];
        snprintf(buf, sizeof(buf), "FatFs 挂载失败，错误代码: 0x%02X", MyFile_Res);
        DEBUG_ERROR(buf);
    }
}

/**
 * @brief 计算并输出 SD 卡总容量与剩余容量（单位：MB）
 */
void FatFs_GetVolume(void)
{
    FATFS *fs;
    DWORD fre_clust, fre_sect, tot_sect;
    uint32_t SD_CardCapacity = 0;
    uint32_t SD_FreeCapacity = 0;
    char buf[128];

    if (f_getfree(SDPath, &fre_clust, &fs) != FR_OK)
    {
        DEBUG_INFO("FatFs 获取空闲簇失败");
        return;
    }

    tot_sect = (fs->n_fatent - 2) * fs->csize; /* 总扇区数 */
    fre_sect = fre_clust * fs->csize;          /* 空闲扇区数 */

    /* 每扇区 512 字节，1 MB = 1024*1024 字节
       total MB = tot_sect * 512 / (1024*1024) = tot_sect / 2048 */
    SD_CardCapacity = tot_sect / 2048;
    SD_FreeCapacity = fre_sect / 2048;

    snprintf(buf, sizeof(buf), " 获取设备容量信息:");
    DEBUG_INFO(buf);
    snprintf(buf, sizeof(buf), "SD 总容量：%u MB", SD_CardCapacity);
    DEBUG_INFO(buf);
    snprintf(buf, sizeof(buf), "SD 剩余：%u MB", SD_FreeCapacity);
    DEBUG_INFO(buf);
}

/**
 * @brief  文件创建、写入与读取测试
 * @return 1=成功, 0=失败
 */
uint8_t FatFs_FileTest(void)
{
    // FIL MyFile;
    // UINT MyFile_Num;
    // BYTE MyFile_WriteBuffer[] = "STM32H750 SD卡 文件系统测试";
    // BYTE MyFile_ReadBuffer[1024];
    // char buf[256];

    // DEBUG_INFO(" FatFs 文件创建/写入测试");

    // /* 创建并写入文件 */
    // MyFile_Res = f_open(&MyFile, "0:FatFs Test.txt", FA_CREATE_ALWAYS | FA_WRITE);
    // if (MyFile_Res == FR_OK)
    // {
    //     MyFile_Res = f_write(&MyFile, MyFile_WriteBuffer, sizeof(MyFile_WriteBuffer), &MyFile_Num);
    //     if (MyFile_Res == FR_OK)
    //     {
    //         snprintf(buf, sizeof(buf), "写入成功，字节数：%u", (unsigned)MyFile_Num);
    //         DEBUG_INFO(buf);
    //     }
    //     else
    //     {
    //         DEBUG_INFO("文件写入失败");
    //         f_close(&MyFile);
    //         return 0;
    //     }
    //     f_close(&MyFile);
    // }
    // else
    // {
    //     DEBUG_INFO("打开/创建文件失败");
    //     return 0;
    // }

    // /* 读取测试 */
    // DEBUG_INFO("FatFs 文件读取测试");

    // size_t BufferSize = sizeof(MyFile_WriteBuffer) / sizeof(BYTE);
    // memset(MyFile_ReadBuffer, 0, sizeof(MyFile_ReadBuffer));

    // MyFile_Res = f_open(&MyFile, "0:FatFs Test.txt", FA_OPEN_EXISTING | FA_READ);
    // if (MyFile_Res != FR_OK)
    // {
    //     DEBUG_INFO("打开要读取的文件失败");
    //     return 0;
    // }

    // MyFile_Res = f_read(&MyFile, MyFile_ReadBuffer, BufferSize, &MyFile_Num);
    // if (MyFile_Res == FR_OK)
    // {
    //     snprintf(buf, sizeof(buf), "读取成功，字节数：%u，内容：%s", (unsigned)MyFile_Num, (char *)MyFile_ReadBuffer);
    //     DEBUG_INFO(buf);
    // }
    // else
    // {
    //     DEBUG_INFO("文件读取失败");
    //     f_close(&MyFile);
    //     return 0;
    // }

    // f_close(&MyFile);
    // return 1;
    // 写入文件
    FatFs_WriteFile("0:data.txt", "Hello World", 11);

    // 读取文件
    char read_buf[256];
    uint32_t bytes = FatFs_ReadFile("0:data.txt", read_buf, sizeof(read_buf));

    // 追加写入
    FatFs_AppendFile("0:data.txt", "\nAppend data", 12);

    // 检查文件是否存在
    if (FatFs_FileExists("0:data.txt"))
        DEBUG_INFO("文件存在");

    // 获取文件大小
    uint32_t size = FatFs_GetFileSize("0:data.txt");

    // 创建目录
    FatFs_CreateDir("0:MyFolder");

    // 列出目录内容
    FatFs_ListDir("0:", 100);

    // 删除文件
    FatFs_DeleteFile("0:data.txt");
		return 1;
}

/*******************************************************************************
 *                          便捷文件操作函数
 ******************************************************************************/

/**
 * @brief 获取错误信息描述
 * @param res FatFs 操作结果
 * @return 错误信息字符串指针
 */
const char *FatFs_GetErrorMsg(FRESULT res)
{
    static const char *error_msg[] = {
        "FR_OK",             /* 0 */
        "FR_DISK_ERR",       /* 1 */
        "FR_INT_ERR",        /* 2 */
        "FR_NOT_READY",      /* 3 */
        "FR_NO_FILE",        /* 4 */
        "FR_NO_PATH",        /* 5 */
        "FR_INVALID_NAME",   /* 6 */
        "FR_DENIED",         /* 7 */
        "FR_EXIST",          /* 8 */
        "FR_INVALID_OBJECT", /* 9 */
        "FR_WRITE_PROTECTED" /* 10 */
    };

    if (res < sizeof(error_msg) / sizeof(error_msg[0]))
    {
        return error_msg[res];
    }
    return "UNKNOWN_ERROR";
}

/**
 * @brief 创建并写入文件
 * @param filename 文件路径（例："0:test.txt"）
 * @param data 写入的数据
 * @param size 写入的数据大小
 * @return FR_OK=成功, 其他=失败
 */
FRESULT FatFs_WriteFile(const char *filename, const void *data, uint32_t size)
{
    FIL file;
    UINT bytes_written;
    char buf[128];

    if (!filename || !data || size == 0)
    {
        DEBUG_ERROR("FatFs_WriteFile: 参数无效");
        return FR_INVALID_NAME;
    }

    MyFile_Res = f_open(&file, filename, FA_CREATE_ALWAYS | FA_WRITE);
    if (MyFile_Res != FR_OK)
    {
        snprintf(buf, sizeof(buf), "打开文件失败: %s, 错误: %s", filename, FatFs_GetErrorMsg(MyFile_Res));
        DEBUG_ERROR(buf);
        return MyFile_Res;
    }

    MyFile_Res = f_write(&file, data, size, &bytes_written);
    if (MyFile_Res != FR_OK)
    {
        snprintf(buf, sizeof(buf), "写入文件失败: %s, 错误: %s", filename, FatFs_GetErrorMsg(MyFile_Res));
        DEBUG_ERROR(buf);
        f_close(&file);
        return MyFile_Res;
    }

    f_close(&file);
    snprintf(buf, sizeof(buf), "文件写入成功: %s (%u 字节)", filename, bytes_written);
    DEBUG_INFO(buf);
    return FR_OK;
}

/**
 * @brief 读取文件
 * @param filename 文件路径
 * @param buffer 读取缓冲区
 * @param size 缓冲区大小
 * @return 实际读取的字节数，失败返回 0
 */
uint32_t FatFs_ReadFile(const char *filename, void *buffer, uint32_t size)
{
    FIL file;
    UINT bytes_read;
    char buf[128];

    if (!filename || !buffer || size == 0)
    {
        DEBUG_ERROR("FatFs_ReadFile: 参数无效");
        return 0;
    }

    MyFile_Res = f_open(&file, filename, FA_OPEN_EXISTING | FA_READ);
    if (MyFile_Res != FR_OK)
    {
        snprintf(buf, sizeof(buf), "打开文件失败: %s, 错误: %s", filename, FatFs_GetErrorMsg(MyFile_Res));
        DEBUG_ERROR(buf);
        return 0;
    }

    MyFile_Res = f_read(&file, buffer, size, &bytes_read);
    if (MyFile_Res != FR_OK)
    {
        snprintf(buf, sizeof(buf), "读取文件失败: %s, 错误: %s", filename, FatFs_GetErrorMsg(MyFile_Res));
        DEBUG_ERROR(buf);
        f_close(&file);
        return 0;
    }

    f_close(&file);
    snprintf(buf, sizeof(buf), "文件读取成功: %s (%u 字节)", filename, bytes_read);
    DEBUG_INFO(buf);
    return bytes_read;
}

/**
 * @brief 追加写入文件
 * @param filename 文件路径
 * @param data 写入的数据
 * @param size 写入的数据大小
 * @return FR_OK=成功, 其他=失败
 */
FRESULT FatFs_AppendFile(const char *filename, const void *data, uint32_t size)
{
    FIL file;
    UINT bytes_written;
    char buf[128];

    if (!filename || !data || size == 0)
    {
        DEBUG_ERROR("FatFs_AppendFile: 参数无效");
        return FR_INVALID_NAME;
    }

    MyFile_Res = f_open(&file, filename, FA_OPEN_APPEND | FA_WRITE);
    if (MyFile_Res != FR_OK)
    {
        snprintf(buf, sizeof(buf), "打开文件失败: %s, 错误: %s", filename, FatFs_GetErrorMsg(MyFile_Res));
        DEBUG_ERROR(buf);
        return MyFile_Res;
    }

    MyFile_Res = f_write(&file, data, size, &bytes_written);
    if (MyFile_Res != FR_OK)
    {
        snprintf(buf, sizeof(buf), "追加写入失败: %s, 错误: %s", filename, FatFs_GetErrorMsg(MyFile_Res));
        DEBUG_ERROR(buf);
        f_close(&file);
        return MyFile_Res;
    }

    f_close(&file);
    snprintf(buf, sizeof(buf), "文件追加成功: %s (%u 字节)", filename, bytes_written);
    DEBUG_INFO(buf);
    return FR_OK;
}

/**
 * @brief 删除文件
 * @param filename 文件路径
 * @return FR_OK=成功, 其他=失败
 */
FRESULT FatFs_DeleteFile(const char *filename)
{
    char buf[128];

    if (!filename)
    {
        DEBUG_ERROR("FatFs_DeleteFile: 文件名无效");
        return FR_INVALID_NAME;
    }

    MyFile_Res = f_unlink(filename);
    if (MyFile_Res != FR_OK)
    {
        snprintf(buf, sizeof(buf), "删除文件失败: %s, 错误: %s", filename, FatFs_GetErrorMsg(MyFile_Res));
        DEBUG_ERROR(buf);
        return MyFile_Res;
    }

    snprintf(buf, sizeof(buf), "文件删除成功: %s", filename);
    DEBUG_INFO(buf);
    return FR_OK;
}

/**
 * @brief 检查文件是否存在
 * @param filename 文件路径
 * @return 1=存在, 0=不存在
 */
uint8_t FatFs_FileExists(const char *filename)
{
    FIL file;

    if (!filename)
        return 0;

    MyFile_Res = f_open(&file, filename, FA_OPEN_EXISTING | FA_READ);
    if (MyFile_Res == FR_OK)
    {
        f_close(&file);
        return 1;
    }
    return 0;
}

/**
 * @brief 获取文件大小
 * @param filename 文件路径
 * @return 文件大小（字节），失败返回 0
 */
uint32_t FatFs_GetFileSize(const char *filename)
{
    FIL file;
    char buf[128];

    if (!filename)
    {
        DEBUG_ERROR("FatFs_GetFileSize: 文件名无效");
        return 0;
    }

    MyFile_Res = f_open(&file, filename, FA_OPEN_EXISTING | FA_READ);
    if (MyFile_Res != FR_OK)
    {
        snprintf(buf, sizeof(buf), "打开文件失败: %s, 错误: %s", filename, FatFs_GetErrorMsg(MyFile_Res));
        DEBUG_ERROR(buf);
        return 0;
    }

    uint32_t file_size = f_size(&file);
    f_close(&file);

    snprintf(buf, sizeof(buf), "文件大小: %s = %u 字节", filename, file_size);
    DEBUG_INFO(buf);
    return file_size;
}

/**
 * @brief 创建目录
 * @param dirname 目录路径（例："0:MyDir"）
 * @return FR_OK=成功, 其他=失败
 */
FRESULT FatFs_CreateDir(const char *dirname)
{
    char buf[128];

    if (!dirname)
    {
        DEBUG_ERROR("FatFs_CreateDir: 目录名无效");
        return FR_INVALID_NAME;
    }

    MyFile_Res = f_mkdir(dirname);
    if (MyFile_Res != FR_OK)
    {
        snprintf(buf, sizeof(buf), "创建目录失败: %s, 错误: %s", dirname, FatFs_GetErrorMsg(MyFile_Res));
        DEBUG_ERROR(buf);
        return MyFile_Res;
    }

    snprintf(buf, sizeof(buf), "目录创建成功: %s", dirname);
    DEBUG_INFO(buf);
    return FR_OK;
}

/**
 * @brief 删除目录
 * @param dirname 目录路径
 * @return FR_OK=成功, 其他=失败
 */
FRESULT FatFs_DeleteDir(const char *dirname)
{
    char buf[128];

    if (!dirname)
    {
        DEBUG_ERROR("FatFs_DeleteDir: 目录名无效");
        return FR_INVALID_NAME;
    }

    MyFile_Res = f_unlink(dirname);
    if (MyFile_Res != FR_OK)
    {
        snprintf(buf, sizeof(buf), "删除目录失败: %s, 错误: %s", dirname, FatFs_GetErrorMsg(MyFile_Res));
        DEBUG_ERROR(buf);
        return MyFile_Res;
    }

    snprintf(buf, sizeof(buf), "目录删除成功: %s", dirname);
    DEBUG_INFO(buf);
    return FR_OK;
}

/**
 * @brief 列出目录中的文件和子目录
 * @param dirname 目录路径
 * @param max_items 最多显示的项目数
 * @return FR_OK=成功, 其他=失败
 */
FRESULT FatFs_ListDir(const char *dirname, uint32_t max_items)
{
    DIR dir;
    FILINFO fno;
    uint32_t count = 0;
    char buf[256];

    if (!dirname)
    {
        DEBUG_ERROR("FatFs_ListDir: 目录名无效");
        return FR_INVALID_NAME;
    }

    MyFile_Res = f_opendir(&dir, dirname);
    if (MyFile_Res != FR_OK)
    {
        snprintf(buf, sizeof(buf), "打开目录失败: %s, 错误: %s", dirname, FatFs_GetErrorMsg(MyFile_Res));
        DEBUG_ERROR(buf);
        return MyFile_Res;
    }

    snprintf(buf, sizeof(buf), "====== 目录内容: %s ======", dirname);
    DEBUG_INFO(buf);

    while (1)
    {
        MyFile_Res = f_readdir(&dir, &fno);
        if (MyFile_Res != FR_OK || fno.fname[0] == 0)
            break;

        if (count >= max_items)
        {
            DEBUG_INFO("(列表已截断)");
            break;
        }

        if (fno.fattrib & AM_DIR)
        {
            snprintf(buf, sizeof(buf), "[DIR]  %s", fno.fname);
        }
        else
        {
            snprintf(buf, sizeof(buf), "[FILE] %s (%u 字节)", fno.fname, (unsigned)fno.fsize);
        }
        DEBUG_INFO(buf);
        count++;
    }

    f_closedir(&dir);
    snprintf(buf, sizeof(buf), "====== 总计: %u 项 ======", count);
    DEBUG_INFO(buf);
    return FR_OK;
}
