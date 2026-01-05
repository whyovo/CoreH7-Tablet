// #include "music_task.h"
// #include "music_ui.h"
// #include "cmsis_os.h"
// #include "lvgl.h"
// #include <stdio.h>
// #include <string.h>
// #include "config.h"
// #include "audio_player.h"
// #include "mp3_player.h"
// #include "ff.h"

// #define MAX_MUSIC_FILES 64

// /* 全局变量供 lv_demo_music.c 使用 */
// char *g_music_titles[MAX_MUSIC_FILES];
// uint32_t g_music_count = 0;

// /* 播放器实例 */
// static AudioPlayer_t g_wav_player = {0};
// static MP3Player_t g_mp3_player = {0};
// static uint8_t g_player_mode = 0; // 0:None, 1:WAV, 2:MP3

// /* --- AV 解析 --- */
// static uint32_t Parse_WAV_Duration(FIL *fp)
// {
//     uint32_t byte_rate = 0;
//     uint32_t data_size = 0;
//     UINT br;
//     uint8_t buf[16]; // 临时缓冲

//     // 1. 读取 RIFF 头 (12字节)
//     f_lseek(fp, 0);
//     f_read(fp, buf, 12, &br);
//     if (br != 12 || *(uint32_t *)&buf[0] != 0x46464952) // "RIFF"
//         return 0;

//     // 2. 遍历 Chunk 查找 fmt 和 data
//     while (1)
//     {
//         uint32_t chunk_id, chunk_size;
//         // 读取 Chunk Header (8字节: ID + Size)
//         if (f_read(fp, buf, 8, &br) != FR_OK || br != 8)
//             break;

//         chunk_id = *(uint32_t *)&buf[0];
//         chunk_size = *(uint32_t *)&buf[4];

//         if (chunk_id == 0x20746d66) // "fmt "
//         {
//             // 读取 fmt 块内容 (至少16字节)
//             if (chunk_size >= 16)
//             {
//                 f_read(fp, buf, 16, &br);
//                 // ByteRate 在 fmt 块的偏移 8 处
//                 byte_rate = *(uint32_t *)&buf[8];

//                 // 跳过 fmt 块剩余部分
//                 if (chunk_size > 16)
//                     f_lseek(fp, f_tell(fp) + (chunk_size - 16));
//             }
//             else
//             {
//                 f_lseek(fp, f_tell(fp) + chunk_size);
//             }
//         }
//         else if (chunk_id == 0x61746164) // "data"
//         {
//             data_size = chunk_size;
//             break; // 找到数据块，停止搜索
//         }
//         else
//         {
//             // 跳过未知块 (如 LIST, JUNK, ID3 等)
//             f_lseek(fp, f_tell(fp) + chunk_size);
//         }
//     }

//     if (byte_rate > 0 && data_size > 0)
//     {
//         return data_size / byte_rate;
//     }
//     return 0;
// }

// /* --- 获取歌曲时长(秒) --- */
// uint32_t Music_Get_Duration(uint32_t index)
// {
//     if (index >= g_music_count)
//         return 0;

//     char path[256];
//     snprintf(path, sizeof(path), "0:/mymusic/%s", g_music_titles[index]);

//     FIL f;
//     if (f_open(&f, path, FA_READ) != FR_OK)
//         return 0;

//     uint32_t duration = 0;
//     char *ext = strrchr(path, '.');

//     if (ext && (strcasecmp(ext, ".wav") == 0))
//     {
//         duration = Parse_WAV_Duration(&f);
//     }
//     else if (ext && (strcasecmp(ext, ".mp3") == 0))
//     {
//         // MP3 估算：假设 128kbps
//         FSIZE_t size = f_size(&f);
//         duration = size / 16384;
//     }

//     f_close(&f);
//     return duration;
// }

// /* --- 导出给 UI 调用的控制函数 --- */

// void Music_Play_Index(uint32_t index)
// {
//     if (index >= g_music_count)
//         return;

//     // 1. 停止当前播放
//     if (g_player_mode == 1)
//         AudioPlayer_Stop(&g_wav_player);
//     else if (g_player_mode == 2)
//         MP3Player_Stop(&g_mp3_player);
//     g_player_mode = 0;

//     // 2. 构建文件路径
//     char path[256];
//     snprintf(path, sizeof(path), "0:/mymusic/%s", g_music_titles[index]);
//     DEBUG_INFO("Playing: %s", path);

