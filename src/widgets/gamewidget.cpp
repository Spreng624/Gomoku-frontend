#include "GameWidget.h"
#include "ui_gamewidget.h"
#include <QMessageBox>
#include <QTime>
#include <QFont>
#include <QFile>
#include <QTextStream>
#include "Logger.h"
#include <QTimer>
#include <QDateTime>

GameWidget::GameWidget(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::GameWidget),
      game(nullptr),
      chessBoard(nullptr),
      gameStatus(NotStarted),
      isLocal(true),
      blackPlayer(nullptr),
      whitePlayer(nullptr)
{
    ui->setupUi(this);
    game = std::make_unique<Game>();
    chessBoard = std::make_unique<ChessBoardWidget>(this);

    QLayout *layout = ui->chessBoardPlaceholder->layout();
    if (!layout)
    {
        layout = new QVBoxLayout(ui->chessBoardPlaceholder);
        layout->setContentsMargins(0, 0, 0, 0);
    }
    layout->addWidget(chessBoard.get());

    SetUpSignals();

    emit init(false);
    LOG_DEBUG("GameWidget constructed successfully");
}

GameWidget::~GameWidget() {}

void GameWidget::SetUpSignals()
{
    // 棋盘信号
    connect(chessBoard.get(), &ChessBoardWidget::makeMove, this, [this](int x, int y)
            {
        if(!isPlaying())
            return;
        // TODO: 先校验，本地交给Game处理，在线游戏交给服务器处理
        // 现在还有问题
        if(!game->isValidMove(x, y)){
            logToUser("无效的走子位置！");
            return;
        }
        if(isLocal)
        {
            game->makeMove(x, y);
            emit onMakeMove(x, y);
        }
        else
        {
            game->makeMove(x, y);
            emit makeMove(x, y);
        }
        LOG_DEBUG_FMT("Player made move at (%d, %d)", x, y); });

    // Part1: 执黑、执白、取消就坐
    connect(ui->player1Avatar, &QPushButton::clicked, this, [this]()
            {
        if (blackPlayer == nullptr) {
            emit takeBlack();
        } else {
            emit cancelTake();
        } });

    connect(ui->player2Avatar, &QPushButton::clicked, this, [this]()
            {
        if (whitePlayer == nullptr) {
            emit takeWhite();
        } else {
            emit cancelTake();
        } });

    // 添加人机按钮
    connect(ui->addAIBlackButton, &QPushButton::clicked, this, [this]()
            {
        if (blackPlayer == nullptr && isLocal) {
            // 添加AI玩家执黑棋
            onBlackTaken("AI玩家(黑)");
            ui->chatHistory->append("<font color='purple'>已添加AI玩家执黑棋</font>");
        } });

    connect(ui->addAIWhiteButton, &QPushButton::clicked, this, [this]()
            {
        if (whitePlayer == nullptr && isLocal) {
            // 添加AI玩家执白棋
            onWhiteTaken("AI玩家(白)");
            ui->chatHistory->append("<font color='purple'>已添加AI玩家执白棋</font>");
        } });

    // Part2: 开始/重新开始、认输、请求和棋、请求悔棋
    connect(ui->startGameButton, &QPushButton::clicked, this, [this]()
            {
        if (gameStatus == NotStarted) {
            emit startGame();
        } else if (gameStatus == End) {
            emit restartGame();
        } });

    connect(ui->surrenderButton, &QPushButton::clicked, this, [this]()
            {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "认输确认", "您确定要认输吗？", QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::No)
            return;
        if (isLocal){
            // 本地模式：直接判定对手胜利
            QString winner;
            if (game->currentPlayer == Piece::BLACK) {
                winner = whitePlayer ? whitePlayer->username : "白棋玩家";
            } else {
                winner = blackPlayer ? blackPlayer->username : "黑棋玩家";
            }
            ui->chatHistory->append(QString("<font color='red'>玩家认输，%1 获胜！</font>").arg(winner));
            onGameEnded("玩家认输结束");
        }else{
            emit giveup();
        } });

    connect(ui->drawButton, &QPushButton::clicked, this, [this]()
            {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "请求和棋", "您确定要请求和棋吗？", QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::No)
            return;
        if (isLocal){
            // 本地模式：直接和局
            ui->chatHistory->append("<font color='orange'>双方同意和棋，游戏结束！</font>");
            onGameEnded("和棋结束");
        }else{
            emit drawRequest();
        } });

    connect(ui->undoButton, &QPushButton::clicked, this, [this]
            {
        if (isLocal) {
            game->undoMove();
        }else{
            emit undoMoveRequest();
        } });

    // Part3: 聊天、记录、玩家列表、设置
    connect(ui->sendButton, &QPushButton::clicked, this, [this]()
            {
        QString message = ui->messageInput->text().trimmed();
        if (!message.isEmpty()) {
            // 乐观响应：立即在聊天记录中显示自己发送的消息
            QString formattedMsg = QString("<b>我:</b> %1").arg(message);
            ui->chatHistory->append(formattedMsg);
            // 自动滚动到底部
            QTextCursor cursor = ui->chatHistory->textCursor();
            cursor.movePosition(QTextCursor::End);
            ui->chatHistory->setTextCursor(cursor);
            
            emit chatMessageSent(message);
            ui->messageInput->clear();
        } });

    connect(ui->messageInput, &QLineEdit::returnPressed, ui->sendButton, &QPushButton::click);

    // connect(ui->enableAICheckBox, &QCheckBox::toggled, this, &GameWidget::aiToggled);

    // Part4: 返回大厅
    connect(ui->backToLobbyButton, &QPushButton::clicked, this, &GameWidget::backToLobby);

    // 弹窗响应（这些信号由外部连接，这里不需要额外连接）
}

