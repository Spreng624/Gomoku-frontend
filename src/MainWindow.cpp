#include "MainWindow.h"
#include "ui_mainwindow.h"
#include <QVBoxLayout>
#include <QStatusBar>
#include <QMessageBox>
#include <QInputDialog>
#include <QDebug>
#include <QMouseEvent>
#include <QApplication>
#include <QStyle>
#include <QLinearGradient>
#include <QPainter>
#include <QTimer>
#include "Logger.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),
                                          ui(new Ui::MainWindow),
                                          titleBarWidget(nullptr),
                                          titleLabel(nullptr),
                                          minimizeButton(nullptr),
                                          maximizeButton(nullptr),
                                          closeButton(nullptr),
                                          statusBarWidget(nullptr),
                                          statusMessageLabel(nullptr),
                                          networkStatusLabel(nullptr),
                                          userInfoLabel(nullptr),
                                          stackedWidget(nullptr),
                                          lobby(nullptr),
                                          game(nullptr),
                                          networkConnected(false),
                                          currentUsername(""),
                                          currentRating(1500),
                                          maximized(false),
                                          toastWidget(nullptr)
{
    LOG_INFO("MainWindow constructor called");
    Logger::init("logs/gomoku.log", LogLevel::DEBUG, true);
    LOG_DEBUG("Logger initialized with DEBUG level");

    // 设置UI
    LOG_DEBUG("Setting up UI...");
    ui->setupUi(this);

    // 初始化 Manager 与 Widget
    LOG_DEBUG("Creating Manager instance...");
    manager = std::make_unique<Manager>();

    LOG_DEBUG("Creating LobbyWidget...");
    lobby = new LobbyWidget(this);

    LOG_DEBUG("Creating GameWidget...");
    game = new GameWidget(this);

    // 创建 GameManager
    LOG_DEBUG("Creating GameManager instance...");
    gameManager = std::make_unique<GameManager>();

    // 连接信号与槽
    LOG_DEBUG("Setting up signal connections...");
    SetUpSignals();

    // 初始化样式
    LOG_DEBUG("Initializing window components...");
    initStyle();
    initLayout();

    // 初始化标题栏和状态栏组件
    LOG_DEBUG("Initializing title bar and status bar...");
    initTitleBar();
    initStatusBar();

    // 从UI中获取stackedWidget并添加widgets
    LOG_DEBUG("Getting stackedWidget from UI...");
    stackedWidget = ui->stackedWidget;
    if (!stackedWidget)
    {
        LOG_WARN("stackedWidget not found in UI! Creating new one...");
        stackedWidget = new QStackedWidget(this);
    }
    else
    {
        LOG_DEBUG("stackedWidget found in UI");
        // 移除UI中默认的页面，添加我们的widgets
        while (stackedWidget->count() > 0)
        {
            QWidget *widget = stackedWidget->widget(0);
            stackedWidget->removeWidget(widget);
        }
    }

    // 添加 Widget 到 StackedWidget中
    stackedWidget->addWidget(lobby); // Index 0: Lobby
    stackedWidget->addWidget(game);  // Index 1: Game
    stackedWidget->setCurrentIndex(0);
    LOG_DEBUG("StackedWidget initialized with Lobby as default");

    // 设置初始状态消息（必须在initStatusBar之后调用）
    setStatusMessage("准备就绪");

    // 创建提示消息组件
    LOG_DEBUG("Creating ToastWidget...");
    toastWidget = new ToastWidget();

    // 显示欢迎消息
    QTimer::singleShot(1000, this, [this]()
                       { showToastMessage("欢迎来到五子棋游戏！"); });

    LOG_INFO("MainWindow initialization completed successfully");
}

MainWindow::~MainWindow()
{
    LOG_INFO("MainWindow destructor called");

    LOG_DEBUG("Cleaning up UI...");
    delete ui;

    LOG_DEBUG("Cleaning up Manager...");
    manager = nullptr;

    LOG_DEBUG("Cleaning up ToastWidget...");
    if (toastWidget)
    {
        delete toastWidget;
        toastWidget = nullptr;
    }

    LOG_DEBUG("Shutting down Logger...");
    Logger::shutdown();

    LOG_INFO("MainWindow cleanup completed");
}

