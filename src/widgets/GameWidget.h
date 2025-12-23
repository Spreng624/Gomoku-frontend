#ifndef GAMEWIDGET_H
#define GAMEWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QList>
#include <QPair>
#include <QString>
#include "Game.h"
#include "ChessBoardWidget.h" // 包含新的棋盘部件

class Manager; // 前向声明

namespace Ui
{
    class GameWidget;
}

class GameWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GameWidget(QWidget *parent = nullptr);
    ~GameWidget();

signals:
    // 用户操作信号（发送给GameManager）
    void backToLobby();
    void takeBlack();
    void takeWhite();
    void cancelTake();
    void startGame();
    void editRoomSetting();
    void chatMessageSent(const QString &message);
    void makeMove(int x, int y);
    void undoMoveRequested();
    void undoMoveResponse(bool accepted);
    void drawRequested();
    void drawResponse(bool accept);
    void giveup();

public slots:
    // 从Manager接收的槽
    void initGameWidget(bool islocal);
    void updateRoomPlayerList(const QStringList &players);
    void playerListUpdated(const QStringList &players);
    void chatMessageReceived(const QString &username, const QString &message);
    void gameStarted();
    void gameEnded();

    // 从GameManager接收的槽
    void onChatMessageReceived(const QString &username, const QString &message);
    void onGameStarted(const QString &username, int rating);
    void onGameEnded(const QString &username, int rating, bool won);
    void onPlayerListUpdated(const QStringList &players);

private slots:
    // 内部槽函数，用于处理UI事件
    void onBackToLobbyButtonClicked();
    void onSurrenderButtonClicked();
    void onUndoButtonClicked();
    void onSendButtonClicked();
    void onMinimizeBtnClicked();
    void onCloseBtnClicked();

private:
    // 初始化函数
    void initUI();
    void setupConnections();
    // 更新UI的辅助方法（现在接收参数）
    void updatePlayerInfo(int player1Time, int player2Time, bool currentPlayer);
    void updateGameStatus(bool isGameStarted, bool isGameOver);

    // 成员变量
    Ui::GameWidget *ui;
    ChessBoardWidget *chessBoard;

public:
    // 获取棋盘部件
    ChessBoardWidget *getChessBoard() const { return chessBoard; }

    // 接收全量游戏状态的槽函数
public slots:
    void updateGameState(bool isGameStarted, bool isGameOver, bool currentPlayer,
                         int player1Time, int player2Time,
                         const QString &player1Name, const QString &player2Name,
                         int player1Rating, int player2Rating);
};

#endif // GAMEWIDGET_H
