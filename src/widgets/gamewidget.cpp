#include "GameWidget.h"
#include "ui_gamewidget.h"
#include "Manager.h"
#include <QMessageBox>
#include <QTime>
#include <QFont>
#include <QFile>
#include <QTextStream>
#include "Logger.h"
#include <QTimer>
#include <QDateTime>

GameWidget::GameWidget(QWidget *parent)
    : QWidget(parent), ui(new Ui::GameWidget), chessBoard(nullptr),
      m_blackTaken(false), m_whiteTaken(false)
{
    ui->setupUi(this);

    // 初始化UI
    initUI();

    // 设置信号槽连接
    setupConnections();

    LOG_DEBUG("GameWidget constructed successfully");
}

GameWidget::~GameWidget()
{
    delete ui;
    LOG_DEBUG("GameWidget destroyed");
}

void GameWidget::initUI()
{
    // 创建棋盘部件
    chessBoard = new ChessBoardWidget(this);
    QLayout *layout = ui->chessBoardPlaceholder->layout();
    if (!layout)
    {
        layout = new QVBoxLayout(ui->chessBoardPlaceholder);
        layout->setContentsMargins(0, 0, 0, 0);
    }
    layout->addWidget(chessBoard);

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
    ui->sendButton->setEnabled(true);
    ui->backToLobbyButton->setEnabled(true);
    ui->restartButton->setEnabled(false);

    LOG_DEBUG("GameWidget UI initialized");
}

void GameWidget::setupConnections()
{
    // 连接按钮点击信号
    connect(ui->backToLobbyButton, &QPushButton::clicked, this, &GameWidget::onBackToLobbyButtonClicked);
    connect(ui->surrenderButton, &QPushButton::clicked, this, &GameWidget::onSurrenderButtonClicked);
    connect(ui->undoButton, &QPushButton::clicked, this, &GameWidget::onUndoButtonClicked);
    connect(ui->sendButton, &QPushButton::clicked, this, &GameWidget::onSendButtonClicked);
    connect(ui->minimizeBtn, &QPushButton::clicked, this, &GameWidget::onMinimizeBtnClicked);
    connect(ui->closeBtn, &QPushButton::clicked, this, &GameWidget::onCloseBtnClicked);

    // 连接玩家头像点击信号
    connect(ui->player1Avatar, &QPushButton::clicked, this, &GameWidget::onPlayer1AvatarClicked);
    connect(ui->player2Avatar, &QPushButton::clicked, this, &GameWidget::onPlayer2AvatarClicked);

    // 连接消息输入框回车键
    connect(ui->messageInput, &QLineEdit::returnPressed, this, &GameWidget::onSendButtonClicked);

    // 连接棋盘点击信号（乐观UI：立即发送信号，不检查状态）
    if (chessBoard)
    {
        connect(chessBoard, &ChessBoardWidget::moveMade, this, [this](int x, int y)
                {
            qDebug() << "ChessBoardWidget::moveMade signal received, x:" << x << "y:" << y;
            qDebug() << "Emitting makeMove signal (optimistic UI)";
            emit makeMove(x, y); });
    }

    LOG_DEBUG("GameWidget signal connections established");
}

void GameWidget::updatePlayerInfo(int player1Time, int player2Time, bool currentPlayer)
{
    // 更新玩家1时间显示
    int minutes1 = player1Time / 60;
    int seconds1 = player1Time % 60;
    ui->player1TimeLabel->setText(QString("%1:%2").arg(minutes1, 2, 10, QChar('0')).arg(seconds1, 2, 10, QChar('0')));

    // 更新玩家2时间显示
    int minutes2 = player2Time / 60;
    int seconds2 = player2Time % 60;
    ui->player2TimeLabel->setText(QString("%1:%2").arg(minutes2, 2, 10, QChar('0')).arg(seconds2, 2, 10, QChar('0')));

    // 高亮当前玩家
    if (currentPlayer)
    {
        ui->player1Widget->setStyleSheet("background-color: #e8f4fd; border: 1px solid #1a7f37;");
        ui->player2Widget->setStyleSheet("");
    }
    else
    {
        ui->player2Widget->setStyleSheet("background-color: #e8f4fd; border: 1px solid #1a7f37;");
        ui->player1Widget->setStyleSheet("");
    }
}

