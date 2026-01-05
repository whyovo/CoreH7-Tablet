#ifndef MUSIC_GAME_UI_H
#define MUSIC_GAME_UI_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "lvgl.h"
#include "music_game_core.h" // 引用 Core 定义

    void MusicGame_UI_Create(void);
    void MusicGame_UI_Delete(void);

#ifdef __cplusplus
}
#endif

#endif // MUSIC_GAME_UI_H
