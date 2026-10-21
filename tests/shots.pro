QT += quick qml quickcontrols2 webenginequick network webchannel testlib
CONFIG += c++17 console
TARGET = site-shots
SOURCES += shots.cpp ../src/workspacebackend.cpp ../src/localwebserver.cpp
HEADERS += ui_harness.h ../src/workspacebackend.h ../src/localwebserver.h
RESOURCES += ../src/resources.qrc
INCLUDEPATH += ../src
DEFINES += DWCODE_SOURCE_ROOT=\\\"$$PWD/..\\\"
DEFINES += DWCODE_WEB_ROOT=\\\"$$PWD/../web/dist\\\"
