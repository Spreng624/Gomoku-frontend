#include "Manager.h"
#include "LobbyWidget.h"
#include "GameWidget.h"
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

Manager::Manager(QObject *parent)
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
    LOG_INFO("Manager initialized for server");
    client = std::make_unique<Client>(serverIp, serverPort);
    client->SetPacketCallback([this](const Packet &packet)
                              { handlePacket(packet); });
    client->SetSessionActivatedCallback([this](uint64_t sessionId)
                                        {
        LOG_INFO("Session activated callback received, sessionId: " + std::to_string(sessionId));
        this->sessionId = sessionId;
        connected = true;
        emit connectionStatusChanged(true);
        freshLobbyPlayerList();
        freshLobbyRoomList();
        emit logToUser("已连接到服务器"); });
    client->SetDisconnectedCallback([this]()
                                    {
        LOG_INFO("Disconnected from server callback triggered");
        connected = false;
        emit connectionStatusChanged(false);
        emit logToUser("与服务器的断连"); });
    LOG_DEBUG("Session activated callback set");

    setupSignalConnections();
    connectToServer();
}

Manager::~Manager()
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

// 连接管理

void Manager::connectToServer()
{
    if (connected)
    {
        LOG_INFO("Already connected to server, skipping connection attempt");
        return;
    }

    LOG_INFO("Attempting to connect to server...");
    client->Connect();
}

// Status Bar

void Manager::reConnect()
{
    LOG_INFO("Reconnecting to server...");
    connectToServer();
}

void Manager::login(const std::string &username, const std::string &password)
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

void Manager::signin(const std::string &username, const std::string &password)
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

void Manager::logout()
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

void Manager::loginAsGuest()
{
    LOG_INFO("Logging in as guest");

    if (!connected)
    {
        LOG_ERROR("Cannot login as guest: client not connected to server");
        return;
    }

    Packet packet(sessionId, MsgType::LoginByGuest);
    sendPacket(packet);
}

// Lobby
void Manager::localGame()
{
    LOG_DEBUG("Starting local game");
    emit switchWidget(1);
    emit initGameWidget(true);
}

void Manager::createRoom()
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

void Manager::joinRoom(int roomId)
{
    LOG_DEBUG("Joining room: " + std::to_string(roomId));
    if (!connected)
        return;

    Packet packet(sessionId, MsgType::JoinRoom);
    packet.AddParam("roomId", roomId);

    sendPacket(packet);
}

void Manager::quickMatch()
{
    if (!connected)
        return;

    Packet packet(sessionId, MsgType::QuickMatch);
    sendPacket(packet);
}

void Manager::freshLobbyPlayerList()
{
    if (!connected)
        return;

    Packet packet(sessionId, MsgType::GetUserList);
    sendPacket(packet);
}

void Manager::freshLobbyRoomList()
{
    if (!connected)
        return;

    Packet packet(sessionId, MsgType::GetRoomList);
    sendPacket(packet);
}

// GameManager

void Manager::exitRoom()
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

void Manager::takeBlack()
{
    LOG_INFO("Taking black pieces");
    if (!connected || currentRoomId == 0)
        return;

    Packet packet(sessionId, MsgType::TakeBlack);
    packet.AddParam("roomId", currentRoomId);
    sendPacket(packet);
}

void Manager::takeWhite()
{
    LOG_INFO("Taking white pieces");
    if (!connected || currentRoomId == 0)
        return;

    Packet packet(sessionId, MsgType::TakeWhite);
    packet.AddParam("roomId", currentRoomId);
    sendPacket(packet);
}

void Manager::cancelTake()
{
    LOG_INFO("Canceling piece selection");
    if (!connected || currentRoomId == 0)
        return;

    Packet packet(sessionId, MsgType::CancelTake);
    packet.AddParam("roomId", currentRoomId);
    sendPacket(packet);
}

void Manager::startGame()
{
    LOG_INFO("Starting game");
    if (!connected || currentRoomId == 0)
        return;

    Packet packet(sessionId, MsgType::StartGame);
    packet.AddParam("roomId", currentRoomId);
    sendPacket(packet);

    inGame = true;
    emit gameStarted(QString::fromStdString(username), rating);
}

void Manager::editRoomSetting()
{
    LOG_INFO("Editing room settings");
    logToUser("房间设置功能暂未实现");
}

void Manager::chatMessageSent(const QString &message)
{
    LOG_INFO("Chat message sent: " + message.toStdString());

    if (!connected || currentRoomId == 0)
        return;

    // 这里需要发送聊天消息包到服务器
    // 由于 Packet.h 中没有 ChatMessage 类型，暂时只记录日志
    // 可以添加一个临时的消息类型或者使用其他方式
    Packet packet(sessionId, MsgType::ChatMessage);
    packet.AddParam("msg", message.toStdString());
    LOG_DEBUG("Chat message would be sent to room " + std::to_string(currentRoomId) + ": " + message.toStdString());
}

void Manager::makeMove(int x, int y)
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

