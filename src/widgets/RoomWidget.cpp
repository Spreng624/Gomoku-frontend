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
      game(nullptr),
      isLocal(true),
      gameStatus(NotStarted),
      blackPlayer(nullptr),
      whitePlayer(nullptr),
      aiPlayer(nullptr),
      aiMoveTimerId(0)
{
    ui->setupUi(this);
    game = std::make_unique<Game>(); // Game核心

    // 初始化组件
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
    SwitchPlayerInfoPanal(true, false);
    SwitchPlayerInfoPanal(false, false);
    SwitchGameStatus(GameStatus::NotStarted);
    // if (isLocal)
    // {
    //     onBlackTaken("Player 1", false);
    //     onWhiteTaken("Player 2", false);
    // }
}

void RoomWidget::connectComponentSignals()
{
    // 连接游戏核心回调
    if (game)
    {
        game->setOnBoardChanged([this](const std::vector<std::vector<Piece>> &board)
                                { onGameBoardChanged(board); });
        game->setOnCurrentPlayerChanged([this](Piece player)
                                        { onGameCurrentPlayerChanged(player); });
        game->setOnGameEnded([this](Piece winner, const std::string &reason)
                             { onGameEndedFromCore(winner, reason); });
    }

    // 连接UI信号
    SetUpConnections();
}

void RoomWidget::SetUpConnections()
{
    // 连接按钮信号
    connect(ui->startGameButton, &QPushButton::clicked, this, [this]()
            { emit startGame(); });
    connect(ui->backToLobbyButton, &QPushButton::clicked, this, [this]()
            { emit backToLobby(); });
    connect(ui->sendButton, &QPushButton::clicked, this, [this]()
            {
        QString message = ui->messageInput->text();
        if (!message.isEmpty()) {
            emit chatMessageSent(message);
            ui->messageInput->clear();
        } });
}

void RoomWidget::SetUpPlayerInfoPanel()
{
    // 连接玩家就坐按钮（新协议）
    connect(ui->player1Avatar, &QPushButton::clicked, this, [this]()
            { if(isLocal) onBlackTaken("Player 1", false);
                else {
                    emit SyncSeat("玩家1", "玩家2"); // 0表示黑棋
                } });
    connect(ui->player2Avatar, &QPushButton::clicked, this, [this]()
            { if(isLocal) onWhiteTaken("Player 2", false);
                else {
                    // 发送请求选择白棋座位
                    emit SyncSeat("玩家1", "玩家2"); // 1表示白棋
                } });

    // 连接AI按钮
    connect(ui->addAIBlackButton, &QPushButton::clicked, this, [this]()
            { onBlackTaken("AI Player", true); });
    connect(ui->addAIWhiteButton, &QPushButton::clicked, this, [this]()
            { onWhiteTaken("AI Player", true); });

    // 连接取消按钮（新协议）
    connect(ui->cancelBlackButton, &QPushButton::clicked, this, [this]()
            {if(isLocal) {
                blackPlayer=nullptr;
                SwitchPlayerInfoPanal(true, false);
                ui->player1NameLabel->setText("等待玩家...");
            } else {
                // 发送请求取消黑棋座位
                emit SyncSeat("玩家1", "玩家2"); // 2表示取消
            } });
    connect(ui->cancelWhiteButton, &QPushButton::clicked, this, [this]()
            {
        if(isLocal) {
            whitePlayer=nullptr;
            SwitchPlayerInfoPanal(false, false);
            ui->player2NameLabel->setText("等待玩家...");
        } else {
            // 发送请求取消白棋座位
            emit SyncSeat("玩家1", "玩家2"); // 2表示取消
        } });
}

void RoomWidget::SetUpGameCtrlPanel()
{
    // 连接游戏控制按钮
    connect(ui->startGameButton, &QPushButton::clicked, this, [this]()
            { if(isLocal) onGameStarted(); 
              else emit startGame(); });
    connect(ui->drawButton, &QPushButton::clicked, this, [this]()
            { if(isLocal) onDraw(NegStatus::Accept); 
              else emit draw(NegStatus::Ask); });
    connect(ui->undoButton, &QPushButton::clicked, this, [this]()
            { if(isLocal) onUndoMove(NegStatus::Accept); 
              else emit undoMove(NegStatus::Ask); });
    connect(ui->surrenderButton, &QPushButton::clicked, this, [this]()
            { if(isLocal) game->end(); 
              else emit giveup(); });
}