void MainWindow::SetUpSignals()
{
    // Lobby -> Manager
    LOG_DEBUG("Connecting LobbyWidget::localGame to GameManager::setLocalGame...");
    bool localGameConnected = connect(lobby, &LobbyWidget::localGame, this, [this]()
                                      {
        LOG_DEBUG("localGame signal received, initializing local game");
        if (gameManager && game) {
            // 设置本地游戏标志
            gameManager->setLocalGame(true);
            // 初始化游戏部件为本地模式
            game->initGameWidget(true);
            // 切换到游戏界面
            onSwitchWidget(1);
            // 设置棋盘部件的Game实例
            if (gameManager->getLocalGame() && game->getChessBoard()) {
                game->getChessBoard()->setGame(gameManager->getLocalGame());
            }
        } else {
            LOG_WARN("gameManager or game is null when trying to set local game");
        } });
    LOG_DEBUG_FMT("localGame connection result: %d", localGameConnected ? 1 : 0);
    connect(lobby, &LobbyWidget::createRoom, manager.get(), &Manager::createRoom);
    connect(lobby, &LobbyWidget::joinRoom, manager.get(), &Manager::joinRoom);
    connect(lobby, &LobbyWidget::quickMatch, manager.get(), &Manager::quickMatch);
    connect(lobby, &LobbyWidget::freshPlayerList, manager.get(), &Manager::freshLobbyPlayerList);
    connect(lobby, &LobbyWidget::freshRoomList, manager.get(), &Manager::freshLobbyRoomList);

    // GameWidget -> GameManager
    connect(game, &GameWidget::backToLobby, gameManager.get(), &GameManager::onBackToLobby);
    connect(game, &GameWidget::takeBlack, gameManager.get(), &GameManager::onTakeBlack);
    connect(game, &GameWidget::takeWhite, gameManager.get(), &GameManager::onTakeWhite);
    connect(game, &GameWidget::cancelTake, gameManager.get(), &GameManager::onCancelTake);
    connect(game, &GameWidget::startGame, gameManager.get(), &GameManager::onStartGame);
    connect(game, &GameWidget::editRoomSetting, gameManager.get(), &GameManager::onEditRoomSetting);
    connect(game, &GameWidget::chatMessageSent, gameManager.get(), &GameManager::onChatMessageSent);
    connect(game, &GameWidget::makeMove, gameManager.get(), &GameManager::onMakeMove);
    connect(game, &GameWidget::undoMoveRequested, gameManager.get(), &GameManager::onUndoMoveRequested);
    connect(game, &GameWidget::undoMoveResponse, gameManager.get(), &GameManager::onUndoMoveResponse);
    connect(game, &GameWidget::drawRequested, gameManager.get(), &GameManager::onDrawRequested);
    connect(game, &GameWidget::drawResponse, gameManager.get(), &GameManager::onDrawResponse);
    connect(game, &GameWidget::giveup, gameManager.get(), &GameManager::onGiveup);

    // GameManager -> Manager
    connect(gameManager.get(), &GameManager::backToLobby, manager.get(), &Manager::exitRoom);
    connect(gameManager.get(), &GameManager::takeBlack, manager.get(), &Manager::takeBlack);
    connect(gameManager.get(), &GameManager::takeWhite, manager.get(), &Manager::takeWhite);
    connect(gameManager.get(), &GameManager::cancelTake, manager.get(), &Manager::cancelTake);
    connect(gameManager.get(), &GameManager::startGame, manager.get(), &Manager::startGame);
    connect(gameManager.get(), &GameManager::editRoomSetting, manager.get(), &Manager::editRoomSetting);
    connect(gameManager.get(), &GameManager::chatMessageSent, manager.get(), &Manager::chatMessageSent);
    connect(gameManager.get(), &GameManager::makeMove, manager.get(), &Manager::makeMove);
    connect(gameManager.get(), &GameManager::undoMoveRequest, manager.get(), &Manager::undoMoveRequest);
    connect(gameManager.get(), &GameManager::undoMoveResponse, manager.get(), &Manager::undoMoveResponse);
    connect(gameManager.get(), &GameManager::drawRequest, manager.get(), &Manager::drawRequest);
    connect(gameManager.get(), &GameManager::drawResponse, manager.get(), &Manager::drawResponse);
    connect(gameManager.get(), &GameManager::giveUp, manager.get(), &Manager::giveUp);

    // MainWindow -> Manager
    connect(this, &MainWindow::login, manager.get(), &Manager::login);
    connect(this, &MainWindow::signin, manager.get(), &Manager::signin);
    connect(this, &MainWindow::loginAsGuest, manager.get(), &Manager::loginAsGuest);
    connect(this, &MainWindow::logout, manager.get(), &Manager::logout);

    // Manager -> MainWindow
    connect(manager.get(), &Manager::switchWidget, this, &MainWindow::onSwitchWidget);
    connect(manager.get(), &Manager::connectionStatusChanged, this, &MainWindow::setNetworkStatus);
    connect(manager.get(), &Manager::userIdentityChanged, this, &MainWindow::setUserInfo);
    connect(manager.get(), &Manager::statusBarMessageChanged, this, &MainWindow::setStatusMessage);
    connect(manager.get(), &Manager::logToUser, this, [this](const QString &message)
            { showToastMessage(message, 3000); });

    // Manager -> Lobby
    connect(manager.get(), &Manager::updateLobbyPlayerList, lobby, &LobbyWidget::updatePlayerList);
    connect(manager.get(), &Manager::updateLobbyRoomList, lobby, &LobbyWidget::updateRoomList);

    // Manager -> GameManager (新增的连接)
    connect(manager.get(), &Manager::chatMessageReceived,
            gameManager.get(), &GameManager::onChatMessageReceived);
    connect(manager.get(), &Manager::gameStarted,
            gameManager.get(), &GameManager::onGameStarted);
    connect(manager.get(), &Manager::gameEnded,
            gameManager.get(), &GameManager::onGameEnded);
    connect(manager.get(), &Manager::playerListUpdated,
            gameManager.get(), &GameManager::onPlayerListUpdated);
    connect(manager.get(), &Manager::updateRoomPlayerList,
            gameManager.get(), &GameManager::onUpdateRoomPlayerList);

    // Manager -> GameWidget (保留必要的直接连接)
    connect(manager.get(), &Manager::initGameWidget, this, [this](bool isLocal)
            {
        game->initGameWidget(isLocal);
        if (gameManager) {
            gameManager->setLocalGame(isLocal);
            // 如果是本地游戏，设置棋盘部件的Game实例
            if (isLocal && gameManager->getLocalGame()) {
                // 获取GameWidget的棋盘部件并设置Game实例
                if (game && game->getChessBoard()) {
                    game->getChessBoard()->setGame(gameManager->getLocalGame());
                }
            }
        } });

    // GameManager -> GameWidget
    connect(gameManager.get(), &GameManager::chatMessageReceived, game, &GameWidget::onChatMessageReceived);
    connect(gameManager.get(), &GameManager::gameStarted, game, &GameWidget::onGameStarted);
    connect(gameManager.get(), &GameManager::gameEnded, game, &GameWidget::onGameEnded);
    connect(gameManager.get(), &GameManager::playerListUpdated, game, &GameWidget::onPlayerListUpdated);
    connect(gameManager.get(), &GameManager::updateRoomPlayerList, game, &GameWidget::updateRoomPlayerList);

    // GameManager 本地游戏返回大厅
    connect(gameManager.get(), &GameManager::localGameBackToLobby, this, [this]()
            {
        qDebug() << "Local game back to lobby signal received";
        onSwitchWidget(0); // 切换回大厅
        // 重置游戏状态
        if (game) {
            game->initGameWidget(false);
        }
        // 重置GameManager的本地游戏状态
        if (gameManager) {
            gameManager->setLocalGame(false);
        } });
}