void GameWidget::updateGameStatus(bool isGameStarted, bool isGameOver)
{
    // 更新游戏状态显示
    if (isGameOver)
    {
        ui->gameIdLabel->setText("游戏结束");
        ui->surrenderButton->setEnabled(false);
        ui->undoButton->setEnabled(false);
    }
    else if (isGameStarted)
    {
        ui->gameIdLabel->setText("游戏中");
        ui->surrenderButton->setEnabled(true);
        ui->undoButton->setEnabled(true);
        ui->restartButton->setEnabled(false);
    }
    else
    {
        ui->gameIdLabel->setText("等待开始");
        ui->surrenderButton->setEnabled(false);
        ui->undoButton->setEnabled(false);
        ui->restartButton->setEnabled(true);
    }
}

// ==================== 从Manager接收的槽函数 ====================

void GameWidget::initGameWidget(bool islocal)
{
    qDebug() << "GameWidget::initGameWidget called, islocal:" << islocal;
    LOG_DEBUG("Initializing GameWidget, islocal: " + std::to_string(islocal));

    // 清空聊天记录
    ui->chatHistory->clear();

    // 清空走子记录
    ui->moveList->clear();

    // 清空在线玩家列表
    ui->onlinePlayers->clear();

    // 重置UI显示
    ui->player1NameLabel->setText("等待玩家...");
    ui->player1ScoreLabel->setText("等级分: 0");
    ui->player1TimeLabel->setText("20:00");

    ui->player2NameLabel->setText("等待玩家...");
    ui->player2ScoreLabel->setText("等级分: 0");
    ui->player2TimeLabel->setText("20:00");

    // 重置就坐状态
    m_blackTaken = false;
    m_whiteTaken = false;
    updatePlayerSeatStatus(false, false);

    // 更新UI状态
    updatePlayerInfo(1200, 1200, true); // 默认20分钟，黑棋先行
    updateGameStatus(false, false);     // 游戏未开始，未结束

    // 添加欢迎消息
    QString welcomeMsg = islocal ? "本地游戏已就绪" : "在线游戏已就绪";
    ui->chatHistory->append(QString("<font color='gray'>%1</font>").arg(welcomeMsg));

    LOG_INFO("GameWidget initialized successfully");
}

void GameWidget::updateRoomPlayerList(const QStringList &players)
{
    LOG_DEBUG("Updating room player list, count: " + std::to_string(players.size()));
    ui->onlinePlayers->clear();
    for (const QString &player : players)
    {
        ui->onlinePlayers->addItem(player);
    }
}

void GameWidget::playerListUpdated(const QStringList &players)
{
    LOG_DEBUG("Player list updated, count: " + std::to_string(players.size()));
    // 这里可以更新玩家显示，但UI中可能没有专门的控件
    // 暂时只记录日志
}

void GameWidget::chatMessageReceived(const QString &username, const QString &message)
{
    LOG_DEBUG("Chat message from " + username.toStdString() + ": " + message.toStdString());
    QString formattedMsg = QString("<b>%1:</b> %2").arg(username).arg(message);
    ui->chatHistory->append(formattedMsg);
    // 自动滚动到底部
    QTextCursor cursor = ui->chatHistory->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->chatHistory->setTextCursor(cursor);
}

void GameWidget::gameStarted()
{
    LOG_INFO("Game started");
    // 更新UI状态
    updateGameStatus(true, false); // 游戏开始，未结束
    // 添加游戏开始消息
    ui->chatHistory->append("<font color='green'>游戏开始！</font>");
}

