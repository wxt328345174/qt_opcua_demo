QT += core gui widgets

CONFIG += c++11
CONFIG -= app_bundle

TEMPLATE = app
TARGET = qt_opcua_demo

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/simulatedbackend.cpp

HEADERS += \
    src/idatabackend.h \
    src/mainwindow.h \
    src/simulatedbackend.h \
    src/variableinfo.h

win32:msvc:QMAKE_CXXFLAGS += /utf-8
