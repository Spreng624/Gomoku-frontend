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
                                          networkStatusButton(nullptr),
                                          userInfoButton(nullptr),
                                          stackedWidget(nullptr),
                                          lobby(nullptr),
                                          room(nullptr),
                                          currentUsername(""),
                                          currentRating(1500),
                                          maximized(false),
                                          toastWidget(nullptr)
{
    Logger::init("gomoku.log", LogLevel::DEBUG, true);

    LOG_DEBUG("================ Initializing MainWindow ================");
    LOG_DEBUG("Initializing Controller...");
    ctrl = std::make_unique<Controller>();
    LOG_DEBUG("Initializing LobbyWidget...");
    lobby = new LobbyWidget(this);
    LOG_DEBUG("Initializing RoomWidget...");
    room = new RoomWidget(this);
    LOG_DEBUG("Setting up UI...");
    ui->setupUi(this);
    LOG_DEBUG("Initializing Window Components...");
    initStyle();
    initLayout();
    LOG_DEBUG("Initializing Title Bar & Status Bar...");
    initTitleBar();
    initStatusBar();
    LOG_DEBUG("Setting up signal connections...");
    SetUpSignals();
    LOG_DEBUG("Initializing ToastWidget...");
    toastWidget = new ToastWidget();

    stackedWidget = ui->stackedWidget;
    if (!stackedWidget)
    {
        LOG_ERROR("stackedWidget not found in UI! Creating new one...");
        stackedWidget = new QStackedWidget(this);
    }
    else
    {
        while (stackedWidget->count() > 0)
        {
            QWidget *widget = stackedWidget->widget(0);
            stackedWidget->removeWidget(widget);
        }
    }
    stackedWidget->addWidget(lobby); // Index 0: Lobby
    stackedWidget->addWidget(room);  // Index 1: Game
    stackedWidget->setCurrentIndex(0);

    ctrl->onConnectToServer();
    setStatusMessage("准备就绪");
    QTimer::singleShot(1000, this, [this]()
                       { showToastMessage("欢迎来到五子棋游戏！"); });
    LOG_DEBUG("=========================================================");
}

