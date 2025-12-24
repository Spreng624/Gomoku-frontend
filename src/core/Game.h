#ifndef GAME_H
#define GAME_H

#include <vector>
#include <stack>

enum class Piece
{
    EMPTY,
    BLACK,
    WHITE
};

class Game
{
public:
    Game(int size = 15); // 默认15x15棋盘

    void reset();
    bool isValidMove(int x, int y) const;
    bool makeMove(int x, int y);
    bool undoMove();

    const std::vector<std::vector<Piece>> &getBoard() const;

    // 五子棋特有方法
    bool checkWin(int x, int y, Piece player) const;
    bool isBoardFull() const;
    int getBoardSize() const;

    void startGame(); // 准备给计时用，模拟白棋按棋钟

    int boardSize;
    Piece currentPlayer;
    std::vector<std::vector<Piece>> board;
    std::stack<std::pair<int, int>> moveHistory;
    bool strictMode = true; // 非严格模式不校验连五，不计时

private:
    // 检查连珠的辅助函数
    int countDirection(int x, int y, int dx, int dy, Piece player) const;
};

#endif // GAME_H
