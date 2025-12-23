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

// 前向声明
class LobbyWidget;
class GameWidget;
class GameManager;
class Client;
class Packet;
class QStatusBar;
class QTimer;
class Server;

class Manager : public QObject
{
    Q_OBJECT

public:
    explicit Manager(QObject *parent = nullptr);
    ~Manager();

    void connectToServer();

public slots:
    // StatusBar
    void login(const std::string &username, const std::string &password);
    void signin(const std::string &username, const std::string &password);
    void loginAsGuest();
    void logout();
    void reConnect();

    // Lobby
    void localGame();
    void createRoom();
    void joinRoom(int roomId);
    void quickMatch();
    void freshLobbyPlayerList();
    void freshLobbyRoomList();

    // GameManager
    void exitRoom();
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

signals:
    // 给 MainWindow 发送界面切换信号
    void switchWidget(int index);                                  // 切换界面
    void logToUser(QString message);                               // 提示框
    void connectionStatusChanged(bool connected);                  // 状态栏                // 连接状态的信息
    void userIdentityChanged(const QString &username, int rating); // 用户身份信息
    void statusBarMessageChanged(const QString &message);          // 实时信息

    // Lobby
    void updateLobbyPlayerList(const QStringList &players);
    void updateLobbyRoomList(const QStringList &rooms);

    // GameWidget
    void initGameWidget(bool islocal);
    void updateRoomPlayerList(const QStringList &players);
    void playerListUpdated(const QStringList &players);
    void chatMessageReceived(const QString &username, const QString &message);
    void gameStarted(const QString &username, int rating);
    void gameEnded(const QString &username, int rating, bool won);

private slots:

private:
    void handlePacket(const Packet &packet);
    void sendPacket(const Packet &packet);

    // 初始化
    void setupSignalConnections();

    // 成员变量
    QMainWindow *mainWindow;
    QStackedWidget *stackedWidget;
    QStatusBar *statusBar;
    LobbyWidget *lobbyWidget;
    GameWidget *gameWidget;
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
