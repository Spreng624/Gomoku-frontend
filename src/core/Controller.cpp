#include "Controller.h"
#include "LobbyWidget.h"
#include "RoomWidget.h"
#include "Client.h"
#include "Packet.h"
#include "Logger.h"
#include <QDebug>
#include <QThread>
#include <QStatusBar>
#include <QTimer>
#include <functional>
#include <chrono>
#include <random>

Controller::Controller(QObject *parent)
    : QObject(parent),
      mainWindow(nullptr),
      stackedWidget(nullptr),
      statusBar(nullptr),
      lobbyWidget(nullptr),
      gameWidget(nullptr),
      client(nullptr),
      clientThread(nullptr),
      returnToLobbyTimer(nullptr),
      serverIp("169.254.56.77"),
      serverPort(8080),
      connected(false),
      sessionId(0),
      rating(0),
      currentRoomId(0),
      inGame(false),
      currentRating(1500)
{
    client = std::make_unique<Client>(serverIp, serverPort);
    client->SetPacketCallback([this](const Packet &packet)
                              { handlePacket(packet); });
    client->SetSessionActivatedCallback([this](uint64_t sessionId)
                                        {
        LOG_INFO("Session activated callback received, sessionId: " + std::to_string(sessionId));
        this->sessionId = sessionId;
        connected = true;
        emit connectionStatusChanged(true);
        onUpdateLobbyPlayerList();
        onUpdateLobbyRoomList();
        emit logToUser("已连接到服务器"); });
    client->SetDisconnectedCallback([this]()
                                    {
        LOG_INFO("Disconnected from server callback triggered");
        connected = false;
        emit connectionStatusChanged(false);
        emit logToUser("与服务器的断连"); });
}

Controller::~Controller()
{
    LOG_INFO("Manager destructor called");
    if (client)
    {
        LOG_DEBUG("Disconnecting client...");
        client->Disconnect();
        client = nullptr;
        LOG_DEBUG("Client disconnected and reset");
    }
    LOG_INFO("Manager cleanup completed");
}

// Network

void Controller::onConnectToServer()
{
    if (connected)
    {
        LOG_DEBUG("Already connected to server, skipping connection attempt");
        logToUser("已经连接到服务器");
        return;
    }
    client->Connect();
}

// Status Bar

void Controller::onLogin(const std::string &username, const std::string &password)
{
    LOG_INFO("Login attempt for user: " + username);

    if (!connected)
    {
        LOG_ERROR("Cannot login: client not connected to server");
        // emit loginFailed("未连接到服务器");
        return;
    }

    Packet packet(sessionId, MsgType::Login);
    packet.AddParam("username", username);
    packet.AddParam("password", password);

    LOG_DEBUG("Login packet created with session ID: " + std::to_string(sessionId));

    LOG_DEBUG("Sending login packet...");
    sendPacket(packet);
    LOG_INFO("Login request sent for user: " + username);
}

void Controller::onSignin(const std::string &username, const std::string &password)
{
    LOG_INFO("Signin attempt for user: " + username);

    if (!connected)
    {
        LOG_ERROR("Cannot signin: client not connected to server");
        emit logToUser("未连接到服务器");
        return;
    }

    Packet packet(sessionId, MsgType::SignIn);
    packet.AddParam("username", username);
    packet.AddParam("password", password); // 不记录实际密码

    sendPacket(packet);
}

void Controller::onLogout()
{
    if (!connected)
        return;

    Packet packet(sessionId, MsgType::LogOut);
    sendPacket(packet);

    username.clear();
    rating = 0;
    currentRoomId = 0;
    inGame = false;
}

void Controller::onLoginAsGuest()
{
    LOG_INFO("Logging in as guest");

    if (!connected)
    {
        LOG_ERROR("Cannot login as guest: client not connected to server");
        return;
    }

    Packet packet(sessionId, MsgType::LoginAsGuest);
    sendPacket(packet);
}

// Lobby

void Controller::onCreateRoom()
{
    LOG_DEBUG("Creating room");
    if (!connected)
    {
        logToUser("未连接到服务器");
        return;
    }

    Packet packet(sessionId, MsgType::CreateRoom);
    sendPacket(packet);
}

void Controller::onJoinRoom(int roomId)
{
    LOG_DEBUG("Joining room: " + std::to_string(roomId));
    if (!connected)
        return;

    Packet packet(sessionId, MsgType::JoinRoom);
    packet.AddParam("roomId", roomId);

    sendPacket(packet);
}

void Controller::onQuickMatch()
{
    if (!connected)
        return;

    Packet packet(sessionId, MsgType::QuickMatch);
    sendPacket(packet);
}

