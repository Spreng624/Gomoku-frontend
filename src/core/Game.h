#pragma once
#include <vector>
#include <string>
#include <functional>

enum class Piece
{
    EMPTY,
    BLACK,
    WHITE
};

class Game
{
public:
    enum class Status
    {
        Idle,
        Active,
        Paused,
        Settled
    };

    Game() { reset(); }

    // ==================== 配置与控制 ====================
    void setLocalMode(bool local) { isLocal = local; }
    void reset();
    void start()
    {
        status = Status::Active;
        if (onGameStarted)
            onGameStarted();
    }
    void pause() { status = Status::Paused; }
    void resume() { status = Status::Active; }
    void end(std::string msg)
    {
        status = Status::Settled;
        onGameEnded(msg);
    }
    void end(Piece winner)
    {
        status = Status::Settled;
        std::string msg = (winner == Piece::BLACK ? "黑方获胜" : "白方获胜");
        onGameEnded(msg);
    }

    // ==================== 核心操作 ====================
    bool move(int x, int y); // 玩家尝试落子
    bool undo();
    bool sync(const std::string &data);
    bool applyRemoteMove(int x, int y, Piece p); // 响应服务器确认

    std::vector<std::vector<Piece>> getBoard() const { return board; };

    // ==================== 导出接口 (回调注入) ====================
    void setOnBoardChanged(std::function<void(const std::vector<std::vector<Piece>> &)> cb) { onBoardChanged = cb; }
    void setOnTurnChanged(std::function<void(Piece)> cb) { onTurnChanged = cb; }
    void setOnGameStarted(std::function<void()> cb) { onGameStarted = cb; }
    void setOnGameEnded(std::function<void(const std::string &)> cb) { onGameEnded = cb; }
    void setOnMoveRequest(std::function<void(int, int)> cb) { onMoveRequest = cb; }
    void setOnGameSyncReq(std::function<void(const std::string &)> cb) { onGameSyncReq = cb; }

    // ==================== 状态序列化 ====================
    std::string serialize() const;
    bool deserialize(const std::string &data);

private:
    bool checkWin(int x, int y, Piece p) const;
    void emitUpdate()
    {
        if (onBoardChanged)
            onBoardChanged(board);
        if (onTurnChanged)
            onTurnChanged(currPlayer);
    }

    Status status = Status::Idle;
    bool isLocal = true;
    int size = 15;
    Piece currPlayer = Piece::BLACK;
    std::vector<std::vector<Piece>> board;

    struct Step
    {
        int x, y;
        Piece p;
    };
    std::vector<Step> history; // 替代 stack，更易于遍历序列化

    // 回调句柄
    std::function<void(const std::vector<std::vector<Piece>> &)> onBoardChanged;
    std::function<void(Piece)> onTurnChanged;
    std::function<void()> onGameStarted;
    std::function<void(const std::string &)> onGameEnded;
    std::function<void(int, int)> onMoveRequest;
    std::function<void(const std::string &)> onGameSyncReq;
};