#include "RoomWidget.h"
#include "ui_gamewidget.h"
#include <QMessageBox>
#include <QTime>
#include <QFont>
#include <QFile>
#include <QTextStream>
#include <QTimer>
#include <QDateTime>
#include <QPainter>
#include <QPainterPath>
#include <QDebug>
#include "Logger.h"

RoomWidget::RoomWidget(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::GameWidget),
      game(std::make_unique<Game>()),
      isLocal(true),
      gameStatus(NotStarted),
      isBlackTaken(false),
      isWhiteTaken(false),
      isBlackAI(false),
      isWhiteAI(false),
      blackAI(std::make_unique<AiPlayer>(Piece::BLACK)),
      whiteAI(std::make_unique<AiPlayer>(Piece::WHITE))
{
    ui->setupUi(this);
    initComponents();
}

RoomWidget::~RoomWidget() {}

void RoomWidget::initComponents()
{
    SetUpChessBoardWidget();
    SetUpPlayerInfoPanel();
    SetUpGameCtrlPanel();
    SetUpFunctionalPanel();
    connectComponentSignals();
    reset(false);
}

void RoomWidget::reset(bool localMode)
{
    isLocal = localMode;
    game->setLocalMode(isLocal);
    game->reset();

    isBlackTaken = isWhiteTaken = false;
    isBlackAI = isWhiteAI = false;

    ui->player1NameLabel->setText("等待玩家...");
    ui->player2NameLabel->setText("等待玩家...");

    SwitchPlayerInfoPanal(true, false);
    SwitchPlayerInfoPanal(false, false);
    SwitchGameStatus(GameStatus::NotStarted);

    ui->addAIBlackButton->setVisible(isLocal);
    ui->addAIWhiteButton->setVisible(isLocal);
    update();
}

void RoomWidget::connectComponentSignals()
{
    // Game 核心回调
    game->setOnBoardChanged([this](const auto &)
                            { update(); });
    game->setOnTurnChanged([this](Piece p)
                           { checkAndExecuteAI(p); });
    game->setOnGameStarted([this]
                           { SwitchGameStatus(GameStatus::Playing); });
    game->setOnGameEnded([this](const std::string &msg)
                         {
        paintGameOver(QString::fromStdString(msg));
        SwitchGameStatus(GameStatus::End); });

    game->setOnMoveRequest([this](int x, int y)
                           {
        if (!isLocal) emit makeMove(x, y); });

    // 基础 UI 信号
    connect(ui->backToLobbyButton, &QPushButton::clicked, this, &RoomWidget::backToLobby);
    connect(ui->sendButton, &QPushButton::clicked, this, [this]()
            {
        QString msg = ui->messageInput->text();
        if (!msg.isEmpty()) { emit chatMessage(msg); ui->messageInput->clear(); } });
}

void RoomWidget::SetUpPlayerInfoPanel()
{
    // 玩家就坐逻辑
    auto handleSeat = [this](bool black)
    {
        if (isLocal)
        {
            bool &taken = black ? isBlackTaken : isWhiteTaken;
            if (taken)
            {
                logToUser(black ? "执黑位置已满" : "执白位置已满");
                return;
            }
            taken = true;
            (black ? isBlackAI : isWhiteAI) = false;
            (black ? ui->player1NameLabel : ui->player2NameLabel)->setText(black ? "玩家 1" : "玩家 2");
            SwitchPlayerInfoPanal(black, true);
        }
        else
        {
            emit SyncSeat(black ? username : "", black ? "" : username);
        }
    };

    // AI 就坐逻辑
    auto handleAI = [this](bool black)
    {
        bool &taken = black ? isBlackTaken : isWhiteTaken;
        if (taken)
        {
            logToUser("该位置已占用");
            return;
        }
        taken = true;
        (black ? isBlackAI : isWhiteAI) = true;
        (black ? ui->player1NameLabel : ui->player2NameLabel)->setText("AI 选手");
        SwitchPlayerInfoPanal(black, true);
    };

    connect(ui->player1Avatar, &QPushButton::clicked, this, [=]
            { handleSeat(true); });
    connect(ui->player2Avatar, &QPushButton::clicked, this, [=]
            { handleSeat(false); });
    connect(ui->addAIBlackButton, &QPushButton::clicked, this, [=]
            { handleAI(true); });
    connect(ui->addAIWhiteButton, &QPushButton::clicked, this, [=]
            { handleAI(false); });

    // 取消座位
    connect(ui->cancelBlackButton, &QPushButton::clicked, this, [this]
            {
        if (isLocal) { isBlackTaken = false; SwitchPlayerInfoPanal(true, false); ui->player1NameLabel->setText("等待玩家..."); }
        else emit SyncSeat("", ""); });
    connect(ui->cancelWhiteButton, &QPushButton::clicked, this, [this]
            {
        if (isLocal) { isWhiteTaken = false; SwitchPlayerInfoPanal(false, false); ui->player2NameLabel->setText("等待玩家..."); }
        else emit SyncSeat("", ""); });
}