void GameWidget::gameEnded()
{
    LOG_INFO("Game ended");
    // 更新UI状态
    updateGameStatus(false, true); // 游戏未开始，已结束
    // 添加游戏结束消息
    ui->chatHistory->append("<font color='red'>游戏结束！</font>");
}

// ==================== 从GameManager接收的槽函数 ====================

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

void GameWidget::onGameStarted(const QString &username, int rating)
{
    qDebug() << "GameWidget::onGameStarted called, username:" << username << "rating:" << rating;
    LOG_INFO("Game started for player: " + username.toStdString() + ", rating: " + std::to_string(rating));

    // 设置玩家信息（假设黑棋先行）
    ui->player1NameLabel->setText(username);
    ui->player1ScoreLabel->setText(QString("等级分: %1").arg(rating));

    // 更新UI状态
    updateGameStatus(true, false); // 游戏开始，未结束

    // 添加游戏开始消息
    QString startMsg = QString("游戏开始！%1 执黑先行").arg(username);
    ui->chatHistory->append(QString("<font color='green'>%1</font>").arg(startMsg));
}

void GameWidget::onGameEnded(const QString &username, int rating, bool won)
{
    LOG_INFO("Game ended for player: " + username.toStdString() + ", won: " + std::to_string(won));

    // 更新UI状态
    updateGameStatus(false, true); // 游戏未开始，已结束

    // 添加游戏结果消息
    QString resultMsg = won ? QString("%1 获胜！").arg(username) : QString("%1 失败").arg(username);
    ui->chatHistory->append(QString("<font color='%1'>%2</font>").arg(won ? "green" : "red").arg(resultMsg));
}

void GameWidget::onPlayerListUpdated(const QStringList &players)
{
    LOG_DEBUG("Player list updated, count: " + std::to_string(players.size()));
    ui->onlinePlayers->clear();
    for (const QString &player : players)
    {
        ui->onlinePlayers->addItem(player);
    }

    // 更新玩家就坐状态
    // 假设players列表包含两个玩家：黑棋玩家和白棋玩家
    if (players.size() >= 2)
    {
        QString blackPlayer = players[0];
        QString whitePlayer = players[1];

        // 判断座位是否被占用
        bool blackTaken = (blackPlayer != "等待玩家..." && !blackPlayer.isEmpty());
        bool whiteTaken = (whitePlayer != "等待玩家..." && !whitePlayer.isEmpty());

        updatePlayerSeatStatus(blackTaken, whiteTaken);

        // 更新玩家名称显示
        ui->player1NameLabel->setText(blackPlayer);
        ui->player2NameLabel->setText(whitePlayer);
    }
}

// ==================== 内部槽函数 ====================

void GameWidget::onBackToLobbyButtonClicked()
{
    LOG_DEBUG("Back to lobby button clicked");
    emit backToLobby();
}

void GameWidget::onSurrenderButtonClicked()
{
    LOG_DEBUG("Surrender button clicked");
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "确认认输", "确定要认输吗？",
                                  QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes)
    {
        emit giveup();
    }
}

void GameWidget::onUndoButtonClicked()
{
    LOG_DEBUG("Undo button clicked");
    emit undoMoveRequested();
}

void GameWidget::onSendButtonClicked()
{
    QString message = ui->messageInput->text().trimmed();
    if (!message.isEmpty())
    {
        LOG_DEBUG("Sending chat message: " + message.toStdString());
        emit chatMessageSent(message);
        ui->messageInput->clear();
    }
}

void GameWidget::onMinimizeBtnClicked()
{
    LOG_DEBUG("Minimize button clicked");
    // 最小化窗口（由父窗口处理）
    if (parentWidget())
    {
        parentWidget()->showMinimized();
    }
}

void GameWidget::onCloseBtnClicked()
{
    LOG_DEBUG("Close button clicked");
    // 关闭游戏窗口（由父窗口处理）
    if (parentWidget())
    {
        parentWidget()->close();
    }
}

