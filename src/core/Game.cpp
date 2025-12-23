#include "Game.h"
#include <algorithm>

Game::Game(int size) : boardSize(size) {
    reset();
}

void Game::reset() {
    board.assign(boardSize, std::vector<Piece>(boardSize, Piece::EMPTY));
    while (!moveHistory.empty()) {
        moveHistory.pop();
    }
}

bool Game::makeMove(int x, int y, Piece player) {
    if (x < 0 || x >= boardSize || y < 0 || y >= boardSize) {
        return false;
    }

    if (board[x][y] != Piece::EMPTY) {
        return false;
    }

    board[x][y] = player;
    moveHistory.push({x, y});
    return true;
}

bool Game::undoMove(int x, int y) {
    if (moveHistory.empty()) {
        return false;
    }

    auto lastMove = moveHistory.top();
    if (lastMove.first != x || lastMove.second != y) {
        return false;
    }

    board[x][y] = Piece::EMPTY;
    moveHistory.pop();
    return true;
}

const std::vector<std::vector<Piece>>& Game::getBoard() const {
    return board;
}

bool Game::isValidMove(int x, int y) const {
    if (x < 0 || x >= boardSize || y < 0 || y >= boardSize) {
        return false;
    }
    return board[x][y] == Piece::EMPTY;
}

bool Game::checkWin(int x, int y, Piece player) const {
    // 检查8个方向
    const int directions[4][2] = {
        {1, 0},  // 水平
        {0, 1},  // 垂直
        {1, 1},  // 正斜线
        {1, -1}  // 反斜线
    };

    for (int i = 0; i < 4; i++) {
        int count = 1;  // 当前位置

        // 正向计数
        for (int step = 1; step < 5; step++) {
            int nx = x + directions[i][0] * step;
            int ny = y + directions[i][1] * step;

            if (nx < 0 || nx >= boardSize || ny < 0 || ny >= boardSize) {
                break;
            }

            if (board[nx][ny] == player) {
                count++;
            } else {
                break;
            }
        }

        // 反向计数
        for (int step = 1; step < 5; step++) {
            int nx = x - directions[i][0] * step;
            int ny = y - directions[i][1] * step;

            if (nx < 0 || nx >= boardSize || ny < 0 || ny >= boardSize) {
                break;
            }

            if (board[nx][ny] == player) {
                count++;
            } else {
                break;
            }
        }

        if (count >= 5) {
            return true;
        }
    }

    return false;
}

bool Game::isBoardFull() const {
    for (const auto& row : board) {
        for (const auto& cell : row) {
            if (cell == Piece::EMPTY) {
                return false;
            }
        }
    }
    return true;
}

int Game::getBoardSize() const {
    return boardSize;
}
