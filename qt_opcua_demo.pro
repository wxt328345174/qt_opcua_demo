QT += core gui widgets

CONFIG += c++11
CONFIG -= app_bundle

TEMPLATE = app
TARGET = qt_opcua_demo

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    simulator.cpp

HEADERS += \
    mainwindow.h \
    simulator.h

win32:msvc:QMAKE_CXXFLAGS += /utf-8
