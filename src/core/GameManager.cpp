#include "GameManager.h"
#include "GameWidget.h"
#include "Manager.h"
#include "Logger.h"
#include "AiPlayer.h"
#include <QTimer>

GameManager::GameManager(QObject *parent) : QObject(parent),
                                            isLocalGame(true),
                                            enableAI(false),
                                            gameStartedFlag(false),
                                            gameOverFlag(false),
                                            currentPlayer(Piece::BLACK),
                                            playerBlackName("玩家1"),
                                            playerWhiteName("玩家2"),
                                            playerBlackRating(1500),
                                            playerWhiteRating(1500),
                                            blackTaken(false),
                                            whiteTaken(false),
                                            waitingForUndoResponse(false),
                                            waitingForDrawResponse(false)
{
    localGame = std::make_unique<Game>();
    aiPlayer = std::make_unique<AiPlayer>(Piece::WHITE); // 默认AI执白棋
}

GameManager::~GameManager()
{
}

Game *GameManager::getLocalGame() const
{
    return localGame.get();
}

void GameManager::setLocalGame(bool isLocal)
{
    setLocalGame(isLocal, false);
}

void GameManager::setLocalGame(bool isLocal, bool enableAI)
{
    LOG_DEBUG_FMT("GameManager::setLocalGame called with isLocal: %d, enableAI: %d", isLocal ? 1 : 0, enableAI ? 1 : 0);
    isLocalGame = isLocal;
    this->enableAI = enableAI;
    if (isLocalGame)
    {
        initLocalGame();
        sendLocalGameMessage("本地游戏已就绪");
    }
    else
    {
        LOG_DEBUG("GameManager::setLocalGame: isLocal is false, resetting game state for online play");
        // 在线游戏也需要重置游戏状态，以便棋盘部件有正确的Game实例
        if (localGame)
        {
            localGame->reset();
        }
        // 重置游戏状态标志
        gameStartedFlag = false;
        gameOverFlag = false;
        currentPlayer = Piece::BLACK;
        blackTaken = false;
        whiteTaken = false;
        waitingForUndoResponse = false;
        waitingForDrawResponse = false;

        qDebug() << "GameManager: Game reset for online play";
    }
}

void GameManager::setCurrentUsername(const QString &username)
{
    currentUsername = username;
}

void GameManager::initLocalGame()
{
    qDebug() << "GameManager::initLocalGame called";
    if (localGame)
    {
        localGame->reset();
    }
    gameStartedFlag = false;
    gameOverFlag = false;
    currentPlayer = Piece::BLACK;

    // 本地游戏中，初始时玩家未就坐，等待玩家选择
    blackTaken = false;
    whiteTaken = false;
    playerBlackName = currentUsername.isEmpty() ? "等待玩家..." : currentUsername;
    playerWhiteName = "等待玩家...";
    playerBlackRating = 1500;
    playerWhiteRating = 1500;

    waitingForUndoResponse = false;
    waitingForDrawResponse = false;

    // 发送初始玩家列表
    QStringList players;
    players << playerBlackName << playerWhiteName;
    emit playerListUpdated(players);

    // 发送游戏就绪消息
    sendLocalGameMessage("本地游戏已就绪，请在设置中启用AI对手，或点击玩家头像选择对手");

    // 注意：不再直接操作GameWidget，通过信号通知UI更新
    // 如果需要更新棋盘当前玩家显示，可以通过信号实现
}

// AI toggle handler
void GameManager::onAIToggled(bool enabled)
{
    LOG_DEBUG_FMT("GameManager::onAIToggled called with enabled: %d", enabled ? 1 : 0);
    if (isLocalGame && !gameStartedFlag)
    {
        enableAI = enabled;
        if (enabled)
        {
            // 启用AI：自动设置AI为白棋玩家
            whiteTaken = true;
            playerWhiteName = "AI玩家";
            sendLocalGameMessage("AI对手已启用，AI将执白棋");
        }
        else
        {
            // 禁用AI：如果AI之前占用白棋位置，现在释放
            if (playerWhiteName == "AI玩家")
            {
                whiteTaken = false;
                playerWhiteName = "等待玩家...";
                sendLocalGameMessage("AI对手已禁用");
            }
        }

        // 更新玩家列表
        QStringList players;
        players << playerBlackName << playerWhiteName;
        emit playerListUpdated(players);
    }
}

