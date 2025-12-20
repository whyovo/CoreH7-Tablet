#ifndef GAME_2048_CORE_H
#define GAME_2048_CORE_H

#include <stdint.h>
#include <stdbool.h>

// 游戏方向定义
typedef enum
{
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} Game2048_Dir_t;

// 初始化/重置游戏
void Game2048_Core_Init(void);
void Game2048_Core_Reset(void);

// 移动逻辑，返回 true 表示有移动发生
bool Game2048_Core_Move(Game2048_Dir_t dir);

// 获取当前分数
uint32_t Game2048_Core_GetScore(void);

// 获取指定位置的数值 (0-3, 0-3)
uint16_t Game2048_Core_GetTile(int x, int y);

// 检查游戏是否结束
bool Game2048_Core_IsGameOver(void);

#endif
