#include "ChessBoardWidget.h"
#include <QPainterPath>
#include <QFont>
#include <QDebug>

ChessBoardWidget::ChessBoardWidget(QWidget *parent)
    : QWidget(parent), game(nullptr), currentPlayer(true),
      isGameOver(false), boardSize(15), gridSize(40), pieceRadius(18)
{
    setFixedSize(700, 700);
    setStyleSheet("background-color: #E8B96A;");
}

void ChessBoardWidget::resetGame()
{
    if (game)
    {
        game->reset();
    }
    currentPlayer = true;
    isGameOver = false;
    winner.clear();
    update();
}

void ChessBoardWidget::setGame(Game *game)
{
    qDebug() << "ChessBoardWidget::setGame called with game pointer:" << game;
    this->game = game;
    update(); // 设置游戏实例后立即重绘
    qDebug() << "ChessBoardWidget::setGame: update() called, game pointer set to:" << this->game;
}

void ChessBoardWidget::refreshBoard()
{
    qDebug() << "ChessBoardWidget::refreshBoard called, forcing repaint";
    update();
}

void ChessBoardWidget::setCurrentPlayer(bool currentPlayer)
{
    this->currentPlayer = currentPlayer;
    update();
}

void ChessBoardWidget::setGameOver(bool gameOver, const QString &winner)
{
    this->isGameOver = gameOver;
    this->winner = winner;
    update();
}

void ChessBoardWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制棋盘背景
    painter.fillRect(rect(), QColor(222, 184, 135));

    // 绘制棋盘网格
    drawChessBoard(painter);

    // 绘制棋子
    if (game)
    {
        drawPieces(painter);
    }

    // 绘制当前玩家指示器
    // drawCurrentPlayerIndicator(painter); // 注释掉，不显示当前玩家指示器

    // 如果游戏结束，显示获胜信息
    if (isGameOver && !winner.isEmpty())
    {
        painter.setPen(QPen(QColor(239, 68, 68, 200), 3));
        painter.setBrush(QBrush(QColor(239, 68, 68, 180)));
        painter.drawRect(250, 320, 200, 60);

        painter.setPen(Qt::white);
        painter.setFont(QFont("Microsoft YaHei", 16, QFont::Bold));
        painter.drawText(QRect(250, 320, 200, 60), Qt::AlignCenter, winner + " 獲勝！");
    }
}

void ChessBoardWidget::drawChessBoard(QPainter &painter)
{
    // 计算棋盘实际位置
    int boardLength = gridSize * (boardSize - 1);
    boardTopLeft = QPoint((width() - boardLength) / 2,
                          (height() - boardLength) / 2);

    // 绘制网格线
    painter.setPen(QPen(Qt::black, 2));

    for (int i = 0; i < boardSize; i++)
    {
        // 横线
        int y = boardTopLeft.y() + i * gridSize;
        painter.drawLine(boardTopLeft.x(), y,
                         boardTopLeft.x() + boardLength, y);

        // 竖线
        int x = boardTopLeft.x() + i * gridSize;
        painter.drawLine(x, boardTopLeft.y(),
                         x, boardTopLeft.y() + boardLength);
    }

    // 绘制天元和星
    int center = boardSize / 2;
    int starPoints[4][2] = {{3, 3}, {3, 11}, {11, 3}, {11, 11}};

    painter.setBrush(Qt::black);
    painter.setPen(Qt::NoPen);

    // 绘制天元
    QPoint centerPoint(boardTopLeft.x() + center * gridSize,
                       boardTopLeft.y() + center * gridSize);
    painter.drawEllipse(centerPoint, 4, 4);

    // 绘制星
    for (auto &point : starPoints)
    {
        QPoint starPoint(boardTopLeft.x() + point[0] * gridSize,
                         boardTopLeft.y() + point[1] * gridSize);
        painter.drawEllipse(starPoint, 4, 4);
    }
}