// Restart game handler
void GameManager::onRestartGame()
{
    LOG_DEBUG("GameManager::onRestartGame called");
    if (isLocalGame)
    {
        // 重置本地游戏状态
        gameStartedFlag = false;
        gameOverFlag = false;
        currentPlayer = Piece::BLACK;
        blackTaken = false;
        whiteTaken = false;

        // 重新初始化游戏
        initLocalGame();

        // 发送重新开始消息
        sendLocalGameMessage("游戏已重新开始");
    }
    else
    {
        // 在线游戏：可以发送重新开始请求
        // 这里可能需要实现具体的重新开始逻辑
        LOG_INFO("Restart game requested for online game");
    }
}

// 从 GameWidget 接收的槽（需要转发给 Manager）
void GameManager::onBackToLobby()
{
    if (isLocalGame)
    {
        // 本地游戏直接返回大厅
        gameStartedFlag = false;
        gameOverFlag = false;
        sendLocalGameMessage("已退出本地游戏");
        // 发射本地游戏返回大厅信号
        emit localGameBackToLobby();
    }
    else
    {
        // 联网游戏：转发给Manager
        emit backToLobby();
    }
}

void GameManager::onTakeBlack()
{
    qDebug() << "GameManager::onTakeBlack called, isLocalGame:" << isLocalGame;
    if (isLocalGame)
    {
        blackTaken = true;
        // 如果还没有设置玩家名，使用当前用户名
        if (playerBlackName == "等待玩家..." || playerBlackName.isEmpty())
        {
            playerBlackName = currentUsername.isEmpty() ? "玩家" : currentUsername;
        }
        sendLocalChatMessage(playerBlackName + " 选择了黑棋");

        // 发送更新后的玩家列表
        QStringList players;
        players << playerBlackName << playerWhiteName;
        emit playerListUpdated(players);

        qDebug() << "blackTaken:" << blackTaken << "whiteTaken:" << whiteTaken << "gameStartedFlag:" << gameStartedFlag;
        if (blackTaken && whiteTaken && !gameStartedFlag)
        {
            qDebug() << "Both players ready, calling onStartGame";
            onStartGame();
        }
    }
    else
    {
        emit takeBlack();
    }
}

void GameManager::onTakeWhite()
{
    qDebug() << "GameManager::onTakeWhite called, isLocalGame:" << isLocalGame;
    if (isLocalGame)
    {
        whiteTaken = true;
        playerWhiteName = "玩家2";
        sendLocalChatMessage(playerWhiteName + " 选择了白棋");

        // 发送更新后的玩家列表
        QStringList players;
        players << playerBlackName << playerWhiteName;
        emit playerListUpdated(players);

        qDebug() << "blackTaken:" << blackTaken << "whiteTaken:" << whiteTaken << "gameStartedFlag:" << gameStartedFlag;
        if (blackTaken && whiteTaken && !gameStartedFlag)
        {
            qDebug() << "Both players ready, calling onStartGame";
            onStartGame();
        }
    }
    else
    {
        emit takeWhite();
    }
}

void GameManager::onCancelTake()
{
    if (isLocalGame)
    {
        // 本地游戏中取消选择
        if (currentPlayer == Piece::BLACK)
        {
            blackTaken = false;
            sendLocalChatMessage(playerBlackName + " 取消了黑棋选择");
        }
        else
        {
            whiteTaken = false;
            sendLocalChatMessage(playerWhiteName + " 取消了白棋选择");
        }
    }
    else
    {
        emit cancelTake();
    }
}