void RoomWidget::SetUpFunctionalPanel()
{
    // 连接功能面板
    connect(ui->enableAICheckBox, &QCheckBox::stateChanged, this, [this](int state)
            {
        bool enabled = (state == Qt::Checked);
        qDebug() << "AI enabled:" << enabled; });

    // 连接设置按钮
    connect(ui->soundToggle, &QPushButton::toggled, this, [this](bool checked)
            { ui->soundToggle->setText(checked ? "音效: 開" : "音效: 關"); });
    connect(ui->bgmToggle, &QPushButton::toggled, this, [this](bool checked)
            { ui->bgmToggle->setText(checked ? "背景音樂: 開" : "背景音樂: 關"); });
}

void RoomWidget::SetUpChessBoardWidget()
{
    // 获取棋盘组件指针
    QWidget *chessBoardWidget = ui->chessBoardWidget;
    if (!chessBoardWidget)
    {
        LOG_ERROR("chessBoardWidget is null");
        return;
    }

    // 启用鼠标跟踪和点击
    chessBoardWidget->setMouseTracking(true);
    chessBoardWidget->installEventFilter(this);
    chessBoardWidget->setStyleSheet("background-color: #E8B96A;");

    if (game)
    {
        updateChessBoardDisplay();
    }
}

void RoomWidget::onBlackTaken(const QString &username, bool isAI)
{
    if (isLocal && blackPlayer != nullptr)
    {
        logToUser("Black has been taken");
    }
    else
    {
        blackPlayer = std::make_unique<PlayerInfo>();
        blackPlayer->username = username;
        blackPlayer->TimeLeft = "00:00:00";
        blackPlayer->isAI = isLocal && isAI;
        ui->player1NameLabel->setText(blackPlayer->username);
        ui->player1TimeLabel->setText(blackPlayer->TimeLeft);
        SwitchPlayerInfoPanal(true, true);
    }
}

void RoomWidget::onWhiteTaken(const QString &username, bool isAI)
{
    if (isLocal && whitePlayer != nullptr)
    {
        logToUser("White has been taken");
    }
    else
    {
        whitePlayer = std::make_unique<PlayerInfo>();
        whitePlayer->username = username;
        whitePlayer->TimeLeft = "00:00:00";
        whitePlayer->isAI = isLocal && isAI;
        ui->player2NameLabel->setText(whitePlayer->username);
        ui->player2TimeLabel->setText(whitePlayer->TimeLeft);
        SwitchPlayerInfoPanal(false, true);
    }
}

void RoomWidget::onBlackTimeUpdate(const QString &playerTime)
{
    ui->player1TimeLabel->setText(playerTime);
}
void RoomWidget::onWhiteTimeUpdate(const QString &playerTime)
{
    ui->player2TimeLabel->setText(playerTime);
}
void RoomWidget::onChatMessageReceived(const QString &username, const QString &message)
{
    ui->chatHistory->append(QString("%1: %2").arg(username).arg(message));
}
void RoomWidget::onUpdateRoomPlayerList(const QStringList &players)
{
    ui->moveList->clear();
    for (const QString &player : players)
        ui->moveList->addItem(player);
}
void RoomWidget::onUpdateRoomSetting(const QStringList &settings)
{
    // TODO: 待定
}