// ==================== 公共槽函数实现 ====================

// 游戏初始化和状态变化信号

void GameWidget::init(bool islocal)
{
    gameStatus = NotStarted;
    isLocal = islocal;
    blackPlayer = nullptr;
    whitePlayer = nullptr;

    game->reset();
    game->strictMode = isLocal;
    gameStatus = NotStarted;

    // UI初始化

    // 设置初始文本
    ui->gameIdLabel->setText("#0 桌");
    ui->timeRuleLabel->setText("20+5 分，19路");

    // 设置玩家信息
    ui->player1NameLabel->setText("等待玩家...");
    ui->player1ScoreLabel->setText("等级分: 0");
    ui->player1TimeLabel->setText("20:00");

    ui->player2NameLabel->setText("等待玩家...");
    ui->player2ScoreLabel->setText("等级分: 0");
    ui->player2TimeLabel->setText("20:00");

    // 设置按钮状态
    ui->surrenderButton->setEnabled(false);
    ui->undoButton->setEnabled(false);
    ui->startGameButton->setEnabled(true);    // 开始游戏按钮初始可用
    ui->startGameButton->setText("开始游戏"); // 设置初始文本
    ui->sendButton->setEnabled(true);
    ui->backToLobbyButton->setEnabled(true);

    // 设置AI复选框初始状态（未选中）
    ui->enableAICheckBox->setChecked(false);
    ui->enableAICheckBox->setEnabled(true); // 在游戏开始前可以切换

    // 初始化添加人机按钮状态（默认隐藏，等待initGameWidget设置）
    ui->addAIBlackButton->hide();
    ui->addAIWhiteButton->hide();

    // 更新玩家座位UI
    updatePlayerSeatUI(true);  // 更新黑棋座位
    updatePlayerSeatUI(false); // 更新白棋座位

    // 根据游戏模式设置UI
    if (isLocal)
    {
        // 本地游戏：隐藏聊天tab，只显示记录和玩家tab
        ui->tabWidget->setTabEnabled(0, false); // 隐藏聊天tab
        ui->tabWidget->setCurrentIndex(1);      // 默认显示记录tab

        // 显示添加人机按钮
        ui->addAIBlackButton->show();
        ui->addAIWhiteButton->show();
    }
    else
    {
        // 在线游戏：显示所有tab
        ui->tabWidget->setTabEnabled(0, true); // 显示聊天tab
        ui->tabWidget->setCurrentIndex(0);     // 默认显示聊天tab

        // 隐藏添加人机按钮
        ui->addAIBlackButton->hide();
        ui->addAIWhiteButton->hide();
    }

    LOG_DEBUG("GameWidget UI initialized");

    // 添加欢迎消息
    QString welcomeMsg = islocal ? "本地游戏已就绪" : "在线游戏已就绪";
    ui->chatHistory->append(QString("<font color='gray'>%1</font>").arg(welcomeMsg));
}