void GameManager::onStartGame()
{
    if (isLocalGame)
    {
        if (!gameStartedFlag && blackTaken && whiteTaken)
        {
            gameStartedFlag = true;
            gameOverFlag = false;
            currentPlayer = Piece::BLACK;

            qDebug() << "Starting local game, emitting gameStarted signal";
            // 发送游戏开始信号
            emit gameStarted(playerBlackName, playerBlackRating);
            sendLocalGameMessage("游戏开始！" + playerBlackName + " 执黑先行");
        }
        else
        {
            qDebug() << "Cannot start game: conditions not met";
        }
    }
    else
    {
        qDebug() << "Online game, emitting startGame signal";
        emit startGame();
    }
}

void GameManager::onEditRoomSetting()
{
    if (isLocalGame)
    {
        sendLocalChatMessage("本地游戏设置：棋盘大小15x15，无时间限制");
    }
    else
    {
        emit editRoomSetting();
    }
}

void GameManager::onChatMessageSent(const QString &message)
{
    if (isLocalGame)
    {
        sendLocalChatMessage("你: " + message);
    }
    else
    {
        emit chatMessageSent(message);
    }
}

void GameManager::onMakeMove(int x, int y)
{
    LOG_DEBUG_FMT("GameManager::onMakeMove called, x: %d, y: %d, isLocalGame: %d", x, y, isLocalGame ? 1 : 0);
    if (isLocalGame)
    {
        qDebug() << "Local game, calling handleLocalMove";
        handleLocalMove(x, y);
        // 本地游戏在handleLocalMove中会触发棋盘重绘
    }
    else
    {
        qDebug() << "Online game, emitting makeMove signal";
        // 在线游戏：不立即更新棋盘，等待服务器确认，服务器会发送MakeMove包确认落子
        emit makeMove(x, y);
    }
}

void GameManager::onUndoMoveRequested()
{
    if (isLocalGame)
    {
        handleLocalUndoRequest();
    }
    else
    {
        emit undoMoveRequest();
    }
}

void GameManager::onUndoMoveResponse(bool accepted)
{
    if (isLocalGame)
    {
        handleLocalUndoResponse(accepted);
    }
    else
    {
        emit undoMoveResponse(accepted);
    }
}

void GameManager::onDrawRequested()
{
    if (isLocalGame)
    {
        handleLocalDrawRequest();
    }
    else
    {
        emit drawRequest();
    }
}

void GameManager::onDrawResponse(bool accept)
{
    if (isLocalGame)
    {
        handleLocalDrawResponse(accept);
    }
    else
    {
        emit drawResponse(accept);
    }
}

void GameManager::onGiveup()
{
    if (isLocalGame)
    {
        handleLocalGiveUp();
    }
    else
    {
        emit giveUp();
    }
}

// 从 Manager 接收的槽（需要转发给 GameWidget）
void GameManager::onChatMessageReceived(const QString &username, const QString &message)
{
    emit chatMessageReceived(username, message);
}

void GameManager::onGameStarted(const QString &username, int rating)
{
    qDebug() << "GameManager::onGameStarted called, username:" << username << "rating:" << rating;

    // 无论是本地游戏还是在线游戏，游戏开始时设置标志
    gameStartedFlag = true;
    gameOverFlag = false;

    // 在线游戏时，设置玩家名称
    if (!isLocalGame)
    {
        playerBlackName = username; // 当前用户执黑
        playerWhiteName = "对手";   // 暂时设置为对手
        playerBlackRating = rating;
        playerWhiteRating = 1500;     // 默认值
        currentPlayer = Piece::BLACK; // 黑方先手
        blackTaken = true;
        whiteTaken = true; // 假设双方都已就坐
    }

    qDebug() << "GameManager: gameStartedFlag set to true for online game";

    emit gameStarted(username, rating);
}

void GameManager::onGameEnded(const QString &username, int rating, bool won)
{
    emit gameEnded(username, rating, won);
}

void GameManager::onPlayerListUpdated(const QStringList &players)
{
    emit playerListUpdated(players);
}

