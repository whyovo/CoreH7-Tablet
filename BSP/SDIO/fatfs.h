/**
 ******************************************************************************
 * @file    fatfs.h
 * @author  菜菜why（B站：菜菜whyy）
 * @brief   FatFs 文件系统头文件，提供 SD 卡文件操作接口
 *          集成自动挂载、格式化、文件读写、目录管理等功能
 ******************************************************************************
 * @attention
 *
 * 使用说明：
 * 1. 需要先启用 SDMMC_ENABLE 和 FATFS_ENABLE 宏定义（在 init.h 中配置）
 * 2. init_all() 会自动初始化 SD 卡并挂载 FatFs 文件系统
 * 3. 若 SD 卡未格式化，会自动进行 FAT32 格式化
 * 4. 所有操作结果通过 DEBUG_INFO/DEBUG_ERROR 输出
 * 5. 文件路径格式：带前缀 "0:" 或 "0:/" 开头（例："0:test.txt"、"0:MyDir/data.bin"）
 *
 ******************************************************************************
 */
#ifndef FATFS_H
#define FATFS_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "init.h"
#include "FatFs/ff.h"
#include "FatFs/ff_gen_drv.h"
#include "FatFs/sd_diskio.h"
#include <stdint.h>

    /* 全局变量（在 fatfs.c 中定义） */
    extern FATFS SD_FatFs;     /* 文件系统对象 */
    extern FRESULT MyFile_Res; /* 最近一次文件操作结果 */
    extern char SDPath[4];     /* SD 卡逻辑驱动路径，例如 "0:" */

    /* 导出函数（供 main.c 调用） */
    void FatFs_Check(void);       /* 挂载 / 初始化 FatFs（若需要可格式化） */
    void FatFs_GetVolume(void);   /* 计算 SD 卡容量并通过 DEBUG 输出 */
    uint8_t FatFs_FileTest(void); /* 文件创建/写入/读取测试，返回 1=成功，0=失败 */

    /* ==================== 便捷文件操作函数 ==================== */

    /**
     * @brief 获取错误信息描述
     * @param res FatFs 操作结果
     * @return 错误信息字符串
     */
    const char *FatFs_GetErrorMsg(FRESULT res);

    /**
     * @brief 创建并写入文件
     * @param filename 文件路径（例："0:test.txt"）
     * @param data 写入的数据指针
     * @param size 写入的数据大小（字节）
     * @return FR_OK=成功, 其他=失败
     * @note 如果文件已存在，则覆盖
     */
    FRESULT FatFs_WriteFile(const char *filename, const void *data, uint32_t size);

    /**
     * @brief 读取文件
     * @param filename 文件路径
     * @param buffer 读取缓冲区
     * @param size 缓冲区大小（字节）
     * @return 实际读取的字节数，失败返回 0
     */
    uint32_t FatFs_ReadFile(const char *filename, void *buffer, uint32_t size);

    /**
     * @brief 追加写入文件
     * @param filename 文件路径
     * @param data 写入的数据指针
     * @param size 写入的数据大小（字节）
     * @return FR_OK=成功, 其他=失败
     * @note 如果文件不存在，则创建文件
     */
    FRESULT FatFs_AppendFile(const char *filename, const void *data, uint32_t size);

    /**
     * @brief 删除文件
     * @param filename 文件路径
     * @return FR_OK=成功, 其他=失败
     */
    FRESULT FatFs_DeleteFile(const char *filename);

    /**
     * @brief 检查文件是否存在
     * @param filename 文件路径
     * @return 1=存在, 0=不存在
     */
    uint8_t FatFs_FileExists(const char *filename);

    /**
     * @brief 获取文件大小
     * @param filename 文件路径
     * @return 文件大小（字节），失败返回 0
     */
    uint32_t FatFs_GetFileSize(const char *filename);

    /**
     * @brief 创建目录
     * @param dirname 目录路径（例："0:MyDir"）
     * @return FR_OK=成功, 其他=失败
     */
    FRESULT FatFs_CreateDir(const char *dirname);

    /**
     * @brief 删除目录（目录必须为空）
     * @param dirname 目录路径
     * @return FR_OK=成功, 其他=失败
     */
    FRESULT FatFs_DeleteDir(const char *dirname);

    /**
     * @brief 列出目录中的文件和子目录
     * @param dirname 目录路径
     * @param max_items 最多显示的项目数
     * @return FR_OK=成功, 其他=失败
     */
    FRESULT FatFs_ListDir(const char *dirname, uint32_t max_items);

#ifdef __cplusplus
}
#endif

#endif /* TEMPLATE_FATFS_H */