void RoomWidget::onDraw(NegStatus status)
{
    switch (status)
    {
    case NegStatus::Ask:
        QMessageBox::StandardButton reply =
            QMessageBox::question(this, "和棋请求", "对方请求和棋，是否同意？",
                                  QMessageBox::Yes | QMessageBox::No);

        emit draw(reply == QMessageBox::Yes ? NegStatus::Accept : NegStatus::Reject);
        break;
    case NegStatus::Accept:
        game->end();
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
        QMessageBox::StandardButton reply =
            QMessageBox::question(this, "悔棋请求", "对方请求悔棋，是否同意？",
                                  QMessageBox::Yes | QMessageBox::No);

        emit draw(reply == QMessageBox::Yes ? NegStatus::Accept : NegStatus::Reject);
        break;
    case NegStatus::Accept:
        game->end();
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
    // 获取当前玩家
    Piece currentPlayer = game->getCurrentPlayer();

    // 尝试落子
    bool success = game->move(x, y);
    if (success)
    {
        // 更新棋盘显示
        updateChessBoardDisplay();

        // 检查游戏是否结束
        if (game->isGameOver())
        {
            Piece winner = game->getWinner();
            QString winnerText = (winner == Piece::BLACK) ? "黑棋" : "白棋";
            qDebug() << "RoomWidget::onMakeMove: Game over, winner:" << winnerText;
        }

        // 发射信号通知其他组件
        emit makeMove(x, y);
    }
    else
    {
        qDebug() << "RoomWidget::onMakeMove: Invalid move at (" << x << "," << y << ")";
    }
}
void RoomWidget::onBoardUpdated(const std::vector<std::vector<Piece>> &board)
{
    // 更新棋盘显示
    updateChessBoardDisplay();
    qDebug() << "RoomWidget::onBoardUpdated: Board updated";
}

void RoomWidget::init(bool islocal)
{
    isLocal = islocal;
    qDebug() << "RoomWidget::init: isLocal =" << isLocal;

    // 根据模式初始化
    if (isLocal)
    {
        ui->addAIBlackButton->setVisible(true);
        ui->addAIWhiteButton->setVisible(true);
    }
    else
    {
        ui->addAIBlackButton->setVisible(false);
        ui->addAIWhiteButton->setVisible(false);
    }
}

void RoomWidget::onGameStarted()
{
    if (isLocal && (blackPlayer == nullptr || whitePlayer == nullptr))

        SwitchGameStatus(GameStatus::Playing);
}

void RoomWidget::onGameEnded(QString message)
{

    logToUser(message);
    SwitchGameStatus(GameStatus::End);
}

// private slots

void RoomWidget::SwitchPlayerInfoPanal(bool isBlack, bool isTaken)
{
    if (isBlack)
    {
        if (isTaken && !blackPlayer)
            LOG_WARN("blackPlayer is null");
        ui->player1Avatar->setEnabled(!isTaken);
        if (!isTaken && isLocal)
            ui->addAIBlackButton->show();
        else
            ui->addAIBlackButton->hide();
        if (isTaken)
            ui->cancelBlackButton->show();
        else
            ui->cancelBlackButton->hide();
    }
    else
    {
        if (isTaken && !whitePlayer)
            LOG_WARN("whitePlayer is null");
        ui->player2Avatar->setEnabled(!isTaken);
        if (!isTaken && isLocal)
            ui->addAIWhiteButton->show();
        else
            ui->addAIWhiteButton->hide();
        if (isTaken)
            ui->cancelWhiteButton->show();
        else
            ui->cancelWhiteButton->hide();
    }
}

void RoomWidget::SwitchGameStatus(GameStatus status)
{
    gameStatus = status;
    switch (status)
    {
    case GameStatus::NotStarted:
        ui->startGameButton->setEnabled(true);
        ui->startGameButton->setText("开始游戏");
        ui->undoButton->setEnabled(false);
        ui->drawButton->setEnabled(false);
        ui->surrenderButton->setEnabled(false);
        break;
    case GameStatus::Playing:
        ui->startGameButton->setEnabled(false);
        ui->undoButton->setEnabled(true);
        ui->drawButton->setEnabled(true);
        ui->surrenderButton->setEnabled(true);

        break;
    case GameStatus::End:
        ui->startGameButton->setEnabled(true);
        ui->startGameButton->setText("重新开始");
        ui->undoButton->setEnabled(false);
        ui->drawButton->setEnabled(false);
        ui->surrenderButton->setEnabled(false);
        break;
    default:
        break;
    }
}

void RoomWidget::onGameBoardChanged(const std::vector<std::vector<Piece>> &board)
{
    onBoardUpdated(board);
}

void RoomWidget::onGameCurrentPlayerChanged(Piece player)
{
    updateChessBoardDisplay();
}

void RoomWidget::onGameEndedFromCore(Piece winner, const std::string &reason)
{
    QString winnerText = (winner == Piece::BLACK) ? "黑棋" : "白棋";
    QString message = QString("%1 获胜！原因：%2").arg(winnerText).arg(QString::fromStdString(reason));

    onGameEnded(message);

    // 更新棋盘显示以显示游戏结束信息
    updateChessBoardDisplay();
}

void RoomWidget::onGameMoveHistoryUpdated(const std::string &history)
{
    // TODO: ui->tabWidget “记录”页 Text替换为history
}

void RoomWidget::onGameTimeUpdated(const std::string &blackTime, const std::string &whiteTime)
{
    ui->player1TimeLabel->setText(QString::fromStdString(blackTime));
    ui->player2TimeLabel->setText(QString::fromStdString(whiteTime));
}

void RoomWidget::handleBoardClick(int x, int y)
{
    qDebug() << "RoomWidget::handleBoardClick: Board clicked at (" << x << "," << y << ")";

    // 处理棋盘点击事件
    onMakeMove(x, y);
}

bool RoomWidget::eventFilter(QObject *watched, QEvent *event)
{
    QWidget *chessBoardWidget = ui->chessBoardWidget;
    if (!chessBoardWidget || watched != chessBoardWidget)
    {
        LOG_ERROR("Invalid event filter");
        return QWidget::eventFilter(watched, event);
    }
    // 鼠标点击事件 -> 落子
    if (event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        QPoint gridPos = screenPosToGrid(mouseEvent->pos(), chessBoardWidget);
        if (gridPos.x() >= 0 && gridPos.y() >= 0)
        {
            handleBoardClick(gridPos.x(), gridPos.y());
            return true; // 事件已处理
        }
    }
    // update触发绘制事件 -> 绘制棋盘
    else if (event->type() == QEvent::Paint)
    {
        // 绘制棋盘
        QPaintEvent *paintEvent = static_cast<QPaintEvent *>(event);
        QPainter painter(chessBoardWidget);
        paintChessBoard(painter, chessBoardWidget);
        return true;
    }

    return QWidget::eventFilter(watched, event);
}

void RoomWidget::paintChessBoard(QPainter &painter, QWidget *boardWidget)
{
    if (!game)
        return;

    const std::vector<std::vector<Piece>> &board = game->getBoard();
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
    int boardWidth = boardWidget->width();
    int boardHeight = boardWidget->height();
    QPoint boardTopLeft((boardWidth - boardLength) / 2, (boardHeight - boardLength) / 2);

    // 绘制棋盘背景
    painter.fillRect(boardWidget->rect(), QColor(222, 184, 135));

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

    // 绘制当前玩家指示器
    if (!game->isGameOver())
    {
        Piece currentPlayer = game->getCurrentPlayer();
        int indicatorX = boardTopLeft.x() + (boardSize - 1) * gridSize + 30;
        int indicatorY = boardTopLeft.y() + (boardSize / 2) * gridSize;
        const int indicatorRadius = 12;

        painter.setPen(Qt::NoPen);
        if (currentPlayer == Piece::BLACK)
        {
            // 黑棋指示器
            QRadialGradient blackGrad(indicatorX, indicatorY, indicatorRadius);
            blackGrad.setColorAt(0, QColor(100, 100, 100));
            blackGrad.setColorAt(1, QColor(0, 0, 0));
            painter.setBrush(QBrush(blackGrad));
            painter.drawEllipse(indicatorX - indicatorRadius, indicatorY - indicatorRadius,
                                indicatorRadius * 2, indicatorRadius * 2);

            painter.setPen(QPen(Qt::black, 2));
            painter.setFont(QFont("Microsoft YaHei", 12, QFont::Bold));
            painter.drawText(indicatorX + 15, indicatorY + 5, "黑方回合");
        }
        else
        {
            // 白棋指示器
            QRadialGradient whiteGrad(indicatorX, indicatorY, indicatorRadius);
            whiteGrad.setColorAt(0, QColor(255, 255, 255));
            whiteGrad.setColorAt(1, QColor(200, 200, 200));
            painter.setBrush(QBrush(whiteGrad));
            painter.setPen(QPen(Qt::black, 1));
            painter.drawEllipse(indicatorX - indicatorRadius, indicatorY - indicatorRadius,
                                indicatorRadius * 2, indicatorRadius * 2);

            painter.setPen(QPen(Qt::white, 2));
            painter.setFont(QFont("Microsoft YaHei", 12, QFont::Bold));
            painter.drawText(indicatorX + 15, indicatorY + 5, "白方回合");
        }
    }

    // 如果游戏结束，绘制获胜信息
    if (game->isGameOver())
    {
        Piece winner = game->getWinner();
        QString winnerText = (winner == Piece::BLACK) ? "黑棋" : "白棋";

        int infoX = boardWidth / 2 - 100;
        int infoY = boardHeight / 2 - 30;
        int infoWidth = 200;
        int infoHeight = 60;

        painter.setPen(QPen(QColor(239, 68, 68, 200), 3));
        painter.setBrush(QBrush(QColor(239, 68, 68, 180)));
        painter.drawRect(infoX, infoY, infoWidth, infoHeight);

        painter.setPen(Qt::white);
        painter.setFont(QFont("Microsoft YaHei", 16, QFont::Bold));
        painter.drawText(QRect(infoX, infoY, infoWidth, infoHeight), Qt::AlignCenter, winnerText + " 获胜！");
    }
}

void RoomWidget::updateChessBoardDisplay()
{
    QWidget *chessBoardWidget = ui->chessBoardWidget;
    if (chessBoardWidget)
    {
        chessBoardWidget->update(); // 触发重绘
    }
}

QPoint RoomWidget::screenPosToGrid(const QPoint &pos, QWidget *boardWidget)
{
    if (!game)
        return QPoint(-1, -1);

    const std::vector<std::vector<Piece>> &board = game->getBoard();
    int boardSize = board.size();
    if (boardSize == 0)
        return QPoint(-1, -1);

    const int gridSize = 40;
    const int boardLength = gridSize * (boardSize - 1);
    int boardWidth = boardWidget->width();
    int boardHeight = boardWidget->height();
    QPoint boardTopLeft((boardWidth - boardLength) / 2, (boardHeight - boardLength) / 2);

    // 检查是否在棋盘范围内
    if (pos.x() < boardTopLeft.x() || pos.y() < boardTopLeft.y())
    {
        return QPoint(-1, -1);
    }

    int gridX = (pos.x() - boardTopLeft.x() + gridSize / 2) / gridSize;
    int gridY = (pos.y() - boardTopLeft.y() + gridSize / 2) / gridSize;

    // 确保在棋盘范围内
    if (gridX < 0 || gridX >= boardSize || gridY < 0 || gridY >= boardSize)
    {
        return QPoint(-1, -1);
    }

    return QPoint(gridX, gridY);
}

// 新协议槽函数实现

void RoomWidget::onSyncSeat(const QString &player1, const QString &player2)
{
    qDebug() << "RoomWidget::onSyncSeat: Received seat sync - Player1:" << player1 << "Player2:" << player2;

    // 更新黑棋玩家
    if (!player1.isEmpty() && player1 != "null")
    {
        onBlackTaken(player1, false);
    }
    else
    {
        // 黑棋座位为空
        blackPlayer = nullptr;
        SwitchPlayerInfoPanal(true, false);
        ui->player1NameLabel->setText("等待玩家...");
    }

    // 更新白棋玩家
    if (!player2.isEmpty() && player2 != "null")
    {
        onWhiteTaken(player2, false);
    }
    else
    {
        // 白棋座位为空
        whitePlayer = nullptr;
        SwitchPlayerInfoPanal(false, false);
        ui->player2NameLabel->setText("等待玩家...");
    }

    emit logToUser("座位信息已同步");
}

void RoomWidget::onSyncRoomSetting(const QString &settings)
{
    qDebug() << "RoomWidget::onSyncRoomSetting: Received room settings:" << settings;
    // 解析设置字符串并更新UI
    // 这里可以根据实际协议格式进行解析
    QStringList settingList = settings.split(';', Qt::SkipEmptyParts);
    onUpdateRoomSetting(settingList);
    emit logToUser("房间设置已同步");
}

void RoomWidget::onSyncGame(const QString &statusStr)
{
}