void GameManager::onMoveMade(int x, int y)
{
    LOG_DEBUG_FMT("GameManager::onMoveMade called, x: %d, y: %d", x, y);
    if (!isLocalGame && localGame && gameStartedFlag && !gameOverFlag)
    {
        // 在线游戏：服务器确认的落子，更新本地棋盘
        // 假设这是对手的落子，用相反的颜色
        Piece opponentPiece = (currentPlayer == Piece::BLACK) ? Piece::WHITE : Piece::BLACK;
        bool success = localGame->makeMove(x, y, opponentPiece);

        if (success)
        {
            LOG_DEBUG("Opponent move applied to local board");
            // 切换当前玩家
            currentPlayer = opponentPiece;
            emit boardUpdated();
        }
        else
        {
            LOG_ERROR("Failed to apply opponent move to local board");
        }
    }
}

void GameManager::onUpdateRoomPlayerList(const QStringList &players)
{
    emit updateRoomPlayerList(players);
}

// ==================== 本地游戏处理方法 ====================

void GameManager::handleLocalMove(int x, int y)
{
    if (!gameStartedFlag || gameOverFlag || !localGame)
    {
        sendLocalGameMessage("游戏未开始或已结束");
        return;
    }

    // 尝试落子
    bool success = localGame->makeMove(x, y, currentPlayer);
    if (!success)
    {
        sendLocalGameMessage("无效的落子位置");
        return;
    }

    // 发送落子消息
    QString playerName = (currentPlayer == Piece::BLACK) ? playerBlackName : playerWhiteName;
    QString piece = (currentPlayer == Piece::BLACK) ? "黑子" : "白子";
    sendLocalGameMessage(playerName + " 在 (" + QString::number(x) + ", " + QString::number(y) + ") 落" + piece);

    // 触发棋盘重绘
    emit boardUpdated();

    // 检查游戏是否结束
    checkLocalGameEnd(x, y, currentPlayer);

    // 如果游戏未结束，切换玩家
    if (!gameOverFlag)
    {
        switchLocalPlayer();

        // 如果启用AI且当前是AI的回合，自动让AI落子
        if (enableAI && ((currentPlayer == Piece::WHITE && playerWhiteName == "AI玩家") ||
                         (currentPlayer == Piece::BLACK && playerBlackName == "AI玩家")))
        {
            // 延迟一点时间再让AI落子，给用户更好的体验
            QTimer::singleShot(500, this, [this]()
                               {
                if (aiPlayer && localGame && gameStartedFlag && !gameOverFlag)
                {
                    auto board = localGame->getBoard();
                    auto aiMove = aiPlayer->getNextMove(board);
                    if (aiMove.first >= 0 && aiMove.second >= 0)
                    {
                        handleLocalMove(aiMove.first, aiMove.second);
                    }
                } });
        }
    }
}

void GameManager::handleLocalUndoRequest()
{
    if (!gameStartedFlag || gameOverFlag || !localGame)
    {
        sendLocalGameMessage("游戏未开始或已结束");
        return;
    }

    if (waitingForUndoResponse)
    {
        sendLocalGameMessage("已在等待悔棋响应");
        return;
    }

    // 本地游戏中，直接询问另一个玩家
    QString currentPlayerName = (currentPlayer == Piece::BLACK) ? playerBlackName : playerWhiteName;
    QString opponentName = (currentPlayer == Piece::BLACK) ? playerWhiteName : playerBlackName;

    sendLocalChatMessage(currentPlayerName + " 请求悔棋，等待 " + opponentName + " 响应");
    waitingForUndoResponse = true;

    // 在本地游戏中，我们可以自动响应或等待用户操作
    // 这里我们发送一个消息，让用户通过UI响应
    sendLocalGameMessage("请" + opponentName + "在聊天框中输入 '同意' 或 '拒绝'");
}