void ChessBoardWidget::drawPieces(QPainter &painter)
{
    if (!game)
    {
        qDebug() << "ChessBoardWidget::drawPieces: game is null, cannot draw pieces";
        return;
    }

    qDebug() << "ChessBoardWidget::drawPieces: game pointer:" << game;
    const auto &board = game->getBoard();
    int pieceCount = 0;

    // 调试：输出棋盘状态
    qDebug() << "ChessBoardWidget::drawPieces: board size:" << board.size() << "x" << (board.size() > 0 ? board[0].size() : 0);

    for (int i = 0; i < boardSize; i++)
    {
        for (int j = 0; j < boardSize; j++)
        {
            if (board[i][j] != Piece::EMPTY)
            {
                pieceCount++;
                qDebug() << "  Piece at (" << i << "," << j << "):"
                         << (board[i][j] == Piece::BLACK ? "BLACK" : "WHITE");
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

    if (pieceCount > 0)
    {
        qDebug() << "ChessBoardWidget::drawPieces: drew" << pieceCount << "pieces";
    }
    else
    {
        qDebug() << "ChessBoardWidget::drawPieces: no pieces to draw (empty board)";
    }
}

void ChessBoardWidget::drawCurrentPlayerIndicator(QPainter &painter)
{
    if (!isGameOver)
    {
        // 在棋盘旁边显示当前玩家指示器
        int indicatorX = boardTopLeft.x() + (boardSize - 1) * gridSize + 30;
        int indicatorY = boardTopLeft.y() + (boardSize / 2) * gridSize;

        painter.setPen(Qt::NoPen);

        if (currentPlayer)
        {
            // 黑棋指示器
            QRadialGradient blackGrad(indicatorX, indicatorY, 12);
            blackGrad.setColorAt(0, QColor(100, 100, 100));
            blackGrad.setColorAt(1, QColor(0, 0, 0));
            painter.setBrush(QBrush(blackGrad));
            painter.drawEllipse(indicatorX - 10, indicatorY - 10, 20, 20);

            painter.setPen(QPen(Qt::black, 2));
            painter.setFont(QFont("Microsoft YaHei", 12, QFont::Bold));
            painter.drawText(indicatorX + 15, indicatorY + 5, "黑方回合");
        }
        else
        {
            // 白棋指示器
            QRadialGradient whiteGrad(indicatorX, indicatorY, 12);
            whiteGrad.setColorAt(0, QColor(255, 255, 255));
            whiteGrad.setColorAt(1, QColor(200, 200, 200));
            painter.setBrush(QBrush(whiteGrad));
            painter.setPen(QPen(Qt::black, 1));
            painter.drawEllipse(indicatorX - 10, indicatorY - 10, 20, 20);

            painter.setPen(QPen(Qt::white, 2));
            painter.setFont(QFont("Microsoft YaHei", 12, QFont::Bold));
            painter.drawText(indicatorX + 15, indicatorY + 5, "白方回合");
        }
    }
}

void ChessBoardWidget::mousePressEvent(QMouseEvent *event)
{
    qDebug() << "ChessBoardWidget::mousePressEvent called, isGameOver:" << isGameOver << "game pointer:" << game;

    if (isGameOver)
    {
        qDebug() << "Game is over, ignoring click";
        return;
    }

    if (!game)
    {
        qDebug() << "Game pointer is null, cannot process move";
        return;
    }

    QPoint clickPos = event->pos();
    QPoint boardPos = boardPosToGrid(clickPos);

    qDebug() << "Click position:" << clickPos << "Board position:" << boardPos;

    if (boardPos.x() >= 0 && boardPos.x() < boardSize &&
        boardPos.y() >= 0 && boardPos.y() < boardSize)
    {
        qDebug() << "Emitting moveMade signal for position (" << boardPos.x() << "," << boardPos.y() << ")";
        emit makeMove(boardPos.x(), boardPos.y());
    }
    else
    {
        qDebug() << "Click outside board bounds";
    }
}

QPoint ChessBoardWidget::boardPosToGrid(const QPoint &pos)
{
    // 计算点击位置到棋盘网格的映射
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