void RoomWidget::SetUpGameCtrlPanel()
{
    connect(ui->startGameButton, &QPushButton::clicked, this, [this]
            {
        if (isLocal) onGameStarted(); else emit gameStart(); });
    connect(ui->drawButton, &QPushButton::clicked, this, [this]
            { onDraw(isLocal ? NegStatus::Accept : NegStatus::Ask); });
    connect(ui->undoButton, &QPushButton::clicked, this, [this]
            { onUndoMove(isLocal ? NegStatus::Accept : NegStatus::Ask); });
    connect(ui->surrenderButton, &QPushButton::clicked, this, [this]
            {
        if (isLocal) game->end("对局结束"); else emit giveup(); });
}

void RoomWidget::SetUpFunctionalPanel()
{
    connect(ui->soundToggle, &QPushButton::toggled, this, [this](bool chk)
            { ui->soundToggle->setText(chk ? "音效: 開" : "音效: 關"); });
    connect(ui->bgmToggle, &QPushButton::toggled, this, [this](bool chk)
            { ui->bgmToggle->setText(chk ? "背景音樂: 開" : "背景音樂: 關"); });
}
void RoomWidget::onChatMessage(const QString &username, const QString &message)
{
    ui->chatHistory->append(QString("%1: %2").arg(username).arg(message));
}
void RoomWidget::onSyncUsersToRoom(const QStringList &players)
{
    ui->moveList->clear();
    for (const QString &player : players)
        ui->moveList->addItem(player);
}
void RoomWidget::onDraw(NegStatus status)
{
    switch (status)
    {
    case NegStatus::Ask:
    {
        QMessageBox::StandardButton reply =
            QMessageBox::question(this, "和棋请求", "对方请求和棋，是否同意？",
                                  QMessageBox::Yes | QMessageBox::No);

        emit draw(reply == QMessageBox::Yes ? NegStatus::Accept : NegStatus::Reject);
        break;
    }
    case NegStatus::Accept:
        logToUser("和棋请求已同意");
        if (isLocal)
        {
            // 本地模式：直接结束游戏
            if (game)
            {
                game->end("对局结束，和棋");
            }
        }
        // 网络模式：等待服务器同步游戏状态
        break;
    case NegStatus::Reject:
        logToUser("对方拒绝和棋");
        break;
    default:
        LOG_ERROR("Invalid NegStatus");
        break;
    }
}
void RoomWidget::onUndoMove(NegStatus status)
{
    switch (status)
    {
    case NegStatus::Ask:
    {
        QMessageBox::StandardButton reply =
            QMessageBox::question(this, "悔棋请求", "对方请求悔棋，是否同意？",
                                  QMessageBox::Yes | QMessageBox::No);

        emit undoMove(reply == QMessageBox::Yes ? NegStatus::Accept : NegStatus::Reject);
        break;
    }
    case NegStatus::Accept:
        // 同意悔棋，调用Game的undo方法
        if (game)
        {
            bool success = game->undo();
            if (success)
            {
                logToUser("悔棋成功");
            }
            else
            {
                logToUser("悔棋失败：没有可悔的棋步");
            }
        }
        break;
    case NegStatus::Reject:
        logToUser("对方拒绝悔棋");
        break;
    default:
        LOG_ERROR("Invalid NegStatus");
        break;
    }
}
void RoomWidget::onMakeMove(int x, int y)
{
    bool success = game->move(x, y);
}

