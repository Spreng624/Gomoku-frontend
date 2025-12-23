#ifndef GAMEMANAGER_H
#define GAMEMANAGER_H

#include <QObject>
#include <QString>
#include <QList>
#include <QPair>
#include <memory>
#include "Game.h"

// 前向声明（保留，因为信号槽中使用了这些类型）
class GameWidget;
class Manager;

class GameManager : public QObject
{
    Q_OBJECT

public:
    explicit GameManager(QObject *parent = nullptr);
    ~GameManager();
    Game *getLocalGame() const;
    void setLocalGame(bool isLocal);

public slots:
    // 从 GameWidget 接收的槽（需要转发给 Manager）
    void onBackToLobby();
    void onTakeBlack();
    void onTakeWhite();
    void onCancelTake();
    void onStartGame();
    void onEditRoomSetting();
    void onChatMessageSent(const QString &message);
    void onMakeMove(int x, int y);
    void onUndoMoveRequested();
    void onUndoMoveResponse(bool accepted);
    void onDrawRequested();
    void onDrawResponse(bool accept);
    void onGiveup();

    // 从 Manager 接收的槽（需要转发给 GameWidget）
    void onChatMessageReceived(const QString &username, const QString &message);
    void onGameStarted(const QString &username, int rating);
    void onGameEnded(const QString &username, int rating, bool won);
    void onPlayerListUpdated(const QStringList &players);
    void onUpdateRoomPlayerList(const QStringList &players);

signals:
    // 转发给 Manager 的信号
    void backToLobby();
    void takeBlack();
    void takeWhite();
    void cancelTake();
    void startGame();
    void editRoomSetting();
    void chatMessageSent(const QString &message);
    void makeMove(int x, int y);
    void undoMoveRequest();
    void undoMoveResponse(bool accepted);
    void drawRequest();
    void drawResponse(bool accept);
    void giveUp();

    // 转发给 GameWidget 的信号
    void chatMessageReceived(const QString &username, const QString &message);
    void gameStarted(const QString &username, int rating);
    void gameEnded(const QString &username, int rating, bool won);
    void playerListUpdated(const QStringList &players);
    void updateRoomPlayerList(const QStringList &players);

    // 本地游戏专用信号
    void localGameBackToLobby();

private:
    // 本地游戏处理方法
    void handleLocalMove(int x, int y);
    void handleLocalUndoRequest();
    void handleLocalUndoResponse(bool accepted);
    void handleLocalDrawRequest();
    void handleLocalDrawResponse(bool accept);
    void handleLocalGiveUp();
    void switchLocalPlayer();
    void checkLocalGameEnd(int x, int y, Piece player);

    // 发送本地游戏消息
    void sendLocalChatMessage(const QString &message);
    void sendLocalGameMessage(const QString &message);

    // 初始化本地游戏
    void initLocalGame();

private:
    // 本地游戏相关成员
    std::unique_ptr<Game> localGame;
    bool isLocalGame;
    bool gameStartedFlag;
    bool gameOverFlag;
    Piece currentPlayer; // 当前执子方
    QString playerBlackName;
    QString playerWhiteName;
    int playerBlackRating;
    int playerWhiteRating;
    bool blackTaken;
    bool whiteTaken;
    bool waitingForUndoResponse;
    bool waitingForDrawResponse;
};

#endif // GAMEMANAGER_H