MainWindow::~MainWindow()
{
    LOG_INFO("MainWindow destructor called");

    LOG_DEBUG("Cleaning up UI...");
    delete ui;

    LOG_DEBUG("Cleaning up Manager...");
    ctrl = nullptr;

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
    // MainWindow <--> Controller
    connect(this, &MainWindow::login, ctrl.get(), &Controller::onLogin);
    connect(this, &MainWindow::signin, ctrl.get(), &Controller::onSignin);
    connect(this, &MainWindow::loginAsGuest, ctrl.get(), &Controller::onLoginAsGuest);
    connect(this, &MainWindow::logout, ctrl.get(), &Controller::onLogout);
    connect(networkStatusButton, &QPushButton::clicked, ctrl.get(), &Controller::onConnectToServer);
    connect(ctrl.get(), &Controller::connectionStatusChanged, this, &MainWindow::setNetworkStatus);
    connect(ctrl.get(), &Controller::statusBarMessageChanged, this, &MainWindow::setStatusMessage);
    connect(ctrl.get(), &Controller::userIdentityChanged, this, &MainWindow::setUserInfo);

    // LobbyWidget <--> Controller
    connect(lobby, &LobbyWidget::createRoom, ctrl.get(), &Controller::onCreateRoom);
    connect(lobby, &LobbyWidget::joinRoom, ctrl.get(), &Controller::onJoinRoom);
    connect(lobby, &LobbyWidget::quickMatch, ctrl.get(), &Controller::onQuickMatch);
    connect(lobby, &LobbyWidget::freshPlayerList, ctrl.get(), &Controller::onUpdateLobbyPlayerList);
    connect(lobby, &LobbyWidget::freshRoomList, ctrl.get(), &Controller::onUpdateLobbyRoomList);
    connect(ctrl.get(), &Controller::updateLobbyPlayerList, lobby, &LobbyWidget::updatePlayerList);
    connect(ctrl.get(), &Controller::updateLobbyRoomList, lobby, &LobbyWidget::updateRoomList);

    // GameWidget <--> Controller
    connect(room, &RoomWidget::SyncSeat, ctrl.get(), &Controller::onSyncSeat);
    connect(room, &RoomWidget::syncRoomSetting, ctrl.get(), &Controller::onSyncRoomSetting);
    connect(room, &RoomWidget::chatMessage, ctrl.get(), &Controller::onChatMessage);
    connect(room, &RoomWidget::syncUsersToRoom, ctrl.get(), &Controller::onSyncUsersToRoom);
    connect(room, &RoomWidget::backToLobby, ctrl.get(), &Controller::onExitRoom);

    connect(room, &RoomWidget::gameStart, ctrl.get(), &Controller::onGameStarted);
    connect(room, &RoomWidget::makeMove, ctrl.get(), &Controller::onMakeMove);
    connect(room, &RoomWidget::giveup, ctrl.get(), &Controller::onGiveUp);
    connect(room, &RoomWidget::draw, ctrl.get(), &Controller::onDraw);
    connect(room, &RoomWidget::undoMove, ctrl.get(), &Controller::onUndoMove);
    connect(room, &RoomWidget::SyncGame, ctrl.get(), &Controller::onSyncGame);

    connect(ctrl.get(), &Controller::syncSeat, room, &RoomWidget::onSyncSeat);
    connect(ctrl.get(), &Controller::syncRoomSetting, room, &RoomWidget::onSyncRoomSetting);
    connect(ctrl.get(), &Controller::chatMessage, room, &RoomWidget::onChatMessage);
    connect(ctrl.get(), &Controller::SyncUsersToRoom, room, &RoomWidget::onSyncUsersToRoom);

    connect(ctrl.get(), &Controller::initRomeWidget, room, &RoomWidget::reset);
    connect(ctrl.get(), &Controller::gameStarted, room, &RoomWidget::onGameStarted);
    connect(ctrl.get(), &Controller::gameEnded, room, &RoomWidget::onGameEnded);
    connect(ctrl.get(), &Controller::makeMove, room, &RoomWidget::onMakeMove);

    connect(ctrl.get(), &Controller::syncGame, room, &RoomWidget::onSyncGame);
    connect(ctrl.get(), &Controller::draw, room, &RoomWidget::onDraw);
    connect(ctrl.get(), &Controller::undoMove, room, &RoomWidget::onUndoMove);

    // Else signals-slots
    connect(room, &RoomWidget::backToLobby, this, [this]()
            { emit onSwitchWidget(0); });
    connect(lobby, &LobbyWidget::localGame, this, [this]()
            { room->reset(true); emit onSwitchWidget(1); });
    connect(ctrl.get(), &Controller::switchWidget, this, &MainWindow::onSwitchWidget);
    connect(ctrl.get(), &Controller::logToUser, this, [this](const QString &message)
            { showToastMessage(message, 3000); });
    connect(room, &RoomWidget::logToUser, this, [this](const QString &message)
            { showToastMessage(message, 3000); });
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
    setAttribute(Qt::WA_StyledBackground, true);

    QString mainWindowStyle = "QMainWindow {"
                              "    background-color: white;"
                              "    border: none;"
                              "}";
    setStyleSheet(mainWindowStyle);
    Qt::WindowFlags flags = Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowMinimizeButtonHint;
    setWindowFlags(flags);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setWindowFlags(windowFlags() | Qt::Window);

    this->resize(1000, 700);

    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(255, 255, 255)); // 白色
    setPalette(pal);
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
    if (!networkStatusButton)
    {
        LOG_ERROR("Network status button not found!");
        return;
    }
    if (connected)
    {
        networkStatusButton->setText("● 在线");
        networkStatusButton->setStyleSheet("color: #1a7f37;"); // 绿色
    }
    else
    {
        networkStatusButton->setText("● 离线");
        networkStatusButton->setStyleSheet("color: #cf222e;"); // 红色
    }
}

void MainWindow::setUserInfo(const QString &username, int rating)
{
    room->username = username;
    currentUsername = username;
    currentRating = rating;
    if (userInfoButton)
    {
        if (username.isEmpty())
        {
            userInfoButton->setText(QString("未登录").arg(rating));
        }
        else
        {
            userInfoButton->setText(QString("%1 | 等级分: %2").arg(username).arg(rating));
        }
    }
}

