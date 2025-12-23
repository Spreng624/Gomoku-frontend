#ifndef TOASTWIDGET_H
#define TOASTWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPropertyAnimation>
#include <QTimer>
#include <QGraphicsOpacityEffect>

class ToastWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(float opacity READ opacity WRITE setOpacity)

public:
    explicit ToastWidget(QWidget *parent = nullptr);
    ~ToastWidget();

    void showMessage(const QString &message, int duration = 3000);
    void setMessage(const QString &message);

    float opacity() const;
    void setOpacity(float opacity);

signals:
    void hidden();

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void hideAnimationFinished();

private:
    void setupUI();
    void startShowAnimation();
    void startHideAnimation();

    QLabel *messageLabel;
    QPropertyAnimation *showAnimation;
    QPropertyAnimation *hideAnimation;
    QTimer *hideTimer;
    QGraphicsOpacityEffect *opacityEffect;
    float m_opacity;
};

#endif // TOASTWIDGET_H