void GameWidget::onGameStarted()
{
    LOG_INFO("Game started");
    gameStatus = Playing;

    // 更新UI状态
    ui->gameIdLabel->setText("游戏中");
    ui->surrenderButton->setEnabled(true);
    ui->undoButton->setEnabled(true);
    ui->startGameButton->setEnabled(false);

    // 添加游戏开始消息
    ui->chatHistory->append("<font color='green'>游戏开始！</font>");
}

void GameWidget::onGameEnded(QString message)
{
    LOG_INFO("Game ended: " + message.toStdString());
    gameStatus = End;

    // 更新UI状态
    ui->gameIdLabel->setText("游戏结束");
    ui->surrenderButton->setEnabled(false);
    ui->undoButton->setEnabled(false);
    ui->startGameButton->setEnabled(true);
    ui->startGameButton->setText("重新开始");

    // 添加游戏结束消息
    QString endMsg = message.isEmpty() ? "游戏结束！" : message;
    ui->chatHistory->append(QString("<font color='red'>%1</font>").arg(endMsg));
}

// 棋盘信息（同时更新历史记录）

void GameWidget::onMakeMove(int x, int y)
{
    LOG_DEBUG(QString("Move made at x:%1 y:%2").arg(x).arg(y).toStdString());
    // 这里可以更新走子记录
    QString moveText = QString("走子: (%1, %2)").arg(x).arg(y);
    ui->moveList->addItem(moveText);
}

void GameWidget::onBoardUpdated(const std::vector<std::vector<Piece>> &board)
{
    LOG_DEBUG("Board updated");
    // 棋盘更新由ChessBoardWidget处理，这里可以添加额外逻辑
}

// Part1: 就坐、计时信息

void GameWidget::onBlackTaken(const QString &username)
{
    LOG_DEBUG("Black taken by: " + username.toStdString());

    if (!blackPlayer)
    {
        blackPlayer = std::make_unique<PlayerInfo>();
    }
    blackPlayer->username = username;

    ui->player1NameLabel->setText(username);
    updatePlayerSeatUI(true); // 更新黑棋座位UI

    // 添加聊天消息
    ui->chatHistory->append(QString("<font color='blue'>%1 执黑</font>").arg(username));
}

void GameWidget::onWhiteTaken(const QString &username)
{
    LOG_DEBUG("White taken by: " + username.toStdString());

    if (!whitePlayer)
    {
        whitePlayer = std::make_unique<PlayerInfo>();
    }
    whitePlayer->username = username;

    ui->player2NameLabel->setText(username);
    updatePlayerSeatUI(false); // 更新白棋座位UI

    // 添加聊天消息
    ui->chatHistory->append(QString("<font color='blue'>%1 执白</font>").arg(username));
}

