QT += quick qml quickcontrols2 webenginequick network webchannel
CONFIG += c++17
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app
TARGET = dwcode

DEFINES += DWCODE_WEB_ROOT=\\\"$$PWD/web/dist\\\"

SOURCES += src/main.cpp \
           src/localwebserver.cpp

HEADERS += src/localwebserver.h

RESOURCES += src/resources.qrc

QML_IMPORT_PATH += /usr/lib/qt6/qml

INCLUDEPATH += $$PWD/src

DEFINES += DWCODE_SOURCE_ROOT=\\\"$$PWD\\\"
SOURCES += src/workspacebackend.cpp
HEADERS += src/workspacebackend.h
