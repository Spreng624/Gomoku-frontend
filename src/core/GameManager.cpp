#include "GameManager.h"
#include "GameWidget.h"
#include "Manager.h"
#include "Logger.h"

GameManager::GameManager(QObject *parent) : QObject(parent),
                                            isLocalGame(true),
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
    LOG_DEBUG_FMT("GameManager::setLocalGame called with isLocal: %d", isLocal ? 1 : 0);
    isLocalGame = isLocal;
    if (isLocalGame)
    {
        initLocalGame();
        sendLocalGameMessage("本地游戏已就绪");
    }
    else
    {
        LOG_DEBUG("GameManager::setLocalGame: isLocal is false, not initializing local game");
    }
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

    // 本地游戏中，自动分配玩家
    blackTaken = true;
    whiteTaken = true;
    playerBlackName = "玩家1 (黑棋)";
    playerWhiteName = "玩家2 (白棋)";
    playerBlackRating = 1500;
    playerWhiteRating = 1500;

    waitingForUndoResponse = false;
    waitingForDrawResponse = false;

    // 发送初始玩家列表
    QStringList players;
    players << playerBlackName << playerWhiteName;
    emit playerListUpdated(players);

    // 本地游戏自动开始
    gameStartedFlag = true;
    qDebug() << "Local game auto-started, emitting gameStarted signal";
    emit gameStarted(playerBlackName, playerBlackRating);
    sendLocalGameMessage("本地游戏已开始！" + playerBlackName + " 执黑先行");

    // 注意：不再直接操作GameWidget，通过信号通知UI更新
    // 如果需要更新棋盘当前玩家显示，可以通过信号实现
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
        playerBlackName = "玩家1";
        sendLocalChatMessage(playerBlackName + " 选择了黑棋");
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
    qDebug() << "GameManager::onStartGame called, isLocalGame:" << isLocalGame;
    if (isLocalGame)
    {
        qDebug() << "Local game: gameStartedFlag:" << gameStartedFlag << "blackTaken:" << blackTaken << "whiteTaken:" << whiteTaken;
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
    }
    else
    {
        qDebug() << "Online game, emitting makeMove signal";
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

    // 检查游戏是否结束
    checkLocalGameEnd(x, y, currentPlayer);

    // 如果游戏未结束，切换玩家
    if (!gameOverFlag)
    {
        switchLocalPlayer();
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