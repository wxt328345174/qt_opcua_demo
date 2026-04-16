QT += core gui widgets

CONFIG += c++11

TEMPLATE = app
TARGET = qt_opcua_demo

!linux:error(This project supports openEuler/Linux only.)

INCLUDEPATH += /usr/local/include
LIBS += /usr/local/lib64/libopen62541.a

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    opcua_client.cpp \
    opcua_nodes.cpp \
    opcua_value_codec.cpp

HEADERS += \
    mainwindow.h \
    opcua_client.h \
    opcua_nodes.h \
    opcua_value_codec.h \
    variable_row.h
