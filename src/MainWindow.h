#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <memory>
#include "LobbyWidget.h"
#include "GameWidget.h"
#include "Manager.h"
#include "ToastWidget.h"
#include "GameManager.h"

namespace Ui
{
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    // 用户认证相关信号
    void login(const std::string &username, const std::string &password);
    void signin(const std::string &username, const std::string &password);
    void loginAsGuest();
    void logout();

private slots:
    void onSwitchWidget(int index);
    void onUserInfoButtonClicked();

private:
    void initStyle();
    void initTitleBar();
    void initStatusBar();
    void initLayout();
    void updateWindowTitle(const QString &title);

    void setStatusMessage(const QString &message);
    void setNetworkStatus(bool connected);
    void setUserInfo(const QString &username, int rating);
    void SetUpSignals();

    // 提示消息方法
    void showToastMessage(const QString &message, int duration = 3000);

    // 背景验证方法
    void validateWindowBackground();

    // UI成员
    Ui::MainWindow *ui;

    // 自定义标题栏组件
    QWidget *titleBarWidget;
    QLabel *titleLabel;
    QPushButton *minimizeButton;
    QPushButton *maximizeButton;
    QPushButton *closeButton;

    // 自定义状态栏组件
    QWidget *statusBarWidget;
    QLabel *statusMessageLabel;
    QPushButton *networkStatusButton;
    QPushButton *userInfoButton;

    // 原有成员变量
    QStackedWidget *stackedWidget;
    LobbyWidget *lobby;
    GameWidget *game;
    std::unique_ptr<Manager> manager;
    std::unique_ptr<GameManager> gameManager;
    bool networkConnected;
    QString currentUsername;
    int currentRating;

    // 提示消息组件
    ToastWidget *toastWidget;

    // 鼠标拖动相关
    QPoint dragPosition;
    bool maximized;

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
};

#endif // MAINWINDOW_H
