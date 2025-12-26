#include "Game.h"
#include <sstream>
#include <vector>

void Game::reset()
{
    board.assign(size, std::vector<Piece>(size, Piece::EMPTY));
    history.clear();
    currPlayer = Piece::BLACK;
    status = Status::Idle;
    emitUpdate();
}

bool Game::move(int x, int y)
{
    if (status != Status::Active || x < 0 || x >= size || y < 0 || y >= size || board[x][y] != Piece::EMPTY)
        return false;

    Piece p = currPlayer;
    board[x][y] = p;
    history.push_back({x, y, p});

    if (checkWin(x, y, p) && isLocal)
    {
        status = Status::Settled;
        if (onBoardChanged)
            onBoardChanged(board);
        if (onGameEnded)
            onGameEnded(p == Piece::BLACK ? "黑方胜" : "白方胜");
    }
    else
    {
        currPlayer = (p == Piece::BLACK) ? Piece::WHITE : Piece::BLACK;
        emitUpdate();
    }
    return true;
}

bool Game::undo()
{
    if (history.empty())
        return false;

    auto &last = history.back();
    board[last.x][last.y] = Piece::EMPTY;
    history.pop_back();
    currPlayer = (currPlayer == Piece::BLACK) ? Piece::WHITE : Piece::BLACK;
    onBoardChanged(board);
    return true;
}

bool Game::sync(const std::string &data)
{
    if (data.empty())
        return false;

    // 尝试解析并恢复状态
    // deserialize 内部已包含 reset() 和 history 重放
    if (deserialize(data))
    {
        // 同步成功后，确保 UI 得到最新的完整状态通知
        emitUpdate();

        // 如果同步后的历史记录显示游戏已结束，需更新状态
        if (!history.empty())
        {
            auto &last = history.back();
            if (checkWin(last.x, last.y, last.p))
            {
                status = Status::Settled;
                if (onGameEnded)
                    onGameEnded(last.p == Piece::BLACK ? "黑方胜" : "白方胜");
            }
        }
        return true;
    }
    return false;
}

bool Game::checkWin(int x, int y, Piece p) const
{
    static const int dirs[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};
    for (auto &d : dirs)
    {
        int count = 1;
        for (int side : {1, -1})
        {
            for (int s = 1; s < 5; ++s)
            {
                int nx = x + d[0] * s * side, ny = y + d[1] * s * side;
                if (nx < 0 || nx >= size || ny < 0 || ny >= size || board[nx][ny] != p)
                    break;
                if (++count >= 5)
                    return true;
            }
        }
    }
    return false;
}

// 序列化格式: version:1;size:15||0,9,8;1,10,10;...
std::string Game::serialize() const
{
    std::ostringstream oss;
    // 1. 配置部分
    oss << "v:1;s:" << size << "||";

    // 2. 历史记录部分 (0,x,y;1,x,y...)
    for (size_t i = 0; i < history.size(); ++i)
    {
        const auto &s = history[i];
        oss << (s.p == Piece::BLACK ? 0 : 1) << "," << s.x << "," << s.y;
        if (i != history.size() - 1)
            oss << ";";
    }
    return oss.str();
}

bool Game::deserialize(const std::string &data)
{
    auto sep = data.find("||");
    if (sep == std::string::npos)
        return false;

    // 解析配置（示例简化处理，可扩展）
    std::string config = data.substr(0, sep);
    if (config.find("s:15") != std::string::npos)
        this->size = 15;

    // 重置状态准备重放
    reset();

    std::string moves = data.substr(sep + 2);
    if (moves.empty())
        return true;

    std::stringstream ss(moves);
    std::string item;
    while (std::getline(ss, item, ';'))
    {
        std::stringstream is(item);
        std::string p_str, x_str, y_str;

        if (std::getline(is, p_str, ',') &&
            std::getline(is, x_str, ',') &&
            std::getline(is, y_str, ','))
        {

            int p_idx = std::stoi(p_str);
            int x = std::stoi(x_str);
            int y = std::stoi(y_str);
            move(x, y);
        }
    }
    return true;
}
