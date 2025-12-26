#ifndef LOBBYWIDGET_H
#define LOBBYWIDGET_H

#include <QWidget>
#include <QResizeEvent>
#include <QList>
#include <QPair>
#include <QString>

namespace Ui
{
    class LobbyWidget;
}

class LobbyWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LobbyWidget(QWidget *parent = nullptr);
    ~LobbyWidget();

signals:
    // Just for Initialization
    void freshPlayerList();
    void freshRoomList();

    // TopBar
    void localGame(); // --> GameWidget
    void quickMatch();
    void createRoom();
    void joinRoom(int roomId);

public slots:
    void updatePlayerList(const QStringList &players);
    void updateRoomList(const QStringList &rooms);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    Ui::LobbyWidget *ui;

    void initStyle();
    void initRoomTable();
    void initPlayerList();
    void setUpSignals();
};

#endif // LOBBYWIDGET_H
