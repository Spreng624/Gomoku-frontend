#ifndef CHESSBOARDWIDGET_H
#define CHESSBOARDWIDGET_H

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include "Game.h"

class ChessBoardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ChessBoardWidget(QWidget *parent = nullptr);

    void resetGame();
    void setGame(Game *game);
    void setCurrentPlayer(bool currentPlayer);
    void setGameOver(bool gameOver, const QString &winner = "");
    void refreshBoard();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

signals:
    void moveMade(int x, int y);

private:
    void drawChessBoard(QPainter &painter);
    void drawPieces(QPainter &painter);
    void drawCurrentPlayerIndicator(QPainter &painter);
    QPoint boardPosToGrid(const QPoint &pos);

    Game *game;
    bool currentPlayer;
    bool isGameOver;
    QString winner;

    int boardSize;
    int gridSize;
    int pieceRadius;
    QPoint boardTopLeft;
};

#endif // CHESSBOARDWIDGET_H
