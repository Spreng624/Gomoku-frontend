#ifndef GAMEWIDGET_H
#define GAMEWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QList>
#include <QPair>
#include <QString>
#include <QMouseEvent>
#include <memory>
#include "Game.h"
#include "AiPlayer.h"
#include "Timer.hpp"
#include "Packet.h"

// 前向声明组件类
class PlayerInfoPanel;
class GameCtrlPanel;
class FunctionalPanel;

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

class RoomWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RoomWidget(QWidget *parent = nullptr);
    ~RoomWidget();

signals:
    // PlayerInfoPanel
    void SyncSeat(const QString &player1, const QString &player2); // 请求同步座位信息

    // GameCtrlPanel
    void gameStart();
    void makeMove(int x, int y); // 占位 From ChessBoard
    void giveup();
    void draw(NegStatus status);
    void undoMove(NegStatus status);
    void SyncGame();

    // FunctionalPanel
    void syncRoomSetting(const QString &settings);
    void chatMessage(const QString &message);
    void syncUsersToRoom(const QStringList &players);
    void exitRoom();

    // 返回大厅--> MainWindow
    void backToLobby();

    // --> MainWindow
    void logToUser(const QString &message);

public slots:
    void onSyncSeat(const QString &player1, const QString &player2);
    void onSyncRoomSetting(const QString &settings);
    void onChatMessage(const QString &username, const QString &message);
    void onSyncUsersToRoom(const QStringList &players);

    // Game
    void reset(bool islocal);
    void onGameStarted();
    void onGameEnded(QString message);
    void onMakeMove(int x, int y);
    void onDraw(NegStatus status);
    void onUndoMove(NegStatus status);
    void onSyncGame(const QString &str);

private slots:
    // ==================== Game核心回调处理 ====================
    void onBoardUpdated(const std::vector<std::vector<Piece>> &board);

private:
    // 事件处理
    void SwitchPlayerInfoPanal(bool isBlack, bool isTaken);
    void SwitchGameStatus(GameStatus status);

    void paintChessBoard(const std::vector<std::vector<Piece>> &board);
    void paintGameOver(const QString msg);
    void updateChessBoardDisplay();
    QPoint screenPosToGrid(const QPoint &pos, QWidget *boardWidget);

    void SetUpSignals();
    bool isPlaying();
    void handleAIPlayerTurn(Piece player);
    void executeAIMove();

    // 初始化组件
    void initComponents();
    // 连接组件信号
    void connectComponentSignals();

public:
    QString username;

private:
    void SetUpConnections();
    void SetUpChessBoardWidget();
    void SetUpPlayerInfoPanel();
    void SetUpGameCtrlPanel();
    void SetUpFunctionalPanel();

    void checkAndExecuteAI(Piece currPlayer);
    void handleBoardClick(int x, int y);
    bool eventFilter(QObject *watched, QEvent *event) override;
    void drawBoard(QPainter &painter); // 纯绘图，不触发 update

    Ui::GameWidget *ui;
    std::unique_ptr<Game> game;
    GameStatus gameStatus;
    bool isLocal;
    bool isBlackTaken, isWhiteTaken;
    bool isBlackAI, isWhiteAI;
    std::unique_ptr<AiPlayer> blackAI, whiteAI;
};

#endif // GAMEWIDGET_H