void Controller::onUpdateLobbyPlayerList()
{
    if (!connected)
        return;

    Packet packet(sessionId, MsgType::updateUsersToLobby);
    sendPacket(packet);
}

void Controller::onUpdateLobbyRoomList()
{
    if (!connected)
        return;

    Packet packet(sessionId, MsgType::updateRoomsToLobby);
    sendPacket(packet);
}

// Room

void Controller::onSyncSeat(const QString &player1, const QString &player2)
{
    LOG_INFO("Syncing seat: " + player1.toStdString() + ", " + player2.toStdString());
    if (!connected || currentRoomId == 0)
        return;

    Packet packet(sessionId, MsgType::SyncSeat);
    packet.AddParam("roomId", currentRoomId);
    packet.AddParam("P1", player1.toStdString());
    packet.AddParam("P2", player2.toStdString());
    sendPacket(packet);
}

void Controller::onSyncRoomSetting(const QString &configStr)
{
    LOG_INFO("Syncing room setting: " + configStr.toStdString());
    if (!connected || currentRoomId == 0)
        return;

    Packet packet(sessionId, MsgType::SyncRoomSetting);
    packet.AddParam("roomId", currentRoomId);
    packet.AddParam("config", configStr.toStdString());
    sendPacket(packet);
}

void Controller::onChatMessage(const QString &message)
{
    LOG_INFO("Chat message sent: " + message.toStdString());

    if (!connected || currentRoomId == 0)
        return;

    Packet packet(sessionId, MsgType::ChatMessage);
    packet.AddParam("msg", message.toStdString());
    sendPacket(packet);
    LOG_DEBUG("Chat message sent to room " + std::to_string(currentRoomId) + ": " + message.toStdString());
}

void Controller::onSyncUsersToRoom()
{
    LOG_INFO("Requesting sync users to room");
    if (!connected || currentRoomId == 0)
        return;

    Packet packet(sessionId, MsgType::SyncUsersToRoom);
    packet.AddParam("roomId", currentRoomId);
    sendPacket(packet);
}

void Controller::onExitRoom()
{
    if (!connected || currentRoomId == 0)
        return;

    Packet packet(sessionId, MsgType::ExitRoom);
    packet.AddParam("roomId", currentRoomId);
    sendPacket(packet);

    // 不立即切换，等待服务器确认
    emit statusBarMessageChanged("正在退出房间...");

    // currentRoomId = 0; // 在服务器确认后设置
    // inGame = false;
}

// Game

void Controller::onGameStarted()
{
    LOG_INFO("Starting game");
    if (!connected || currentRoomId == 0)
        return;

    Packet packet(sessionId, MsgType::GameStarted);
    packet.AddParam("roomId", currentRoomId);
    sendPacket(packet);

    inGame = true;
    emit gameStarted();
}

void Controller::onMakeMove(int x, int y)
{
    if (!connected)
    {
        LOG_WARN("Cannot make move: client not connected to server");
        logToUser("未连接到服务器");
        return;
    }

    if (!inGame)
    {
        LOG_WARN("Cannot make move: not currently in a game");
        logToUser("当前不在游戏中，无法落子");
        return;
    }

    Packet packet(sessionId, MsgType::MakeMove);
    packet.AddParam("roomId", currentRoomId);
    packet.AddParam("x", (uint32_t)x);
    packet.AddParam("y", (uint32_t)y);

    sendPacket(packet);
}

void Controller::onDraw(NegStatus status)
{
    Packet packet(sessionId, MsgType::Draw);
    packet.AddParam("negStatus", static_cast<uint8_t>(status));
    sendPacket(packet);
}

void Controller::onUndoMove(NegStatus status)
{
    Packet packet(sessionId, MsgType::UndoMove);
    packet.AddParam("negStatus", static_cast<uint8_t>(status));
    sendPacket(packet);
}

void Controller::onGiveUp()
{
    if (!connected || !inGame)
        return;

    Packet packet(sessionId, MsgType::GiveUp);
    packet.AddParam("roomId", currentRoomId);
    sendPacket(packet);
}

void Controller::onSyncGame()
{
    LOG_INFO("Requesting game sync");
    if (!connected || currentRoomId == 0)
        return;

    Packet packet(sessionId, MsgType::SyncGame);
    packet.AddParam("roomId", currentRoomId);
    sendPacket(packet);
}

// Private

