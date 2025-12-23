QT += core gui network multimedia

TARGET = Gomoku-frontend
CONFIG += c++17 depends_aware console
QT += widgets

INCLUDEPATH +=  src src/network src/utils
DEPENDPATH  +=  src src/network src/utils
VPATH +=  src src/network src/utils

# 链接Windows Socket API和BCrypt库
LIBS += -lws2_32 -lbcrypt

INCLUDEPATH += . \
               src \
               src/core \
               src/network \
               src/widgets \
               src/utils


DEPENDPATH += . \
               src \
               src/core \
               src/network \
               src/widgets \
               src/utils

VPATH += . \
        src \
        src/core \
        src/network \
        src/widgets \
        src/utils

SOURCES += src/core/Game.cpp \
           src/core/manager.cpp \
           src/core/GameManager.cpp \
           src/network/Client.cpp \
           src/network/Frame.cpp \
           src/network/Packet.cpp \
           src/utils/Crypto.cpp \
           src/utils/Logger.cpp \
           src/widgets/ChessBoardWidget.cpp \
           src/widgets/gamewidget.cpp \
           src/widgets/lobbywidget.cpp \
           src/widgets/ToastWidget.cpp \
           src/main.cpp \
           src/MainWindow.cpp

HEADERS += src/core/Game.h \
           src/core/Manager.h \
           src/core/GameManager.h \
           src/network/Client.h \
           src/network/Frame.h \
           src/network/Packet.h \
           src/utils/Crypto.h \
           src/utils/Logger.h \
           src/utils/EventBus.hpp \
           src/utils/Timer.hpp \
           src/utils/TimeWheel.hpp \
           src/widgets/ChessBoardWidget.h \
           src/widgets/GameWidget.h \
           src/widgets/LobbyWidget.h \
           src/widgets/ToastWidget.h \
           src/MainWindow.h

FORMS += ui/mainwindow.ui \
         ui/lobbywidget.ui \
         ui/gamewidget.ui