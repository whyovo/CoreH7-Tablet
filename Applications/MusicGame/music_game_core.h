#ifndef MUSIC_GAME_CORE_H
#define MUSIC_GAME_CORE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>

// 最大歌曲数量限制
#define MAX_SONGS 20

    typedef struct
    {
        char folder_name[128]; // 文件夹名缓冲区
        char osu_filename[64]; // osu文件名 
        char title[64];        // 歌曲名
        char artist[64];       // 艺术家
        char creator[64];      // 谱面作者
        char version[64];      // 难度名


        // 统计数据
        float stars;
        float bpm;
        int duration;
        int short_notes;
        int long_notes;
    } SongInfo_t;

    // --- Core API ---
    void MusicGame_Core_Init(void);
    void MusicGame_Core_ScanSongs(void);
    int MusicGame_Core_GetSongCount(void);
    SongInfo_t *MusicGame_Core_GetSongInfo(int index);

    // 全局游戏设置
    extern int music_game_speed; // 游戏流速 (建议范围 1-30)

#ifdef __cplusplus
}
#endif

#endif // MUSIC_GAME_CORE_H