void Manager::undoMoveRequest()
{
    LOG_INFO("Requesting undo move");
    if (!connected || !inGame)
        return;

    Packet packet(sessionId, MsgType::UndoMoveRequest);
    packet.AddParam("roomId", currentRoomId);
    sendPacket(packet);
}

void Manager::undoMoveResponse(bool accepted)
{
    LOG_INFO("Responding to undo move request: " + std::string(accepted ? "accepted" : "rejected"));
    if (!connected || !inGame)
        return;

    Packet packet(sessionId, MsgType::UndoMoveResponse);
    packet.AddParam("roomId", currentRoomId);
    packet.AddParam("accepted", accepted);
    sendPacket(packet);
}

void Manager::drawRequest()
{
    LOG_INFO("Requesting draw");
    if (!connected || !inGame)
        return;

    Packet packet(sessionId, MsgType::DrawRequest);
    packet.AddParam("roomId", currentRoomId);
    sendPacket(packet);
}

void Manager::drawResponse(bool accept)
{
    LOG_INFO("Responding to draw request: " + std::string(accept ? "accept" : "reject"));
    if (!connected || !inGame)
        return;

    Packet packet(sessionId, MsgType::DrawResponse);
    packet.AddParam("roomId", currentRoomId);
    packet.AddParam("accept", accept);
    sendPacket(packet);
}

void Manager::giveUp()
{
    if (!connected || !inGame)
        return;

    Packet packet(sessionId, MsgType::GiveUp);
    packet.AddParam("roomId", currentRoomId);
    sendPacket(packet);
}

// Private

void Manager::setupSignalConnections()
{
    // connect(this, &Manager::localGameRequested, this, [this]()
    //         {
    //     switchToGame();
    //     setWindowTitle("五子棋 - 本地对战");
    //     updateStatusBar("本地对战模式");
    //     if (gameWidget)
    //     {
    //         gameWidget->resetGame();
    //     } });

    // connect(this, &Manager::onlineGameRequested, this, [this]()
    //         { switchToLobby(); });
}

// Private

