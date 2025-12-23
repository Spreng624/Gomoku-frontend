#include "LobbyWidget.h"
#include "ui_lobbywidget.h"
#include <QLabel>
#include <QFont>
#include <QHeaderView>
#include <QListWidgetItem>
#include <QResizeEvent>
#include <QPushButton>
#include <QFile>
#include <QTextStream>
#include "Logger.h"

LobbyWidget::LobbyWidget(QWidget *parent) : QWidget(parent), ui(new Ui::LobbyWidget)
{
    LOG_DEBUG("LobbyWidget Initializing...");
    ui->setupUi(this);

    initStyle();

    // 设置表格属性
    LOG_DEBUG("Configuring room table properties...");
    ui->roomTableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->roomTableWidget->verticalHeader()->setDefaultSectionSize(40);
    ui->roomTableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->roomTableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->roomTableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    ui->roomTableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    ui->roomTableWidget->setColumnWidth(3, 80);
    LOG_DEBUG("Room table properties configured");

    // 初始化表格设置
    LOG_DEBUG("Initializing room table...");
    initRoomTable();

    LOG_DEBUG("Initializing player list...");
    initPlayerList();

    LOG_DEBUG("Setting up signals...");
    setUpSignals();

    LOG_INFO("LobbyWidget initialized successfully");
}

LobbyWidget::~LobbyWidget()
{
    LOG_DEBUG("LobbyWidget destructor called");
    delete ui;
    LOG_DEBUG("LobbyWidget cleanup completed");
}

void LobbyWidget::setUpSignals()
{
    // 连接按钮信号槽
    connect(ui->localGameButton, &QPushButton::clicked, this, [this]()
            {
        LOG_DEBUG("Local game button clicked, emitting localGame signal");
        emit localGame(); });
    connect(ui->createRoomButton, &QPushButton::clicked, this, &LobbyWidget::createRoom);
    connect(ui->quickMatchButton, &QPushButton::clicked, this, &LobbyWidget::quickMatch);
    // connect(ui->roomFilterComboBox, &QComboBox::currentTextChanged, this, );
}

void LobbyWidget::updatePlayerList(const QStringList &players)
{
    ui->playerListWidget->clear();
    ui->playerListWidget->addItems(players);

    for (int i = 0; i < ui->playerListWidget->count(); i++)
    {
        QListWidgetItem *item = ui->playerListWidget->item(i);
        if (item->text().contains("在线"))
        {
            item->setForeground(QColor("#1a7f37")); // GitHub绿色
        }
        else if (item->text().contains("忙碌"))
        {
            item->setForeground(QColor("#d1242f")); // GitHub红色
        }
        else if (item->text().contains("离线"))
        {
            item->setForeground(QColor("#8c959f")); // GitHub灰色
        }
    }
}

void LobbyWidget::updateRoomList(const QStringList &rooms)
{

    for (int i = 0; i < rooms.size() / 3; i++)
    {

        QWidget *btnWidget = new QWidget(ui->roomTableWidget);
        QHBoxLayout *layout = new QHBoxLayout(btnWidget); // 水平布局让按钮居中
        layout->setContentsMargins(0, 0, 0, 0);           // 去掉布局边距，适配单元格

        QPushButton *btnJoin = new QPushButton("加入");
        btnJoin->setStyleSheet("QPushButton{padding: 2px 8px;}"); // 可选：调整按钮样式

        layout->addWidget(btnJoin);
        btnWidget->setLayout(layout);

        ui->roomTableWidget->insertRow(i);
        ui->roomTableWidget->setItem(i, 0, new QTableWidgetItem(rooms[i * 3]));
        ui->roomTableWidget->setItem(i, 1, new QTableWidgetItem(rooms[i * 3 + 1]));
        ui->roomTableWidget->setItem(i, 2, new QTableWidgetItem(rooms[i * 3 + 2]));
        ui->roomTableWidget->setCellWidget(i, 3, btnWidget);

        connect(btnJoin, &QPushButton::clicked, this, [this, i]()
                {
                    // 1. 读取当前行的房间ID（表格0列）
                    QTableWidgetItem *roomIdItem = ui->roomTableWidget->item(i, 0);
                    if (!roomIdItem || roomIdItem->text().isEmpty())
                    {
                        return; // 可选：提示无有效房间ID
                    }

                    // 2. 转换房间ID为整数（如果joinRoom需要int参数，按需调整）
                    bool ok;
                    int roomId = roomIdItem->text().toInt(&ok);
                    if (!ok)
                    {
                        return; // 若房间ID是字符串，直接传QString：QString roomId = roomIdItem->text();
                    }

                    // 3. 调用joinRoom（根据实际参数类型调整）
                    this->joinRoom(roomId); // 若joinRoom参数是QString则传 roomIdItem->text()
                });

        // 对战中房间禁用加入按钮
        if (rooms[i * 3 + 1] == "对战中")
        {
            btnJoin->setEnabled(false);
        }
    }
}

// 初始化界面样式 - GitHub Light风格
void LobbyWidget::initStyle()
{
}

// 初始化房间列表表格
void LobbyWidget::initRoomTable()
{
    ui->roomTableWidget->setColumnCount(4);
    ui->roomTableWidget->setHorizontalHeaderLabels({"房间号", "对局状态", "玩家", "操作"});
    ui->roomTableWidget->verticalHeader()->setVisible(false);

    // 示例数据
    QStringList roomData = {
        "#001", "空闲", "等待玩家加入",
        "#002", "对战中", "张三 vs 李四",
        "#003", "空闲", "王五 (等待对手)"};

    updateRoomList(roomData);
}

// 初始化在线玩家列表
void LobbyWidget::initPlayerList()
{
    QStringList playerData = {
        "玩家1 (在线)",
        "玩家2 (忙碌)",
        "玩家3 (在线)",
        "玩家4 (离线)",
        "玩家5 (在线)"};
    updatePlayerList(playerData);
}

// 可选：窗口大小变化时强制刷新表格（确保自适应生效）
void LobbyWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    // m_roomTable->resize(m_roomTable->parentWidget()->width() - m_playerList->width() - 1, m_roomTable->height());
}
