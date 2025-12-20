#include "game_2048_core.h"
#include <stdlib.h>
#include <string.h>
#include "FreeRTOS.h" // 引入 FreeRTOS 以使用 xTaskGetTickCount
#include "task.h"

#define BOARD_SIZE 4

static uint16_t board[BOARD_SIZE][BOARD_SIZE];
static uint32_t score = 0;

// 在空位生成一个随机块 (2 或 4)
static void spawn_random_tile(void)
{
    int empty_spots[BOARD_SIZE * BOARD_SIZE][2];
    int count = 0;

    for (int y = 0; y < BOARD_SIZE; y++)
    {
        for (int x = 0; x < BOARD_SIZE; x++)
        {
            if (board[y][x] == 0)
            {
                empty_spots[count][0] = x;
                empty_spots[count][1] = y;
                count++;
            }
        }
    }

    if (count > 0)
    {
        int r = rand() % count;
        int val = (rand() % 10 == 0) ? 4 : 2; // 10% 概率生成 4
        board[empty_spots[r][1]][empty_spots[r][0]] = val;
    }
}

void Game2048_Core_Init(void)
{
    // 使用系统 Tick 作为随机种子
    srand(xTaskGetTickCount());
    Game2048_Core_Reset();
}

void Game2048_Core_Reset(void)
{
    memset(board, 0, sizeof(board));
    score = 0;
    spawn_random_tile();
    spawn_random_tile();
}

// 旋转矩阵（用于简化移动逻辑，只实现向左移动，其他方向通过旋转实现）
static void rotate_board(void)
{
    uint16_t temp[BOARD_SIZE][BOARD_SIZE];
    for (int y = 0; y < BOARD_SIZE; y++)
    {
        for (int x = 0; x < BOARD_SIZE; x++)
        {
            temp[x][BOARD_SIZE - 1 - y] = board[y][x];
        }
    }
    memcpy(board, temp, sizeof(board));
}

// 向左移动并合并的一行逻辑
static bool move_left_line(uint16_t *line)
{
    bool moved = false;
    int insert_pos = 0;

    // 1. 移动非零数到左侧
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        if (line[i] != 0)
        {
            if (i != insert_pos)
            {
                line[insert_pos] = line[i];
                line[i] = 0;
                moved = true;
            }
            insert_pos++;
        }
    }

    // 2. 合并相同的数
    for (int i = 0; i < BOARD_SIZE - 1; i++)
    {
        if (line[i] != 0 && line[i] == line[i + 1])
        {
            line[i] *= 2;
            score += line[i];
            line[i + 1] = 0;
            moved = true;
            // 合并后后面的数需要再次紧凑
            for (int j = i + 1; j < BOARD_SIZE - 1; j++)
            {
                line[j] = line[j + 1];
            }
            line[BOARD_SIZE - 1] = 0;
        }
    }
    return moved;
}

bool Game2048_Core_Move(Game2048_Dir_t dir)
{
    bool moved = false;
    int rotations = 0;

    // 将任意方向旋转为向左
    if (dir == DIR_UP)
        rotations = 1; // 逆时针90度 -> 左
    else if (dir == DIR_RIGHT)
        rotations = 2; // 180度 -> 左
    else if (dir == DIR_DOWN)
        rotations = 3; // 顺时针90度 -> 左

    // 旋转矩阵以便统一处理
    for (int i = 0; i < rotations; i++)
        rotate_board();

    // 执行向左移动
    for (int y = 0; y < BOARD_SIZE; y++)
    {
        if (move_left_line(board[y]))
        {
            moved = true;
        }
    }

    // 还原旋转
    for (int i = 0; i < (4 - rotations) % 4; i++)
        rotate_board();

    if (moved)
    {
        spawn_random_tile();
    }

    return moved;
}

uint32_t Game2048_Core_GetScore(void)
{
    return score;
}

uint16_t Game2048_Core_GetTile(int x, int y)
{
    if (x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE)
        return board[y][x];
    return 0;
}

bool Game2048_Core_IsGameOver(void)
{
    // 检查是否有空位
    for (int y = 0; y < BOARD_SIZE; y++)
        for (int x = 0; x < BOARD_SIZE; x++)
            if (board[y][x] == 0)
                return false;

    // 检查是否有相邻相同
    for (int y = 0; y < BOARD_SIZE; y++)
    {
        for (int x = 0; x < BOARD_SIZE; x++)
        {
            if (x < BOARD_SIZE - 1 && board[y][x] == board[y][x + 1])
                return false;
            if (y < BOARD_SIZE - 1 && board[y][x] == board[y + 1][x])
                return false;
        }
    }
    return true;
}
