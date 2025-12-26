#ifndef GAMECONFIG_H
#define GAMECONFIG_H

#include <string>

/**
 * @brief 游戏配置常量
 *
 * 集中管理游戏相关的配置常量
 */
namespace GameConfig
{
    // 棋盘配置
    constexpr int DEFAULT_BOARD_SIZE = 15;
    constexpr int MIN_BOARD_SIZE = 9;
    constexpr int MAX_BOARD_SIZE = 19;

    // 时间配置
    constexpr int DEFAULT_GAME_TIME_MINUTES = 20;
    constexpr int DEFAULT_INCREMENT_SECONDS = 5;
    const char *DEFAULT_TIME_FORMAT = "mm:ss";

    // 游戏规则
    constexpr int WIN_COUNT = 5; // 五子连珠获胜

    // 网络配置
    const char *DEFAULT_SERVER_IP = "169.254.56.77";
    constexpr int DEFAULT_SERVER_PORT = 8080;

    // UI配置
    constexpr int CHESSBOARD_GRID_SIZE = 40;
    constexpr int CHESSBOARD_PIECE_RADIUS = 18;
    constexpr int CHESSBOARD_MARGIN = 20;
    constexpr int CHESSBOARD_STAR_POINT_RADIUS = 4;
    constexpr int CHESSBOARD_INDICATOR_RADIUS = 12;

    // 玩家配置
    constexpr int DEFAULT_PLAYER_RATING = 1500;
    const char *DEFAULT_PLAYER_NAME = "玩家";
    const char *AI_PLAYER_NAME_PREFIX = "AI玩家";

    // 游戏状态
    enum class GameMode
    {
        LOCAL,      // 本地游戏
        ONLINE,     // 在线游戏
        AI_VS_AI,   // AI对战
        HUMAN_VS_AI // 人机对战
    };

    enum class GameDifficulty
    {
        EASY,
        MEDIUM,
        HARD
    };
}

#endif // GAMECONFIG_H