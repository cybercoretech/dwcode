QT += quick qml quickcontrols2 webenginequick network webchannel testlib
CONFIG += c++17 console testcase
TARGET = ui-tests
SOURCES += ui_test.cpp ../src/workspacebackend.cpp ../src/localwebserver.cpp
HEADERS += ../src/workspacebackend.h ../src/localwebserver.h
RESOURCES += ../src/resources.qrc
INCLUDEPATH += ../src
DEFINES += DWCODE_SOURCE_ROOT=\\\"$$PWD/..\\\"
DEFINES += DWCODE_WEB_ROOT=\\\"$$PWD/../web/dist\\\"