void RoomWidget::onBoardUpdated(const std::vector<std::vector<Piece>> &board)
{
    // 更新棋盘显示
    updateChessBoardDisplay();
    qDebug() << "RoomWidget::onBoardUpdated: Board updated";
}
void RoomWidget::onGameStarted()
{
    // 逻辑：如果是本地模式，且 (黑位没满 或者 白位没满)
    if (isLocal && (!isBlackTaken || !isWhiteTaken))
    {
        logToUser("请先让两位黑白玩家全部就坐");
        return;
    }
    if (gameStatus == End)
    {
        game->reset();
    }
    game->start();
    gameStatus = GameStatus::Playing;
    update();
}
void RoomWidget::onGameEnded(QString message)
{

    logToUser(message);
    SwitchGameStatus(GameStatus::End);
}

void RoomWidget::onSyncRoomSetting(const QString &settings)
{
    qDebug() << "RoomWidget::onSyncRoomSetting: Received room settings:" << settings;
    // 解析设置字符串并更新UI
    // 这里可以根据实际协议格式进行解析
    // TODO: 根据实际协议格式解析设置并更新UI
    emit logToUser("房间设置已同步");
}

// 辅助函数

void RoomWidget::handleBoardClick(int x, int y)
{
    if (gameStatus != GameStatus::Playing)
    {
        logToUser("游戏尚未开始，请等待");
        return;
    }
    if (isLocal)
        game->move(x, y);
    else
    {
        // 检查是否是自己的回合
        // if (game->getTurn() != currPlayer)
        // {
        //     logToUser("请等待对方落子");
        //     return;
        // }
        // else
        // {
        // }
        emit makeMove(x, y);
    }
}

void RoomWidget::SetUpSignals()
{
    // 设置信号连接
    // 这个函数可能已经由其他函数处理，这里提供一个空实现
    qDebug() << "RoomWidget::SetUpSignals: Setting up signals";
}

void RoomWidget::paintGameOver(QString msg)
{
    QWidget *chessBoardWidget = ui->chessBoardWidget;
    QPainter painter(chessBoardWidget);

    // 棋盘参数
    const int gridSize = 40;
    const int pieceRadius = 18;
    const int boardMargin = 20;
    const int starPointRadius = 4;
    const int boardSize = 15;

    // 计算棋盘实际位置
    int boardLength = gridSize * (boardSize - 1);
    int boardWidth = chessBoardWidget->width();
    int boardHeight = chessBoardWidget->height();
    QPoint boardTopLeft((boardWidth - boardLength) / 2, (boardHeight - boardLength) / 2);

    int indicatorX = boardTopLeft.x() + (boardSize - 1) * gridSize + 30;
    int indicatorY = boardTopLeft.y() + (boardSize / 2) * gridSize;
    const int indicatorRadius = 12;

    painter.setPen(Qt::NoPen);

    int infoX = boardWidth / 2 - 100;
    int infoY = boardHeight / 2 - 30;
    int infoWidth = 200;
    int infoHeight = 60;

    painter.setPen(QPen(QColor(239, 68, 68, 200), 3));
    painter.setBrush(QBrush(QColor(239, 68, 68, 180)));
    painter.drawRect(infoX, infoY, infoWidth, infoHeight);

    painter.setPen(Qt::white);
    painter.setFont(QFont("Microsoft YaHei", 16, QFont::Bold));
    painter.drawText(QRect(infoX, infoY, infoWidth, infoHeight), Qt::AlignCenter, msg);
}

void RoomWidget::updateChessBoardDisplay()
{
    QWidget *chessBoardWidget = ui->chessBoardWidget;
    if (chessBoardWidget)
    {
        chessBoardWidget->update(); // 触发重绘
    }
}

bool RoomWidget::isPlaying()
{
    // 检查游戏是否正在进行中
    return gameStatus == GameStatus::Playing;
}

void RoomWidget::handleAIPlayerTurn(Piece player)
{
    // // 检查当前玩家是否是AI
    // if ((player == Piece::BLACK && blackPlayer && blackPlayer->isAI) ||
    //     (player == Piece::WHITE && whitePlayer && whitePlayer->isAI))
    // {
    //     if (blackPlayer->isAI)
    //     {
    //         aiPlayer = std::make_unique<AiPlayer>(Piece::BLACK);
    //         aiMoveTimerId = TimerManager::AddTask(std::chrono::milliseconds(1000), [this]()
    //                                               { executeAIMove(); }); // 1秒后执行}
    //     }
    //     if (whitePlayer->isAI)
    //     {
    //         aiPlayer = std::make_unique<AiPlayer>(Piece::WHITE);
    //         aiMoveTimerId = TimerManager::AddTask(std::chrono::milliseconds(1000), [this]()
    //                                               { executeAIMove(); }); // 1秒后执行}
    //     }
    // }
}

