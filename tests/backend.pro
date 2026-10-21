QT += core testlib
QT -= gui
CONFIG += c++17 console testcase
TARGET = backend-tests
SOURCES += backend_test.cpp ../src/workspacebackend.cpp
HEADERS += ../src/workspacebackend.h
RESOURCES += ../src/resources.qrc
INCLUDEPATH += ../src
DEFINES += DWCODE_SOURCE_ROOT=\\\"$$PWD/..\\\"
