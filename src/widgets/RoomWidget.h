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

struct PlayerInfo
{
    QString username;
    QString TimeLeft;
    bool isAI = false;
    bool isOwner = false;
};

class RoomWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RoomWidget(QWidget *parent = nullptr);
    ~RoomWidget();

signals:
    // PlayerInfoPanel: 执黑、执白、取消就坐（根据新协议通过SyncSeat消息处理）
    void SyncSeat(const QString &player1, const QString &player2); // 请求同步座位信息

    // GameCtrlPanel: 开始/重新开始、认输、请求和棋、请求悔棋（等待响应）
    void startGame();                // 对应MsgType::StartGame
    void giveup();                   // 对应MsgType::GiveUp
    void draw(NegStatus status);     // 对应MsgType::Draw (NegStatus::Ask)
    void undoMove(NegStatus status); // 对应MsgType::UndoMove (NegStatus::Ask)

    // FunctionalPanel: 聊天、记录、玩家列表、设置（乐观响应）
    void chatMessageSent(const QString &message);     // 对应MsgType::ChatMessage
    void syncUsersToRoom(const QStringList &players); // 对应MsgType::SyncUsersToRoom
    void syncRoomSetting(const QString &settings);    // 对应MsgType::SyncRoomSetting

    // ChessBoard
    void makeMove(int x, int y); // 对应MsgType::MakeMove

    // 返回大厅--> MainWindow
    void backToLobby();
    void exitRoom(); // 对应MsgType::ExitRoom

    void syncGame(const QString &str); // 对应MsgType::SyncGame

    // --> MainWindow
    void logToUser(const QString &message);

public slots:
    // 游戏初始化和状态变化信号
    void init(bool islocal);
    void onGameStarted();
    void onGameEnded(QString message);

    // PlayerInfoPanel: 就坐、计时信息
    void onBlackTaken(const QString &username, bool isAI = false);
    void onWhiteTaken(const QString &username, bool isAI = false);
    void onBlackTimeUpdate(const QString &playerTime);
    void onWhiteTimeUpdate(const QString &playerTime);

    // FunctionalPanel: 聊天、玩家列表、设置
    void onChatMessageReceived(const QString &username, const QString &message);
    void onUpdateRoomPlayerList(const QStringList &players);
    void onUpdateRoomSetting(const QStringList &settings);

    // 弹窗提示: 和棋、悔棋请求与响应
    void onDraw(NegStatus status);
    void onUndoMove(NegStatus status);

    // 棋盘操作
    void onMakeMove(int x, int y);
    void onBoardUpdated(const std::vector<std::vector<Piece>> &board);

    // 新协议槽函数
    void onSyncSeat(const QString &player1, const QString &player2);
    void onSyncRoomSetting(const QString &settings);
    void onSyncGame(const QString &str);

private slots:
    // ==================== Game核心回调处理 ====================
    void onGameBoardChanged(const std::vector<std::vector<Piece>> &board);
    void onGameCurrentPlayerChanged(Piece player);
    void onGameEndedFromCore(Piece winner, const std::string &reason);
    void onGameMoveHistoryUpdated(const std::string &history);
    void onGameTimeUpdated(const std::string &blackTime, const std::string &whiteTime);

private:
    // 事件处理
    void SwitchPlayerInfoPanal(bool isBlack, bool isTaken);
    void SwitchGameStatus(GameStatus status);

    bool eventFilter(QObject *watched, QEvent *event) override;
    void paintChessBoard(QPainter &painter, QWidget *boardWidget);
    void updateChessBoardDisplay();
    QPoint screenPosToGrid(const QPoint &pos, QWidget *boardWidget);

    void SetUpSignals();
    bool isPlaying();
    void handleAIPlayerTurn(Piece player);
    void executeAIMove();
    void handleBoardClick(int x, int y);

    // 初始化组件
    void initComponents();
    // 连接组件信号
    void connectComponentSignals();

private:
    void SetUpConnections();
    void SetUpChessBoardWidget();
    void SetUpPlayerInfoPanel();
    void SetUpGameCtrlPanel();
    void SetUpFunctionalPanel();

    Ui::GameWidget *ui;
    std::unique_ptr<Game> game;

    bool isLocal;
    GameStatus gameStatus;
    std::unique_ptr<PlayerInfo> blackPlayer;
    std::unique_ptr<PlayerInfo> whitePlayer;

    // AI玩家
    std::unique_ptr<AiPlayer> aiPlayer;

    // Timer任务ID
    TimerID aiMoveTimerId = 0;
};

#endif // GAMEWIDGET_H