void RoomWidget::executeAIMove()
{
    // // 执行AI移动
    // if (!aiPlayer || !game)
    // {
    //     return;
    // }

    // const auto &board = game->getBoard();
    // auto move = aiPlayer->getNextMove(board);

    // if (move.first >= 0 && move.second >= 0)
    // {
    //     // 执行移动
    //     onMakeMove(move.first, move.second);
    // }

    // aiMoveTimerId = 0; // 重置计时器ID
}
void RoomWidget::checkAndExecuteAI(Piece currPlayer)
{
    // 核心修复：增加 isBlackAI 和 isWhiteAI 的判断
    bool isAiTurn = (currPlayer == Piece::BLACK && isBlackAI) ||
                    (currPlayer == Piece::WHITE && isWhiteAI);
    if (isAiTurn && gameStatus == Playing)
    {
        QTimer::singleShot(600, this, [this, currPlayer]()
                           {
            // 再次确认状态，防止在延迟期间游戏结束或玩家退出
            if (gameStatus != Playing) return;

            auto* ai = (currPlayer == Piece::BLACK) ? blackAI.get() : whiteAI.get();
            if (ai) {
                auto [x, y] = ai->getNextMove(game->getBoard());
                game->move(x, y);
            } });
    }
}
void RoomWidget::SetUpChessBoardWidget()
{
    ui->chessBoardWidget->setMouseTracking(true);
    ui->chessBoardWidget->installEventFilter(this);
    ui->chessBoardWidget->setStyleSheet("background-color: #E8B96A;");
}

// UI 辅助方法
void RoomWidget::SwitchPlayerInfoPanal(bool isBlack, bool isTaken)
{
    QPushButton *avatar = isBlack ? ui->player1Avatar : ui->player2Avatar;
    QPushButton *aiBtn = isBlack ? ui->addAIBlackButton : ui->addAIWhiteButton;
    QPushButton *cancelBtn = isBlack ? ui->cancelBlackButton : ui->cancelWhiteButton;
    QLabel *nameLabel = isBlack ? ui->player1NameLabel : ui->player2NameLabel;

    avatar->setEnabled(!isTaken);
    aiBtn->setVisible(!isTaken && isLocal);

    // 只有当该位置被占用且(是本地模式或该位置是自己)时才显示取消按钮
    bool isMe = (nameLabel->text() == username);
    cancelBtn->setVisible(isTaken && (isLocal || isMe));
}

void RoomWidget::SwitchGameStatus(GameStatus status)
{
    gameStatus = status;
    ui->startGameButton->setEnabled(status != Playing);
    ui->startGameButton->setText(status == End ? "重新开始" : "开始游戏");
    ui->undoButton->setEnabled(status == Playing);
    ui->drawButton->setEnabled(status == Playing);
    ui->surrenderButton->setEnabled(status == Playing);
}

