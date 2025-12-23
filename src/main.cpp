#include "MainWindow.h"
#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QDebug>

void loadStyleSheet(const QString &sheetName)
{
    QFile file(sheetName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qWarning() << "Failed to open style sheet file:" << sheetName;
        // 使用后备内联样式表
        QString fallbackStyle = R"(
            #centralContainer {
                background-color: #ffffff;
                border: 1px solid #d0d7de;
                border-top-left-radius: 12px;
                border-top-right-radius: 12px;
            }
            #titleBarWidget {
                background-color: #f6f8fa;
                border-top-left-radius: 12px;
                border-top-right-radius: 12px;
                border-bottom: 1px solid #d0d7de;
                color: #24292f;
                font-weight: bold;
                padding-left: 15px;
            }
            #titleBarWidget QPushButton {
                border: none;
                background: transparent;
                width: 45px;
                height: 40px;
                font-size: 16px;
                color: #57606a;
            }
            #titleBarWidget QPushButton:hover {
                background-color: #eaeef2;
            }
            #closeButton:hover {
                background-color: #cf222e !important;
                color: white !important;
                border-top-right-radius: 12px;
            }
            #statusBarWidget {
                background-color: #f6f8fa;
                border-top: 1px solid #d0d7de;
                color: #57606a;
                border-bottom-left-radius: 6px;
                border-bottom-right-radius: 6px;
            }
            QStackedWidget {
                background-color: white;
            }
        )";
        qApp->setStyleSheet(fallbackStyle);
        qDebug() << "Using fallback style sheet";
        return;
    }

    QString styleSheet = QString::fromUtf8(file.readAll());
    file.close();

    if (styleSheet.isEmpty())
    {
        qWarning() << "Style sheet is empty:" << sheetName;
    }
    else
    {
        qApp->setStyleSheet(styleSheet);
        qDebug() << "Style sheet loaded from:" << sheetName;
    }
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 加载全局样式表
    loadStyleSheet("src/style.qss");

    MainWindow w;
    w.show();

    return a.exec();
}
