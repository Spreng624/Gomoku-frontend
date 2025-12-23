#include "ToastWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QApplication>
#include <QScreen>
#include <QDebug>

ToastWidget::ToastWidget(QWidget *parent)
    : QWidget(parent), messageLabel(nullptr), showAnimation(nullptr), hideAnimation(nullptr), hideTimer(nullptr), opacityEffect(nullptr), m_opacity(1.0f)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);

    setupUI();

    // 创建动画
    showAnimation = new QPropertyAnimation(this, "geometry", this);
    hideAnimation = new QPropertyAnimation(this, "geometry", this);

    // 创建定时器
    hideTimer = new QTimer(this);
    hideTimer->setSingleShot(true);
    connect(hideTimer, &QTimer::timeout, this, &ToastWidget::startHideAnimation);

    // 创建透明度效果
    opacityEffect = new QGraphicsOpacityEffect(this);
    opacityEffect->setOpacity(1.0);
    setGraphicsEffect(opacityEffect);

    // 设置初始大小和位置
    resize(300, 60);
    hide();
}

ToastWidget::~ToastWidget()
{
}

void ToastWidget::setupUI()
{
    // 设置样式
    setStyleSheet(R"(
        QWidget {
            background-color: rgba(255, 255, 255, 230);
            border: 1px solid #d0d7de;
            border-radius: 8px;
        }
        QLabel {
            color: #24292f;
            font-size: 14px;
            font-weight: 500;
            padding: 12px 16px;
        }
    )");

    // 创建消息标签
    messageLabel = new QLabel(this);
    messageLabel->setAlignment(Qt::AlignCenter);
    messageLabel->setWordWrap(true);

    // 创建布局
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(messageLabel);
    setLayout(layout);
}

void ToastWidget::showMessage(const QString &message, int duration)
{
    setMessage(message);

    // 获取主屏幕
    QScreen *screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();

    // 计算初始位置（屏幕右下角，隐藏状态）
    int x = screenGeometry.right() - width() - 20;
    int y = screenGeometry.bottom() + height(); // 完全在屏幕下方

    // 设置初始位置
    move(x, y);
    show();
    raise();

    // 开始显示动画
    startShowAnimation();

    // 设置定时器，duration毫秒后开始隐藏
    hideTimer->start(duration);
}

void ToastWidget::setMessage(const QString &message)
{
    if (messageLabel)
    {
        messageLabel->setText(message);

        // 根据文本长度调整大小
        QFontMetrics fm(messageLabel->font());
        int textWidth = fm.horizontalAdvance(message) + 32; // 加上内边距
        int textHeight = fm.height() * (message.count('\n') + 1) + 24;

        // 限制最小和最大大小
        textWidth = qMax(200, qMin(400, textWidth));
        textHeight = qMax(50, qMin(100, textHeight));

        resize(textWidth, textHeight);
    }
}

float ToastWidget::opacity() const
{
    return m_opacity;
}

void ToastWidget::setOpacity(float opacity)
{
    m_opacity = opacity;
    if (opacityEffect)
    {
        opacityEffect->setOpacity(opacity);
    }
    update();
}

void ToastWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制圆角矩形背景
    QPainterPath path;
    path.addRoundedRect(rect(), 8, 8);

    // 设置背景颜色和阴影
    painter.fillPath(path, QColor(255, 255, 255, 230));

    // 绘制边框
    painter.setPen(QPen(QColor(208, 215, 222, 200), 1));
    painter.drawPath(path);

    // 绘制轻微阴影
    QPainterPath shadowPath;
    shadowPath.addRoundedRect(rect().translated(0, 2), 8, 8);
    painter.fillPath(shadowPath, QColor(0, 0, 0, 20));

    QWidget::paintEvent(event);
}

void ToastWidget::startShowAnimation()
{
    if (!showAnimation)
        return;

    // 获取主屏幕
    QScreen *screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();

    // 计算目标位置（屏幕右下角，距离底部20像素）
    int startX = screenGeometry.right() - width() - 20;
    int startY = screenGeometry.bottom() + height();
    int endX = startX;
    int endY = screenGeometry.bottom() - height() - 20;

    // 设置动画
    showAnimation->setDuration(300);
    showAnimation->setStartValue(QRect(startX, startY, width(), height()));
    showAnimation->setEndValue(QRect(endX, endY, width(), height()));
    showAnimation->setEasingCurve(QEasingCurve::OutBack);

    // 同时设置透明度动画
    QPropertyAnimation *opacityAnim = new QPropertyAnimation(opacityEffect, "opacity", this);
    opacityAnim->setDuration(300);
    opacityAnim->setStartValue(0.0);
    opacityAnim->setEndValue(1.0);
    opacityAnim->setEasingCurve(QEasingCurve::OutCubic);

    // 启动动画
    showAnimation->start();
    opacityAnim->start(QAbstractAnimation::DeleteWhenStopped);
}

void ToastWidget::startHideAnimation()
{
    if (!hideAnimation)
        return;

    // 获取主屏幕
    QScreen *screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();

    // 计算目标位置（屏幕右下角，完全隐藏）
    int startX = screenGeometry.right() - width() - 20;
    int startY = screenGeometry.bottom() - height() - 20;
    int endX = startX;
    int endY = screenGeometry.bottom() + height();

    // 设置动画
    hideAnimation->setDuration(300);
    hideAnimation->setStartValue(QRect(startX, startY, width(), height()));
    hideAnimation->setEndValue(QRect(endX, endY, width(), height()));
    hideAnimation->setEasingCurve(QEasingCurve::InBack);

    // 同时设置透明度动画
    QPropertyAnimation *opacityAnim = new QPropertyAnimation(opacityEffect, "opacity", this);
    opacityAnim->setDuration(300);
    opacityAnim->setStartValue(1.0);
    opacityAnim->setEndValue(0.0);
    opacityAnim->setEasingCurve(QEasingCurve::InCubic);

    // 连接动画结束信号
    connect(hideAnimation, &QPropertyAnimation::finished, this, &ToastWidget::hideAnimationFinished);

    // 启动动画
    hideAnimation->start();
    opacityAnim->start(QAbstractAnimation::DeleteWhenStopped);
}

void ToastWidget::hideAnimationFinished()
{
    hide();
    emit hidden();
}