void Manager::handlePacket(const Packet &packet)
{
    LOG_DEBUG("Received packet (type: " + std::to_string(static_cast<int>(packet.msgType)) + ")");
    switch (packet.msgType)
    {
    // 用户操作响应
    case MsgType::Login:
    {
        bool success = packet.GetParam<bool>("success");
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
        bool success = packet.GetParam<bool>("success");
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
    case MsgType::LoginByGuest:
    {
        bool success = packet.GetParam<bool>("success");
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
    case MsgType::Guest2User:
    {
        bool success = packet.GetParam<bool>("success");
        if (success)
        {
            username = packet.GetParam<std::string>("username");
            rating = packet.GetParam<int>("rating");
            emit userIdentityChanged(QString::fromStdString(username), rating);
            emit logToUser("游客转用户成功");
        }
        else
        {
            std::string error = packet.GetParam<std::string>("error", "未知错误");
            emit logToUser(QString::fromStdString("游客转用户失败: " + error));
        }
        break;
    }
    case MsgType::EditUsername:
    {
        bool success = packet.GetParam<bool>("success");
        if (success)
        {
            username = packet.GetParam<std::string>("username");
            emit userIdentityChanged(QString::fromStdString(username), rating);
            emit logToUser("用户名修改成功");
        }
        else
        {
            std::string error = packet.GetParam<std::string>("error", "未知错误");
            emit logToUser(QString::fromStdString("用户名修改失败: " + error));
        }
        break;
    }
    case MsgType::EditPassword:
    {
        bool success = packet.GetParam<bool>("success");
        if (success)
        {
            emit logToUser("密码修改成功");
        }
        else
        {
            std::string error = packet.GetParam<std::string>("error", "未知错误");
            emit logToUser(QString::fromStdString("密码修改失败: " + error));
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
            emit initGameWidget(false); // 初始化在线游戏模式

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
    case MsgType::CreateSingleRoom:
    {
        bool success = packet.GetParam<bool>("success", true);
        if (success)
        {
            currentRoomId = packet.GetParam<int>("roomId");
            LOG_INFO("Single player room created successfully, room ID: " + std::to_string(currentRoomId));
            emit logToUser(QString("单人房间创建成功，房间号: %1").arg(currentRoomId));

            // 切换到游戏界面
            emit switchWidget(1);
            emit initGameWidget(false); // 单人房间也是在线模式

            // 更新状态
            inGame = false;
            emit statusBarMessageChanged("已创建单人房间 " + QString::number(currentRoomId));
        }
        else
        {
            std::string error = packet.GetParam<std::string>("error", "未知错误");
            LOG_ERROR("Failed to create single player room: " + error);
            emit logToUser(QString::fromStdString("单人房间创建失败: " + error));
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
            emit initGameWidget(false);

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
            emit initGameWidget(false);

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
    case MsgType::TakeBlack:
        emit logToUser("已选择黑棋");
        break;
    case MsgType::TakeWhite:
        emit logToUser("已选择白棋");
        break;
    case MsgType::CancelTake:
        emit logToUser("已取消选择");
        break;
    case MsgType::StartGame:
        inGame = true;
        emit gameStarted(QString::fromStdString(username), rating);
        emit logToUser("游戏开始");
        break;
    case MsgType::EditRoomSetting:
        emit logToUser("房间设置已更新");
        break;

    // 游戏操作响应
    case MsgType::MakeMove:
    {
        int x = packet.GetParam<int>("x");
        int y = packet.GetParam<int>("y");
        emit moveMade(x, y);
        break;
    }
    case MsgType::UndoMoveRequest:
        emit logToUser("对手请求悔棋");
        break;
    case MsgType::UndoMoveResponse:
    {
        bool accepted = packet.GetParam<bool>("accepted");
        if (accepted)
            emit logToUser("悔棋请求被接受");
        else
            emit logToUser("悔棋请求被拒绝");
        break;
    }
    case MsgType::DrawRequest:
        emit logToUser("对手请求平局");
        break;
    case MsgType::DrawResponse:
    {
        bool accept = packet.GetParam<bool>("accept");
        if (accept)
            emit logToUser("平局请求被接受");
        else
            emit logToUser("平局请求被拒绝");
        break;
    }
    case MsgType::GiveUp:
        emit logToUser("对手认输");
        inGame = false;
        // 对手认输，当前用户获胜
        emit gameEnded(QString::fromStdString(username), rating, true);
        break;

    // 查询和订阅响应
    case MsgType::GetUser:
    {
        // 处理用户信息查询结果
        break;
    }
    case MsgType::GetUserList:
    {
        // 解析用户列表
        std::string userListStr = packet.GetParam<std::string>("userList", "");
        QStringList users = QString::fromStdString(userListStr).split(',', Qt::SkipEmptyParts);
        emit updateLobbyPlayerList(users);
        break;
    }
    case MsgType::GetRoomList:
    {
        // 解析房间列表
        std::string roomListStr = packet.GetParam<std::string>("roomList", "");
        QStringList rooms = QString::fromStdString(roomListStr).split(',', Qt::SkipEmptyParts);
        emit updateLobbyRoomList(rooms);
        break;
    }

    // 服务器推送
    case MsgType::OpponentMoved:
    {
        int x = packet.GetParam<int>("x");
        int y = packet.GetParam<int>("y");
        // emit opponentMoved(x, y);
        emit logToUser(QString("对手落子: (%1, %2)").arg(x).arg(y));
        break;
    }
    case MsgType::GameEnded:
    {
        std::string winner = packet.GetParam<std::string>("winner");
        int ratingChange = packet.GetParam<int>("rating_change");
        emit logToUser(QString("游戏结束，获胜者: %1，积分变化: %2").arg(QString::fromStdString(winner)).arg(ratingChange));
        inGame = false;
        // 判断当前用户是否获胜
        bool won = (winner == username);
        emit gameEnded(QString::fromStdString(winner), rating + ratingChange, won);
        break;
    }
    case MsgType::PlayerJoined:
    {
        std::string player = packet.GetParam<std::string>("player");
        emit logToUser(QString("玩家 %1 加入房间").arg(QString::fromStdString(player)));
        // 假设服务器会发送更新后的玩家列表，这里暂时不更新
        // 可以发送信号让 UI 重新获取列表
        break;
    }
    case MsgType::PlayerLeft:
    {
        std::string player = packet.GetParam<std::string>("player");
        emit logToUser(QString("玩家 %1 离开房间").arg(QString::fromStdString(player)));
        break;
    }
    case MsgType::RoomStatusChanged:
    {
        // 房间状态变化
        break;
    }
    case MsgType::DrawRequested:
        emit logToUser("对手请求平局");
        break;
    case MsgType::DrawAccepted:
        emit logToUser("平局请求被接受");
        inGame = false;
        // 平局，没有明确的获胜者
        emit gameEnded(QString::fromStdString(username), rating, false);
        break;
    case MsgType::GiveUpRequested:
        emit logToUser("对手认输");
        inGame = false;
        // 对手认输，当前用户获胜
        emit gameEnded(QString::fromStdString(username), rating, true);
        break;

    // 错误消息
    case MsgType::Success:
        emit logToUser("操作成功");
        break;
    case MsgType::Error:
    {
        std::string error = packet.GetParam<std::string>("error", "未知错误");
        emit logToUser(QString::fromStdString("错误: " + error));
        break;
    }
    case MsgType::InvalidMsgType:
        emit logToUser("无效的消息类型");
        break;
    case MsgType::InvalidArg:
        emit logToUser("无效的参数");
        break;
    case MsgType::UnexpectedError:
        emit logToUser("服务器内部错误");
        break;

    default:
        LOG_DEBUG("Unhandled packet type: " + std::to_string(static_cast<int>(packet.msgType)));
        break;
    }
}

void Manager::sendPacket(const Packet &packet)
{
    LOG_DEBUG("Sent packet (type: " + std::to_string(static_cast<int>(packet.msgType)) + ")");
    client->SendPacket(packet);
}