void MainWindow::onSwitchWidget(int index)
{
    LOG_DEBUG("Switching widget to index: " + std::to_string(index));
    stackedWidget->setCurrentIndex(index);

    switch (index)
    {
    case 0:
        LOG_INFO("Switching to Lobby widget");
        updateWindowTitle("五子棋大厅");
        statusBar()->showMessage("已返回大厅");
        break;
    case 1:
        LOG_INFO("Switching to Game widget");
        updateWindowTitle("五子棋对战");
        statusBar()->showMessage("已进入房间");
        break;
    default:
        LOG_WARN("Unknown widget index: " + std::to_string(index));
        break;
    }

    LOG_DEBUG("Widget switch completed");
}

void MainWindow::initStyle()
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowMinimizeButtonHint);

    // 注释掉透明背景，因为透明背景会导致样式表背景色不显示
    // setAttribute(Qt::WA_TranslucentBackground);

    // 设置不透明背景
    setAttribute(Qt::WA_OpaquePaintEvent);

    // 允许任务栏点击最小化
    setWindowFlags(windowFlags() | Qt::Window);

    this->resize(1000, 700);

    // 设置窗口背景色为白色，确保有背景
    setStyleSheet("QMainWindow { background-color: white; }");
}

void MainWindow::setStatusMessage(const QString &message)
{
    if (statusMessageLabel)
    {
        statusMessageLabel->setText(message);
    }
}