void GameWidget::onBlackTimeUpdate(int playerTime)
{
    // 更新黑棋时间显示
    int minutes = playerTime / 60;
    int seconds = playerTime % 60;
    ui->player1TimeLabel->setText(QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0')));

    // 更新PlayerInfo中的时间
    if (blackPlayer)
    {
        blackPlayer->TimeLeft = QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
    }
}

void GameWidget::onWhiteTimeUpdate(int playerTime)
{
    // 更新白棋时间显示
    int minutes = playerTime / 60;
    int seconds = playerTime % 60;
    ui->player2TimeLabel->setText(QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0')));

    // 更新PlayerInfo中的时间
    if (whitePlayer)
    {
        whitePlayer->TimeLeft = QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
    }
}

// Part3: 聊天、玩家列表、设置

void GameWidget::onChatMessageReceived(const QString &username, const QString &message)
{
    LOG_DEBUG("Chat message from " + username.toStdString() + ": " + message.toStdString());
    QString formattedMsg = QString("<b>%1:</b> %2").arg(username).arg(message);
    ui->chatHistory->append(formattedMsg);

    // 自动滚动到底部
    QTextCursor cursor = ui->chatHistory->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->chatHistory->setTextCursor(cursor);
}

void GameWidget::onUpdateRoomPlayerList(const QStringList &players)
{
    LOG_DEBUG("Updating room player list, count: " + std::to_string(players.size()));
    ui->onlinePlayers->clear();
    for (const QString &player : players)
    {
        ui->onlinePlayers->addItem(player);
    }
}

void GameWidget::onUpdateRoomSetting(const QStringList &settings)
{
    LOG_DEBUG("Room settings updated");
    // 这里可以解析设置并更新UI
    // 例如：时间规则、棋盘大小等
    if (settings.size() >= 1)
    {
        ui->timeRuleLabel->setText(settings[0]);
    }
}

// 弹窗提示: 和棋、悔棋请求与响应

void GameWidget::onDrawRequestReceived()
{
    LOG_DEBUG("Draw request received");

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "和棋请求", "对手请求和棋，是否同意？",
                                  QMessageBox::Yes | QMessageBox::No);

    bool accept = (reply == QMessageBox::Yes);
    emit drawResponse(accept);

    // 添加聊天消息
    QString responseMsg = accept ? "同意了和棋请求" : "拒绝了和棋请求";
    ui->chatHistory->append(QString("<font color='%1'>%2</font>").arg(accept ? "green" : "red").arg(responseMsg));
}

void GameWidget::onDrawResponseReceived(bool accept)
{
    LOG_DEBUG("Draw response received: " + std::to_string(accept));

    QString message = accept ? "对手同意了和棋请求，游戏结束！" : "对手拒绝了和棋请求";
    QMessageBox::information(this, "和棋响应", message);

    // 添加聊天消息
    ui->chatHistory->append(QString("<font color='%1'>%2</font>").arg(accept ? "green" : "orange").arg(message));
}

void GameWidget::onUndoMoveRequestReceived()
{
    LOG_DEBUG("Undo move request received");

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "悔棋请求", "对手请求悔棋，是否同意？",
                                  QMessageBox::Yes | QMessageBox::No);

    bool accept = (reply == QMessageBox::Yes);
    emit undoMoveResponse(accept);

    // 添加聊天消息
    QString responseMsg = accept ? "同意了悔棋请求" : "拒绝了悔棋请求";
    ui->chatHistory->append(QString("<font color='%1'>%2</font>").arg(accept ? "green" : "red").arg(responseMsg));
}

void GameWidget::onUndoMoveResponseReceived(bool accepted)
{
    LOG_DEBUG("Undo move response received: " + std::to_string(accepted));

    QString message = accepted ? "对手同意了悔棋请求" : "对手拒绝了悔棋请求";
    QMessageBox::information(this, "悔棋响应", message);

    // 添加聊天消息
    ui->chatHistory->append(QString("<font color='%1'>%2</font>").arg(accepted ? "green" : "orange").arg(message));
}

// 更新玩家座位UI显示
void GameWidget::updatePlayerSeatUI(bool isBlack)
{
    QPushButton *avatarBtn = isBlack ? ui->player1Avatar : ui->player2Avatar;
    QPushButton *aiBtn = isBlack ? ui->addAIBlackButton : ui->addAIWhiteButton;
    PlayerInfo *player = isBlack ? blackPlayer.get() : whitePlayer.get();

    if (player == nullptr)
    {
        // 座位无人
        if (isBlack)
        {
            avatarBtn->setText("黑棋");
        }
        else
        {
            avatarBtn->setText("白棋");
        }
        avatarBtn->setToolTip("就坐");

        // 右按钮：无人且本地游戏时显示"添加人机"，否则隐藏
        if (isLocal)
        {
            aiBtn->setText("添加人机");
            aiBtn->setToolTip(isBlack ? "添加AI玩家执黑棋" : "添加AI玩家执白棋");
            aiBtn->show();
        }
        else
        {
            aiBtn->hide();
        }
    }
    else
    {
        // 座位有人
        // 左按钮：显示玩家名字
        avatarBtn->setText(player->username);
        avatarBtn->setToolTip(player->username);

        // 右按钮：判断是否是自己或人机
        // 这里简化处理：如果用户名包含"AI"或者是本地玩家，显示"X"
        bool isAI = player->username.contains("AI");
        bool isLocalPlayer = true; // 这里需要实际判断是否本地玩家，暂时简化

        if (isAI || isLocalPlayer)
        {
            aiBtn->setText("X");
            aiBtn->setToolTip("取消就坐");
            aiBtn->show();
        }
        else
        {
            // 别人，隐藏按钮
            aiBtn->hide();
        }
    }
}

// 私有辅助函数

bool GameWidget::isPlaying()
{
    switch (gameStatus)
    {
    case Playing:
        return true;
    case NotStarted:
        emit logToUser("游戏尚未开始");
        return false;
    case End:
        emit logToUser("游戏已结束");
        return false;
    }
}