void MainWindow::initLayout()
{
    QWidget *centralWidget = ui->centralwidget;
    if (!centralWidget)
    {
        LOG_WARN("centralwidget not found in UI! ui pointer: " + std::to_string((long long)ui));
        LOG_WARN("This means ui->setupUi(this) may have failed or UI file not compiled properly");
        return;
    }
    centralWidget->setObjectName("centralContainer");
    centralWidget->setAttribute(Qt::WA_StyledBackground, true);
    QPalette pal = centralWidget->palette();
    centralWidget->setAutoFillBackground(true);
    pal.setColor(QPalette::Window, QColor(255, 255, 255)); // 白色
    centralWidget->setPalette(pal);
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

    titleLabel = ui->titleLabel;
    if (!titleLabel)
    {
        LOG_WARN("titleLabel not found!");
    }

    minimizeButton = ui->minimizeButton;
    if (!minimizeButton)
    {
        LOG_WARN("minimizeButton not found!");
    }

    maximizeButton = ui->maximizeButton;
    if (!maximizeButton)
    {
        LOG_WARN("maximizeButton not found!");
    }
    else
    {
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

    statusMessageLabel = ui->statusMessageLabel;
    if (!statusMessageLabel)
    {
        LOG_WARN("statusMessageLabel not found!");
    }
    else
    {
        statusMessageLabel->setText("准备就绪");
    }

    networkStatusButton = ui->networkStatusButton;
    if (!networkStatusButton)
    {
        LOG_WARN("networkStatusButton not found!");
    }
    else
    {
        networkStatusButton->setText("● 离线");
        networkStatusButton->setStyleSheet("color: #cf222e;"); // 红色
        // 连接点击信号
    }

    userInfoButton = ui->userInfoButton;
    if (!userInfoButton)
    {
        LOG_WARN("userInfoButton not found!");
    }
    else
    {
        setUserInfo("", 0);
        userInfoButton->setText("未登录");
        connect(userInfoButton, &QPushButton::clicked, this, &MainWindow::onUserInfoButtonClicked);
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

void MainWindow::onUserInfoButtonClicked()
{
    LOG_DEBUG("User info button clicked, showing login dialog");

    // 如果用户已经登录，显示登出选项
    if (!currentUsername.isEmpty())
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle("用户操作");
        msgBox.setText(QString("当前用户: %1\n等级分: %2").arg(currentUsername).arg(currentRating));
        msgBox.setInformativeText("请选择操作:");

        QPushButton *logoutButton = msgBox.addButton("登出", QMessageBox::ActionRole);
        QPushButton *cancelButton = msgBox.addButton("取消", QMessageBox::RejectRole);

        msgBox.exec();

        if (msgBox.clickedButton() == logoutButton)
        {
            LOG_DEBUG("User chose to logout");
            emit logout();
        }
        setUserInfo("", 0);
    }
    else
    {
        // 用户未登录，显示登录选项对话框
        QMessageBox msgBox;
        msgBox.setWindowTitle("用户登录");
        msgBox.setText("请选择登录方式:");

        QPushButton *loginButton = msgBox.addButton("账号登录", QMessageBox::ActionRole);
        QPushButton *guestButton = msgBox.addButton("游客登录", QMessageBox::ActionRole);
        QPushButton *signinButton = msgBox.addButton("注册账号", QMessageBox::ActionRole);
        QPushButton *cancelButton = msgBox.addButton("取消", QMessageBox::RejectRole);

        msgBox.exec();

        if (msgBox.clickedButton() == loginButton)
        {
            LOG_DEBUG("User chose to login with account");
            bool ok;
            QString username = QInputDialog::getText(this, "账号登录", "用户名:", QLineEdit::Normal, "", &ok);
            if (ok && !username.isEmpty())
            {
                QString password = QInputDialog::getText(this, "账号登录", "密码:", QLineEdit::Password, "", &ok);
                if (ok && !password.isEmpty())
                {
                    emit login(username.toStdString(), password.toStdString());
                }
            }
        }
        else if (msgBox.clickedButton() == guestButton)
        {
            LOG_DEBUG("User chose to login as guest");
            emit loginAsGuest();
        }
        else if (msgBox.clickedButton() == signinButton)
        {
            LOG_DEBUG("User chose to sign up");
            bool ok;
            QString username = QInputDialog::getText(this, "注册账号", "用户名:", QLineEdit::Normal, "", &ok);
            if (ok && !username.isEmpty())
            {
                QString password = QInputDialog::getText(this, "注册账号", "密码:", QLineEdit::Password, "", &ok);
                if (ok && !password.isEmpty())
                {
                    emit signin(username.toStdString(), password.toStdString());
                }
            }
        }
    }
}