void GameManager::handleLocalUndoResponse(bool accepted)
{
    if (!waitingForUndoResponse)
    {
        sendLocalGameMessage("没有等待处理的悔棋请求");
        return;
    }

    waitingForUndoResponse = false;

    if (accepted)
    {
        // 执行悔棋
        // 注意：Game类的undoMove需要坐标，但这里我们不知道最后一步的坐标
        // 需要修改Game类或在这里记录最后一步
        sendLocalGameMessage("悔棋请求被接受");
        // 暂时不实现具体的悔棋逻辑，需要扩展Game类
    }
    else
    {
        sendLocalGameMessage("悔棋请求被拒绝");
    }
}

void GameManager::handleLocalDrawRequest()
{
    if (!gameStartedFlag || gameOverFlag)
    {
        sendLocalGameMessage("游戏未开始或已结束");
        return;
    }

    if (waitingForDrawResponse)
    {
        sendLocalGameMessage("已在等待平局响应");
        return;
    }

    QString currentPlayerName = (currentPlayer == Piece::BLACK) ? playerBlackName : playerWhiteName;
    QString opponentName = (currentPlayer == Piece::BLACK) ? playerWhiteName : playerBlackName;

    sendLocalChatMessage(currentPlayerName + " 请求平局，等待 " + opponentName + " 响应");
    waitingForDrawResponse = true;

    sendLocalGameMessage("请" + opponentName + "在聊天框中输入 '同意' 或 '拒绝'");
}

void GameManager::handleLocalDrawResponse(bool accept)
{
    if (!waitingForDrawResponse)
    {
        sendLocalGameMessage("没有等待处理的平局请求");
        return;
    }

    waitingForDrawResponse = false;

    if (accept)
    {
        gameOverFlag = true;
        sendLocalGameMessage("平局请求被接受，游戏结束");
        emit gameEnded("双方", 0, false);
    }
    else
    {
        sendLocalGameMessage("平局请求被拒绝");
    }
}

void GameManager::handleLocalGiveUp()
{
    if (!gameStartedFlag || gameOverFlag)
    {
        sendLocalGameMessage("游戏未开始或已结束");
        return;
    }

    QString currentPlayerName = (currentPlayer == Piece::BLACK) ? playerBlackName : playerWhiteName;
    QString opponentName = (currentPlayer == Piece::BLACK) ? playerWhiteName : playerBlackName;

    gameOverFlag = true;
    sendLocalGameMessage(currentPlayerName + " 认输，" + opponentName + " 获胜");

    // 发送游戏结束信号，对手获胜
    bool opponentWon = true;
    emit gameEnded(opponentName, (currentPlayer == Piece::BLACK) ? playerWhiteRating : playerBlackRating, opponentWon);
}

void GameManager::switchLocalPlayer()
{
    currentPlayer = (currentPlayer == Piece::BLACK) ? Piece::WHITE : Piece::BLACK;
    QString playerName = (currentPlayer == Piece::BLACK) ? playerBlackName : playerWhiteName;
    sendLocalGameMessage("轮到 " + playerName + " 落子");

    // 注意：不再直接操作GameWidget，通过信号通知UI更新
    // 如果需要更新棋盘当前玩家显示，可以通过信号实现
}

void GameManager::checkLocalGameEnd(int x, int y, Piece player)
{
    if (!localGame)
        return;

    // 检查是否获胜
    if (localGame->checkWin(x, y, player))
    {
        gameOverFlag = true;
        QString winnerName = (player == Piece::BLACK) ? playerBlackName : playerWhiteName;
        int winnerRating = (player == Piece::BLACK) ? playerBlackRating : playerWhiteRating;

        sendLocalGameMessage(winnerName + " 五子连珠，获胜！");
        emit gameEnded(winnerName, winnerRating, true);
        return;
    }

    // 检查是否平局（棋盘已满）
    if (localGame->isBoardFull())
    {
        gameOverFlag = true;
        sendLocalGameMessage("棋盘已满，平局！");
        emit gameEnded("双方", 0, false);
    }
}

void GameManager::sendLocalChatMessage(const QString &message)
{
    emit chatMessageReceived("系统", message);
}

void GameManager::sendLocalGameMessage(const QString &message)
{
    emit chatMessageReceived("游戏", message);
}