bool RoomWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->chessBoardWidget)
    {
        // 1. 处理绘图事件
        if (event->type() == QEvent::Paint)
        {
            paintChessBoard(game->getBoard());
            return true; // 告诉 Qt 该事件已处理，不要再画默认背景
        }

        // 2. 处理鼠标点击落子
        if (event->type() == QEvent::MouseButtonPress)
        {
            if (gameStatus != Playing)
                return true;

            QPoint gridPos = screenPosToGrid(static_cast<QMouseEvent *>(event)->pos(), ui->chessBoardWidget);
            if (gridPos.x() >= 0)
                handleBoardClick(gridPos.x(), gridPos.y());
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

// 绘图逻辑保持不变，但使用 member variable 替代 local 变量

void RoomWidget::paintChessBoard(const std::vector<std::vector<Piece>> &board)

{
    QWidget *chessBoardWidget = ui->chessBoardWidget;
    QPainter painter(chessBoardWidget);
    chessBoardWidget->update();
    int boardSize = board.size();
    if (boardSize == 0)
        return;

    // 棋盘参数
    const int gridSize = 40;
    const int pieceRadius = 18;
    const int boardMargin = 20;
    const int starPointRadius = 4;

    // 计算棋盘实际位置
    int boardLength = gridSize * (boardSize - 1);
    int boardWidth = chessBoardWidget->width();
    int boardHeight = chessBoardWidget->height();
    QPoint boardTopLeft((boardWidth - boardLength) / 2, (boardHeight - boardLength) / 2);
    // 绘制棋盘背景
    painter.fillRect(chessBoardWidget->rect(), QColor(222, 184, 135));
    // 绘制网格线
    painter.setPen(QPen(Qt::black, 2));
    for (int i = 0; i < boardSize; i++)
    {
        // 横线
        int y = boardTopLeft.y() + i * gridSize;
        painter.drawLine(boardTopLeft.x(), y, boardTopLeft.x() + boardLength, y);
        // 竖线
        int x = boardTopLeft.x() + i * gridSize;
        painter.drawLine(x, boardTopLeft.y(), x, boardTopLeft.y() + boardLength);
    }

    // 绘制天元和星
    int center = boardSize / 2;
    int starPoints[4][2] = {{3, 3}, {3, 11}, {11, 3}, {11, 11}};
    painter.setBrush(Qt::black);
    painter.setPen(Qt::NoPen);
    // 绘制天元
    QPoint centerPoint(boardTopLeft.x() + center * gridSize,
                       boardTopLeft.y() + center * gridSize);
    painter.drawEllipse(centerPoint, starPointRadius, starPointRadius);

    // 绘制星

    for (auto &point : starPoints)
    {
        if (point[0] < boardSize && point[1] < boardSize)
        {
            QPoint starPoint(boardTopLeft.x() + point[0] * gridSize,
                             boardTopLeft.y() + point[1] * gridSize);
            painter.drawEllipse(starPoint, starPointRadius, starPointRadius);
        }
    }

    // 绘制棋子

    for (int i = 0; i < boardSize; i++)
    {
        for (int j = 0; j < boardSize; j++)
        {
            if (board[i][j] != Piece::EMPTY)
            {
                QPoint piecePos(boardTopLeft.x() + i * gridSize,
                                boardTopLeft.y() + j * gridSize);

                // 绘制棋子阴影
                painter.setBrush(QBrush(QColor(0, 0, 0, 80)));
                painter.setPen(Qt::NoPen);
                painter.drawEllipse(piecePos.x() - pieceRadius + 2,
                                    piecePos.y() - pieceRadius + 2,
                                    pieceRadius * 2, pieceRadius * 2);

                // 绘制棋子
                QRadialGradient gradient(piecePos, pieceRadius);
                if (board[i][j] == Piece::BLACK)

                {

                    gradient.setColorAt(0, QColor(80, 80, 80));

                    gradient.setColorAt(0.7, QColor(40, 40, 40));

                    gradient.setColorAt(1, QColor(0, 0, 0));
                }

                else

                {

                    gradient.setColorAt(0, QColor(255, 255, 255));

                    gradient.setColorAt(0.7, QColor(240, 240, 240));

                    gradient.setColorAt(1, QColor(220, 220, 220));
                }

                painter.setBrush(QBrush(gradient));

                painter.setPen(QPen(QColor(50, 50, 50), 1));

                painter.drawEllipse(piecePos, pieceRadius, pieceRadius);
            }
        }
    }
}

void RoomWidget::onSyncSeat(const QString &p1, const QString &p2)
{
    isBlackTaken = !p1.isEmpty();
    isWhiteTaken = !p2.isEmpty();
    isBlackAI = isWhiteAI = false; // 网络模式暂不考虑 AI

    ui->player1NameLabel->setText(isBlackTaken ? p1 : "等待玩家...");
    ui->player2NameLabel->setText(isWhiteTaken ? p2 : "等待玩家...");

    SwitchPlayerInfoPanal(true, isBlackTaken);
    SwitchPlayerInfoPanal(false, isWhiteTaken);
}

void RoomWidget::onSyncGame(const QString &statusStr)
{
    if (!statusStr.isEmpty() && game->sync(statusStr.toStdString()))
    {
        update();
    }
}

QPoint RoomWidget::screenPosToGrid(const QPoint &pos, QWidget *boardWidget)
{
    const int gridSize = 40;
    const int boardSize = 15;
    int boardLength = gridSize * (boardSize - 1);
    QPoint boardTopLeft((boardWidget->width() - boardLength) / 2, (boardWidget->height() - boardLength) / 2);

    int gx = qRound((float)(pos.x() - boardTopLeft.x()) / gridSize);
    int gy = qRound((float)(pos.y() - boardTopLeft.y()) / gridSize);

    if (gx < 0 || gx >= boardSize || gy < 0 || gy >= boardSize)
        return QPoint(-1, -1);
    return QPoint(gx, gy);
}
