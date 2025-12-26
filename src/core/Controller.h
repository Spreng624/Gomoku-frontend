#ifndef MANAGER_H
#define MANAGER_H

#include <QObject>
#include <QThread>
#include <QMainWindow>
#include <QStackedWidget>
#include <QMessageBox>
#include <memory>
#include <string>
#include <map>
#include <functional>
#include <vector>
#include "Game.h"
#include "Packet.h"

// 前向声明
class LobbyWidget;
class RoomWidget;
class GameManager;
class Client;
class Packet;
class QStatusBar;
class QTimer;
class Server;

class Controller : public QObject
{
    Q_OBJECT

public:
    explicit Controller(QObject *parent = nullptr);
    ~Controller();

public slots:
    // NetWork
    void onConnectToServer();

    // StatusBar
    void onLogin(const std::string &username, const std::string &password);
    void onSignin(const std::string &username, const std::string &password);
    void onLoginAsGuest();
    void onLogout();

    // Lobby
    void onCreateRoom();
    void onJoinRoom(int roomId);
    void onQuickMatch();
    void onUpdateLobbyPlayerList();
    void onUpdateLobbyRoomList();

    // Room
    void onSyncSeat(const QString &player1, const QString &player2);
    void onSyncRoomSetting(const QString &configStr);
    void onChatMessage(const QString &message);
    void onSyncUsersToRoom();
    void onExitRoom();

    // Game
    void onGameStarted();
    void onMakeMove(int x, int y);
    void onGiveUp();
    void onDraw(NegStatus status);
    void onUndoMove(NegStatus status);
    void onSyncGame();

signals:
    // 给 MainWindow 发送界面切换信号                         // 提示框
    void connectionStatusChanged(bool connected);                  // 连接状态的信息
    void statusBarMessageChanged(const QString &message);          // 实时信息
    void userIdentityChanged(const QString &username, int rating); // 用户身份信息

    // Lobby
    void updateLobbyPlayerList(const QStringList &players);
    void updateLobbyRoomList(const QStringList &rooms);

    // Room
    void syncSeat(const QString &player1, const QString &player2);
    void syncRoomSetting(const QString &config);
    void chatMessage(const QString &username, const QString &message);
    void SyncUsersToRoom(const QStringList &players);

    // Game
    void initRomeWidget(bool islocal);
    void gameStarted();
    void gameEnded(const QString &msg);
    void makeMove(int x, int y);
    void draw(NegStatus status);
    void undoMove(NegStatus status);
    void syncGame(const QString &statusStr);

    // Else
    void switchWidget(int index); // 切换界面
    void logToUser(QString message);

private slots:

private:
    void handlePacket(const Packet &packet);
    void sendPacket(const Packet &packet);
    void setupSignalConnections();

    // 成员变量
    QMainWindow *mainWindow;
    QStackedWidget *stackedWidget;
    QStatusBar *statusBar;
    LobbyWidget *lobbyWidget;
    RoomWidget *gameWidget;
    std::unique_ptr<Client> client;
    QThread *clientThread;
    QTimer *returnToLobbyTimer;
    std::string serverIp;
    int serverPort;
    bool connected;

    // 会话状态
    uint64_t sessionId;
    std::string username;
    int rating;
    uint32_t currentRoomId;
    bool inGame;
    QString currentUsername;
    int currentRating;
};

#endif // MANAGER_H
