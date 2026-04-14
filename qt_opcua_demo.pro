QT += core gui widgets

CONFIG += c++11
CONFIG -= app_bundle

TEMPLATE = app
TARGET = qt_opcua_demo

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    opcua_client.cpp

HEADERS += \
    mainwindow.h \
    opcua_client.h \
    variable_row.h

unix {
    packagesExist(open62541) {
        CONFIG += link_pkgconfig
        PKGCONFIG += open62541
    } else {
        message(open62541 pkg-config metadata not found; falling back to -lopen62541)
        LIBS += -lopen62541
    }
}

win32 {
    message(Windows is kept as an edit-only environment for this OPC UA demo.)
}

win32:msvc:QMAKE_CXXFLAGS += /utf-8
