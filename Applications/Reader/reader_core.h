#ifndef READER_CORE_H
#define READER_CORE_H

#include <stdint.h>
#include <stdbool.h>

// 最大读取缓冲大小 (32KB)，根据内存情况调整
#define READER_BUFFER_SIZE (32 * 1024)

// 获取文件列表的回调函数类型
// filename: 文件名
// user_data: 用户传入的参数
typedef void (*file_found_cb_t)(const char *filename, void *user_data);

/**
 * @brief 扫描 S:/mytxt 目录下的 .txt 文件
 * @param cb 找到文件时的回调函数
 * @param user_data 用户数据
 */
void Reader_Core_ScanDir(file_found_cb_t cb, void *user_data);

/**
 * @brief 获取文件大小
 */
uint32_t Reader_Core_GetFileSize(const char *filename);

/**
 * @brief 读取文件内容到缓冲区
 * @param filename 文件名 (不含路径，默认在 S:/mytxt/)
 * @param buffer 输出缓冲区
 * @param max_len 缓冲区最大长度
 * @param offset 文件偏移量
 * @return 实际读取字节数
 */
int Reader_Core_ReadFile(const char *filename, uint32_t offset, char *buffer, uint32_t max_len);

#endif // READER_CORE_H