//     // 3. 判断格式并播放
//     char *ext = strrchr(path, '.');
//     if (ext && (strcasecmp(ext, ".wav") == 0))
//     {
//         if (AudioPlayer_OpenFile(&g_wav_player, path) == HAL_OK)
//         {
//             AudioPlayer_Play(&g_wav_player);
//             g_player_mode = 1;
//         }
//     }
//     else if (ext && (strcasecmp(ext, ".mp3") == 0))
//     {
//         if (MP3Player_OpenFile(&g_mp3_player, path) == HAL_OK)
//         {
//             MP3Player_Play(&g_mp3_player);
//             g_player_mode = 2;
//         }
//     }
// }

// void Music_Pause_Playback(void)
// {
//     if (g_player_mode == 1)
//         DSPEAKER_Stop(); // WAV 暂停直接停 I2S
//     else if (g_player_mode == 2)
//         MP3Player_Pause(&g_mp3_player);
// }

// void Music_Resume_Playback(void)
// {
//     if (g_player_mode == 1)
//         DSPEAKER_Start();
//     else if (g_player_mode == 2)
//         MP3Player_Resume(&g_mp3_player);
// }

// void Music_Set_Volume(uint8_t vol)
// {
//     DSPEAKER_SetVolume(vol);
// }

// /**
//  * @brief 扫描 S:/mymusic 下的 mp3 和 wav 文件
//  */
// static void Scan_Music_Files(void)
// {
//     lv_fs_dir_t dir;
//     lv_fs_res_t res;

//     // 清理旧数据
//     for (uint32_t i = 0; i < g_music_count; i++)
//     {
//         if (g_music_titles[i])
//             lv_mem_free(g_music_titles[i]);
//         g_music_titles[i] = NULL;
//     }
//     g_music_count = 0;

//     res = lv_fs_dir_open(&dir, "S:/mymusic");
//     if (res != LV_FS_RES_OK)
//     {
//         DEBUG_INFO("Failed to open music dir\n");
//         return;
//     }

//     char fn[256];
//     while (1)
//     {
//         res = lv_fs_dir_read(&dir, fn);
//         if (res != LV_FS_RES_OK || fn[0] == '\0')
//             break;

//         // 检查扩展名
//         char *ext = strrchr(fn, '.');
//         if (ext && (strcmp(ext, ".mp3") == 0 || strcmp(ext, ".wav") == 0))
//         {
//             if (g_music_count < MAX_MUSIC_FILES)
//             {
//                 // 申请内存保存文件名作为标题
//                 g_music_titles[g_music_count] = lv_mem_alloc(strlen(fn) + 1);
//                 strcpy(g_music_titles[g_music_count], fn);
//                 g_music_count++;
//             }
//         }
//     }
//     lv_fs_dir_close(&dir);
// }

// /**
//  * @brief 音乐播放器初始化
//  */
// static void Music_App_Init(void)
// {
//     // 先扫描文件
//     Scan_Music_Files();
//     // 再创建 UI
//     Music_UI_Create();

//     // 设置默认音量
//     Music_Set_Volume(10);
// }

// /**
//  * @brief 音乐播放器退出
//  */
// static void Music_App_Exit(void)
// {
//     // 停止播放
//     if (g_player_mode == 1)
//         AudioPlayer_Stop(&g_wav_player);
//     else if (g_player_mode == 2)
//         MP3Player_Stop(&g_mp3_player);
//     g_player_mode = 0;

//     Music_UI_Delete();

//     // 释放文件名内存
//     for (uint32_t i = 0; i < g_music_count; i++)
//     {
//         if (g_music_titles[i])
//         {
//             lv_mem_free(g_music_titles[i]);
//             g_music_titles[i] = NULL;
//         }
//     }
//     g_music_count = 0;
// }

// /**
//  * @brief 音乐播放器任务入口
//  */
// static void Music_Task_Entry(void *params)
// {
//     (void)params; // Unused parameter
//     while (1)
//     {
//         // 如果是 MP3 模式，需要不断调用解码任务
//         if (g_player_mode == 2)
//         {
//             MP3Player_DecodeTask(&g_mp3_player);
//             vTaskDelay(2); // 给解码留足时间，但也让出一点 CPU
//         }
//         else
//         {
//             vTaskDelay(pdMS_TO_TICKS(100));
//         }
//     }
// }

// // 定义应用描述符
// App_Descriptor_t MusicPlayerApp = {
//     .name = "Music",
//     .app_init = Music_App_Init,
//     .app_exit = Music_App_Exit,
//     .task_entry = Music_Task_Entry,
//     .stack_size = 2048 // 音乐播放可能需要较大的栈
// };
