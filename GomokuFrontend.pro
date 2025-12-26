QT += core gui network multimedia

TARGET = Gomoku-frontend
CONFIG += c++17 depends_aware console
QT += widgets

INCLUDEPATH +=  src src/network src/utils
DEPENDPATH  +=  src src/network src/utils
VPATH +=  src src/network src/utils

# 链接Windows Socket API和BCrypt库
LIBS += -lws2_32 -lbcrypt -lpthread

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

SOURCES += src/core/AiPlayer.cpp \
           src/core/Controller.cpp \
           src/core/Game.cpp \
           src/network/Client.cpp \
           src/network/Frame.cpp \
           src/network/Packet.cpp \
           src/utils/Crypto.cpp \
           src/utils/Logger.cpp \
           src/utils/Timer.cpp \
           src/widgets/LobbyWidget.cpp \
           src/widgets/RoomWidget.cpp \
           src/widgets/ToastWidget.cpp \
           src/main.cpp \
           src/MainWindow.cpp

HEADERS += src/core/AiPlayer.h \
           src/core/Controller.h \
           src/core/Game.h \
           src/network/Client.h \
           src/network/Frame.h \
           src/network/Packet.h \
           src/utils/Crypto.h \
           src/utils/Logger.h \
           src/utils/Timer.hpp \
           src/utils/TimeWheel.hpp \\
           src/widgets/LobbyWidget.h \
           src/widgets/RoomWidget.h \
           src/widgets/ToastWidget.h \
           src/MainWindow.h

FORMS += ui/mainwindow.ui \
         ui/lobbywidget.ui \
         ui/gamewidget.ui