void MainWindow::setNetworkStatus(bool connected)
{
    if (networkStatusLabel)
    {
        if (connected)
        {
            networkStatusLabel->setText("● 在线");
            networkStatusLabel->setStyleSheet("color: #1a7f37;"); // 绿色
        }
        else
        {
            networkStatusLabel->setText("● 离线");
            networkStatusLabel->setStyleSheet("color: #cf222e;"); // 红色
        }
    }
    networkConnected = connected;
}

void MainWindow::setUserInfo(const QString &username, int rating)
{
    currentUsername = username;
    currentRating = rating;
    if (userInfoLabel)
    {
        if (username.isEmpty())
        {
            userInfoLabel->setText(QString("游客 | 等级分: %1").arg(rating));
        }
        else
        {
            userInfoLabel->setText(QString("%1 | 等级分: %2").arg(username).arg(rating));
        }
    }
}

void MainWindow::initLayout()
{
    LOG_DEBUG("Initializing layout...");

    // UI已经通过setupUi设置了centralwidget，其中包含titleBarWidget、stackedWidget和statusBarWidget
    // 我们只需要确保centralwidget有正确的对象名和属性

    // 1. 获取centralwidget
    QWidget *centralWidget = ui->centralwidget;
    if (!centralWidget)
    {
        LOG_WARN("centralwidget not found in UI! ui pointer: " + std::to_string((long long)ui));
        LOG_WARN("This means ui->setupUi(this) may have failed or UI file not compiled properly");
        return;
    }
    else
    {
        LOG_DEBUG("centralwidget found at address: " + std::to_string((long long)centralWidget));
        LOG_DEBUG("centralwidget object name: " + centralWidget->objectName().toStdString());
    }

    // 2. 设置centralwidget的对象名，以便QSS选择器可以匹配
    centralWidget->setObjectName("centralContainer");
    LOG_DEBUG("centralwidget object name set to: centralContainer");

    // 【核心修复 1】：必须设置此属性，否则在透明窗口下 QSS 背景色不显示
    centralWidget->setAttribute(Qt::WA_StyledBackground, true);
    LOG_DEBUG("centralwidget WA_StyledBackground attribute set");

    // 注意：不要再次调用setCentralWidget，因为ui->setupUi已经设置了
    // setCentralWidget(centralWidget);

    LOG_DEBUG("Layout initialized successfully");
}

