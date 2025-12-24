#ifndef GAMEWIDGET_H
#define GAMEWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QList>
#include <QPair>
#include <QString>
#include <memory>
#include "Game.h"
#include "AiPlayer.h"
#include "ChessBoardWidget.h" // 包含新的棋盘部件

class Controller; // 前向声明

namespace Ui
{
    class GameWidget;
}

enum GameStatus
{
    NotStarted,
    Playing,
    End
};

struct PlayerInfo
{
    QString username;
    QString TimeLeft;
};

class GameWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GameWidget(QWidget *parent = nullptr);
    ~GameWidget();

signals:
    // 棋盘信号
    void makeMove(int x, int y); // 点击棋盘

    // Part1: 执黑、执白、取消就坐（乐观响应）
    void takeBlack();
    void takeWhite();
    void cancelTake();

    // Part2: 开始/重新开始、认输、请求和棋、请求悔棋（等待响应）
    void startGame();
    void restartGame();
    void giveup();
    void drawRequest();
    void undoMoveRequest();

    // Part3: 聊天、记录、玩家列表、设置（乐观响应）
    void chatMessageSent(const QString &message);
    void editRoomSetting(const QStringList &settings);

    // Part4: 返回大厅（乐观响应）--> MainWindow
    void backToLobby();

    // 弹窗响应（等待响应）
    void drawResponse(bool accept);
    void undoMoveResponse(bool accepted);

    // --> MainWindow
    void logToUser(const QString &message);

public slots:
    // 游戏初始化和状态变化信号
    void init(bool islocal);
    void onGameStarted();
    void onGameEnded(QString message);

    // 棋盘信息（同时更新历史记录）
    void onMakeMove(int x, int y);
    void onBoardUpdated(const std::vector<std::vector<Piece>> &board);

    // Part1: 就坐、计时信息
    void onBlackTaken(const QString &username);
    void onWhiteTaken(const QString &username);
    void onBlackTimeUpdate(int playerTime);
    void onWhiteTimeUpdate(int playerTime);

    // Part3: 聊天、玩家列表、设置
    void onChatMessageReceived(const QString &username, const QString &message);
    void onUpdateRoomPlayerList(const QStringList &players);
    void onUpdateRoomSetting(const QStringList &settings);

    // 弹窗提示: 和棋、悔棋请求与响应
    void onDrawRequestReceived();
    void onDrawResponseReceived(bool accept);
    void onUndoMoveRequestReceived();
    void onUndoMoveResponseReceived(bool accepted);

private:
    void SetUpSignals();
    bool isPlaying();
    void updatePlayerSeatUI(bool isBlack); // 更新玩家座位UI显示

private:
    Ui::GameWidget *ui;
    std::unique_ptr<Game> game;
    std::unique_ptr<ChessBoardWidget> chessBoard;
    bool isLocal;
    GameStatus gameStatus;
    std::unique_ptr<PlayerInfo> blackPlayer;
    std::unique_ptr<PlayerInfo> whitePlayer;
};

#endif // GAMEWIDGET_H