void Controller::handlePacket(const Packet &packet)
{
    LOG_DEBUG("Received packet (type: " + std::to_string(static_cast<int>(packet.msgType)) + ")");
    bool success = packet.GetParam<bool>("success");
    switch (packet.msgType)
    {
    // 用户操作响应
    case MsgType::Login:
    {
        if (success)
        {
            username = packet.GetParam<std::string>("username");
            rating = packet.GetParam<int>("rating");
            emit userIdentityChanged(QString::fromStdString(username), rating);
            emit logToUser("登录成功");
        }
        else
        {
            std::string error = packet.GetParam<std::string>("error", "未知错误");
            emit logToUser(QString::fromStdString("登录失败: " + error));
        }
        break;
    }
    case MsgType::SignIn:
    {
        if (success)
        {
            emit logToUser("注册成功");
        }
        else
        {
            std::string error = packet.GetParam<std::string>("error", "未知错误");
            emit logToUser(QString::fromStdString("注册失败: " + error));
        }
        break;
    }
    case MsgType::LoginAsGuest:
    {
        if (success)
        {
            username = packet.GetParam<std::string>("username");
            rating = packet.GetParam<int>("rating");
            emit userIdentityChanged(QString::fromStdString(username), rating);
            emit logToUser("游客登录成功");
        }
        else
        {
            emit logToUser("游客登录失败");
        }
        break;
    }
    case MsgType::LogOut:
        username.clear();
        rating = 0;
        currentRoomId = 0;
        inGame = false;
        emit userIdentityChanged("", 0);
        emit logToUser("已登出");
        break;

    // 房间操作响应
    case MsgType::CreateRoom:
    {
        bool success = packet.GetParam<bool>("success", true); // 默认为true以保持向后兼容
        if (success)
        {
            currentRoomId = packet.GetParam<int>("roomId");
            LOG_INFO("Room created successfully, room ID: " + std::to_string(currentRoomId));
            emit logToUser(QString("房间创建成功，房间号: %1").arg(currentRoomId));

            // 切换到游戏界面
            emit switchWidget(1);       // 切换到GameWidget
            emit initRomeWidget(false); // 初始化在线游戏模式

            // 更新状态
            inGame = false; // 房间创建但游戏尚未开始
            emit statusBarMessageChanged("已创建房间 " + QString::number(currentRoomId));

            // 可以发射房间创建信号，如果需要的话
            // emit roomCreated(currentRoomId);
        }
        else
        {
            std::string error = packet.GetParam<std::string>("error", "未知错误");
            LOG_ERROR("Failed to create room: " + error);
            emit logToUser(QString::fromStdString("房间创建失败: " + error));
        }
        break;
    }
    case MsgType::JoinRoom:
    {
        bool success = packet.GetParam<bool>("success", true);
        if (success)
        {
            currentRoomId = packet.GetParam<int>("roomId");
            LOG_INFO("Joined room successfully, room ID: " + std::to_string(currentRoomId));
            emit logToUser(QString("加入房间成功，房间号: %1").arg(currentRoomId));

            // 切换到游戏界面
            emit switchWidget(1);
            emit initRomeWidget(false);

            // 更新状态
            inGame = false;
            emit statusBarMessageChanged("已加入房间 " + QString::number(currentRoomId));

            // 发射房间加入信号
            // emit roomJoined(currentRoomId);
        }
        else
        {
            std::string error = packet.GetParam<std::string>("error", "未知错误");
            LOG_ERROR("Failed to join room: " + error);
            emit logToUser(QString::fromStdString("加入房间失败: " + error));
        }
        break;
    }
    case MsgType::ExitRoom:
        currentRoomId = 0;
        inGame = false;
        emit logToUser("已退出房间");
        // 切换回大厅界面
        emit switchWidget(0);
        emit statusBarMessageChanged("已返回大厅");
        break;
    case MsgType::QuickMatch:
    {
        bool success = packet.GetParam<bool>("success", true);
        if (success)
        {
            currentRoomId = packet.GetParam<int>("roomId");
            LOG_INFO("Quick match successful, room ID: " + std::to_string(currentRoomId));
            emit logToUser(QString("快速匹配成功，房间号: %1").arg(currentRoomId));

            // 切换到游戏界面
            emit switchWidget(1);
            emit initRomeWidget(false);

            // 更新状态
            inGame = false;
            emit statusBarMessageChanged("快速匹配到房间 " + QString::number(currentRoomId));

            // 发射快速匹配成功信号
            // emit quickMatchJoined(currentRoomId);
        }
        else
        {
            std::string error = packet.GetParam<std::string>("error", "未知错误");
            LOG_ERROR("Quick match failed: " + error);
            emit logToUser(QString::fromStdString("快速匹配失败: " + error));
        }
        break;
    }

    // 座位操作响应
    // case MsgType::TakeBlack:  // 消息类型在Packet.h中已不存在
    //     break;
    // case MsgType::TakeWhite:  // 消息类型在Packet.h中已不存在
    //     break;
    // case MsgType::CancelTake:  // 消息类型在Packet.h中已不存在
    //     break;
    case MsgType::GameStarted:
        inGame = true;
        emit gameStarted();
        emit logToUser("游戏开始");
        break;

    // 游戏操作响应
    case MsgType::MakeMove:
    {
        int x = packet.GetParam<int>("x");
        int y = packet.GetParam<int>("y");
        emit makeMove(x, y);
        break;
    }
    case MsgType::UndoMove:
    {
        uint8_t negStatus = packet.GetParam<uint8_t>("negStatus", 0);
        break;
    }
    case MsgType::Draw:
    {
        uint8_t negStatus = packet.GetParam<uint8_t>("negStatus", 0);
        break;
    }
    case MsgType::GiveUp:
        emit logToUser("对手认输");
        inGame = false;
        // 对手认输，当前用户获胜
        emit gameEnded(QString("对手认输，你获胜"));
        break;
    case MsgType::updateUsersToLobby:
    {
        // 解析用户列表
        std::string userListStr = packet.GetParam<std::string>("userList", "");
        QStringList users = QString::fromStdString(userListStr).split(',', Qt::SkipEmptyParts);
        emit updateLobbyPlayerList(users);
        break;
    }
    case MsgType::updateRoomsToLobby:
    {
        // 解析房间列表
        std::string roomListStr = packet.GetParam<std::string>("roomList", "");
        QStringList rooms = QString::fromStdString(roomListStr).split(',', Qt::SkipEmptyParts);
        emit updateLobbyRoomList(rooms);
        break;
    }

    // 服务器推送
    case MsgType::GameEnded:
    {
        std::string winner = packet.GetParam<std::string>("winner");
        int ratingChange = packet.GetParam<int>("rating_change");
        emit logToUser(QString("游戏结束，获胜者: %1，积分变化: %2").arg(QString::fromStdString(winner)).arg(ratingChange));
        inGame = false;
        // 判断当前用户是否获胜
        bool won = (winner == username);
        QString msg = won ? QString("你获胜了，积分变化: %1").arg(ratingChange) : QString("你输了，积分变化: %1").arg(ratingChange);
        emit gameEnded(msg);
        break;
    }
    case MsgType::SyncUsersToRoom:
    {
        std::string playerListStr = packet.GetParam<std::string>("playerListStr", "");
        // 解析玩家列表字符串，格式可能是逗号分隔
        QStringList players = QString::fromStdString(playerListStr).split(',', Qt::SkipEmptyParts);
        emit SyncUsersToRoom(players);
        break;
    }
    // case MsgType::PlayerLeft:  // 消息类型在Packet.h中已不存在
    // {
    //     std::string player = packet.GetParam<std::string>("player");
    //     emit logToUser(QString("玩家 %1 离开房间").arg(QString::fromStdString(player)));
    //     // 请求更新房间玩家列表
    //     if (currentRoomId != 0)
    //     {
    //         emit updateRoomPlayerList(QStringList());
    //     }
    //     break;
    // }
    // case MsgType::RoomStatusChanged:  // 消息类型在Packet.h中已不存在
    // {
    //     // 房间状态变化
    //     break;
    // }

    // 新协议消息处理
    case MsgType::SyncSeat:
    {
        std::string player1 = packet.GetParam<std::string>("P1", "");
        std::string player2 = packet.GetParam<std::string>("P2", "");
        emit syncSeat(QString::fromStdString(player1), QString::fromStdString(player2));
        break;
    }
    case MsgType::SyncRoomSetting:
    {
        std::string settings = packet.GetParam<std::string>("config", "");
        emit syncRoomSetting(QString::fromStdString(settings));
        break;
    }
    case MsgType::ChatMessage:
    {
        std::string msg = packet.GetParam<std::string>("msg", "");
        std::string sender = packet.GetParam<std::string>("sender", username);
        emit chatMessage(QString::fromStdString(sender), QString::fromStdString(msg));
        break;
    }
    case MsgType::SyncGame:
    {
        std::string statusStr = packet.GetParam<std::string>("statusStr", "");
        // 根据协议，statusStr包含配置和行棋历史
        // 这里可以解析statusStr并发出相应的信号
        // 暂时直接转发给UI处理
        emit syncGame(QString::fromStdString(statusStr));
        break;
    }

    case MsgType::Error:
    {
        std::string error = packet.GetParam<std::string>("error", "未知错误");
        emit logToUser(QString::fromStdString("错误: " + error));
        break;
    }
    default:
        LOG_DEBUG("Unhandled packet type: " + std::to_string(static_cast<int>(packet.msgType)));
        break;
    }
}

void Controller::sendPacket(const Packet &packet)
{
    LOG_DEBUG("Sent packet (type: " + std::to_string(static_cast<int>(packet.msgType)) + ")");
    client->SendPacket(packet);
}