// ==================== 全量游戏状态更新 ====================

void GameWidget::updateGameState(bool isGameStarted, bool isGameOver, bool currentPlayer,
                                 int player1Time, int player2Time,
                                 const QString &player1Name, const QString &player2Name,
                                 int player1Rating, int player2Rating)
{
    LOG_DEBUG("Updating game state with full data");

    // 更新玩家信息
    ui->player1NameLabel->setText(player1Name);
    ui->player1ScoreLabel->setText(QString("等级分: %1").arg(player1Rating));

    ui->player2NameLabel->setText(player2Name);
    ui->player2ScoreLabel->setText(QString("等级分: %1").arg(player2Rating));

    // 更新时间和当前玩家显示
    updatePlayerInfo(player1Time, player2Time, currentPlayer);

    // 更新游戏状态
    updateGameStatus(isGameStarted, isGameOver);

    // 记录状态更新
    qDebug() << "GameWidget: Game state updated - isGameStarted:" << isGameStarted
             << "isGameOver:" << isGameOver << "currentPlayer:" << currentPlayer;
}

// ==================== 玩家就坐相关函数 ====================

void GameWidget::onPlayer1AvatarClicked()
{
    LOG_DEBUG("Player 1 avatar clicked (black seat)");
    if (!m_blackTaken)
    {
        // 黑棋位置未被占用，触发takeBlack信号
        emit takeBlack();
        // 更新本地状态（实际状态应由GameManager更新后通知回来）
        // 这里先乐观更新
        m_blackTaken = true;
        updatePlayerSeatStatus(m_blackTaken, m_whiteTaken);
    }
    else
    {
        LOG_DEBUG("Black seat already taken");
        // 可以显示提示信息
        ui->chatHistory->append("<font color='orange'>黑棋位置已被占用</font>");
    }
}

void GameWidget::onPlayer2AvatarClicked()
{
    LOG_DEBUG("Player 2 avatar clicked (white seat)");
    if (!m_whiteTaken)
    {
        // 白棋位置未被占用，触发takeWhite信号
        emit takeWhite();
        // 更新本地状态
        m_whiteTaken = true;
        updatePlayerSeatStatus(m_blackTaken, m_whiteTaken);
    }
    else
    {
        LOG_DEBUG("White seat already taken");
        ui->chatHistory->append("<font color='orange'>白棋位置已被占用</font>");
    }
}

void GameWidget::updatePlayerSeatStatus(bool blackTaken, bool whiteTaken)
{
    LOG_DEBUG_FMT("Updating player seat status: blackTaken=%d, whiteTaken=%d",
                  blackTaken ? 1 : 0, whiteTaken ? 1 : 0);

    m_blackTaken = blackTaken;
    m_whiteTaken = whiteTaken;

    // 更新黑棋位置显示
    if (blackTaken)
    {
        ui->player1Avatar->setText("黑棋");
        ui->player1Avatar->setToolTip("黑棋位置已被占用");
        ui->player1Avatar->setEnabled(false);
        ui->player1NameLabel->setText("玩家1");
    }
    else
    {
        ui->player1Avatar->setText("点击就坐");
        ui->player1Avatar->setToolTip("点击就坐（黑棋）");
        ui->player1Avatar->setEnabled(true);
        ui->player1NameLabel->setText("等待玩家...");
    }

    // 更新白棋位置显示
    if (whiteTaken)
    {
        ui->player2Avatar->setText("白棋");
        ui->player2Avatar->setToolTip("白棋位置已被占用");
        ui->player2Avatar->setEnabled(false);
        ui->player2NameLabel->setText("玩家2");
    }
    else
    {
        ui->player2Avatar->setText("点击就坐");
        ui->player2Avatar->setToolTip("点击就坐（白棋）");
        ui->player2Avatar->setEnabled(true);
        ui->player2NameLabel->setText("等待玩家...");
    }
}
