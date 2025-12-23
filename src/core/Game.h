#ifndef GAME_H
#define GAME_H

#include <vector>
#include <stack>

enum class Piece {
    EMPTY,
    BLACK,
    WHITE
};

class Game {
public:
    explicit Game(int size = 15);  // 默认15x15棋盘

    // 基本操作
    void reset();
    bool makeMove(int x, int y, Piece player);
    bool undoMove(int x, int y);

    // 获取状态
    const std::vector<std::vector<Piece>>& getBoard() const;
    bool isValidMove(int x, int y) const;

    // 五子棋特有方法
    bool checkWin(int x, int y, Piece player) const;
    bool isBoardFull() const;
    int getBoardSize() const;

private:
    int boardSize;
    std::vector<std::vector<Piece>> board;
    std::stack<std::pair<int, int>> moveHistory;

    // 检查连珠的辅助函数
    int countDirection(int x, int y, int dx, int dy, Piece player) const;
};

#endif // GAME_H