void MainWindow::initTitleBar()
{
    LOG_DEBUG("Initializing title bar components...");

    // 通过UI指针获取标题栏组件
    titleBarWidget = ui->titleBarWidget;
    if (!titleBarWidget)
    {
        LOG_WARN("titleBarWidget not found! ui pointer: " + std::to_string((long long)ui));
    }
    else
    {
        LOG_DEBUG("titleBarWidget found at address: " + std::to_string((long long)titleBarWidget));
        LOG_DEBUG("titleBarWidget object name: " + titleBarWidget->objectName().toStdString());
    }

    titleLabel = ui->titleLabel;
    if (!titleLabel)
    {
        LOG_WARN("titleLabel not found!");
    }
    else
    {
        LOG_DEBUG("titleLabel found at address: " + std::to_string((long long)titleLabel));
        LOG_DEBUG("titleLabel text: " + titleLabel->text().toStdString());
    }

    minimizeButton = ui->minimizeButton;
    if (!minimizeButton)
    {
        LOG_WARN("minimizeButton not found!");
    }
    else
    {
        LOG_DEBUG("minimizeButton found at address: " + std::to_string((long long)minimizeButton));
        // 最小化按钮信号已在UI文件中连接
    }

    maximizeButton = ui->maximizeButton;
    if (!maximizeButton)
    {
        LOG_WARN("maximizeButton not found!");
    }
    else
    {
        LOG_DEBUG("maximizeButton found at address: " + std::to_string((long long)maximizeButton));
        // 连接最大化按钮信号
        connect(maximizeButton, &QPushButton::clicked, this, [this]()
                {
            if (maximized) {
                showNormal();
                maximizeButton->setText("□");
                maximized = false;
            } else {
                showMaximized();
                maximizeButton->setText("❐");
                maximized = true;
            } });
    }

    closeButton = ui->closeButton;
    if (!closeButton)
    {
        LOG_WARN("closeButton not found!");
    }
    else
    {
        LOG_DEBUG("closeButton found at address: " + std::to_string((long long)closeButton));
        // 关闭按钮信号已在UI文件中连接
    }

    LOG_DEBUG("Title bar initialization completed");
}

void MainWindow::initStatusBar()
{
    LOG_DEBUG("Initializing status bar components...");

    // 通过UI指针获取状态栏组件
    statusBarWidget = ui->statusBarWidget;
    if (!statusBarWidget)
    {
        LOG_WARN("statusBarWidget not found!");
    }
    else
    {
        LOG_DEBUG("statusBarWidget found");
    }

    statusMessageLabel = ui->statusMessageLabel;
    if (!statusMessageLabel)
    {
        LOG_WARN("statusMessageLabel not found!");
    }
    else
    {
        LOG_DEBUG("statusMessageLabel found");
        // 设置初始状态消息
        statusMessageLabel->setText("准备就绪");
    }

    networkStatusLabel = ui->networkStatusLabel;
    if (!networkStatusLabel)
    {
        LOG_WARN("networkStatusLabel not found!");
    }
    else
    {
        LOG_DEBUG("networkStatusLabel found");
        // 设置初始网络状态为离线
        networkStatusLabel->setText("● 离线");
        networkStatusLabel->setStyleSheet("color: #cf222e;"); // 红色
    }

    userInfoLabel = ui->userInfoLabel;
    if (!userInfoLabel)
    {
        LOG_WARN("userInfoLabel not found!");
    }
    else
    {
        LOG_DEBUG("userInfoLabel found");
        // 设置初始用户信息
        userInfoLabel->setText("游客 | 等级分: 1500");
    }

    LOG_DEBUG("Status bar initialization completed");
}

void MainWindow::updateWindowTitle(const QString &title)
{
    if (titleLabel)
    {
        titleLabel->setText(title);
    }
    QMainWindow::setWindowTitle(title);
}

void MainWindow::showToastMessage(const QString &message, int duration)
{
    if (toastWidget)
    {
        toastWidget->showMessage(message, duration);
    }
    else
    {
        LOG_WARN("ToastWidget is not initialized, cannot show message: " + message.toStdString());
    }
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        // 只要点击位置在标题栏高度（40px）范围内，且不是点击在按钮上
        if (titleBarWidget && event->position().y() < titleBarWidget->height())
        {
            dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
            event->accept();
            return;
        }
    }
    QMainWindow::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton)
    {
        if (!dragPosition.isNull())
        {
            move(event->globalPosition().toPoint() - dragPosition);
            event->accept();
        }
    }
    QMainWindow::mouseMoveEvent(event);
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (titleBarWidget && titleBarWidget->geometry().contains(event->pos()))
    {
        if (maximized)
        {
            showNormal();
            if (maximizeButton)
                maximizeButton->setText("□");
            maximized = false;
        }
        else
        {
            showMaximized();
            if (maximizeButton)
                maximizeButton->setText("❐");
            maximized = true;
        }
        event->accept();
        return;
    }
    QMainWindow::mouseDoubleClickEvent(event);
}
