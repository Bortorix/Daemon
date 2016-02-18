TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CFLAGS = -I/usr/include/
QMAKE_CXXFLAGS = -I/usr/include/

SOURCES += main.c \
    daemon.c \
    io_operations.c \
    log_manager.c \
    control_app.c

include(deployment.pri)
qtcAddDeployment()

HEADERS += \
    daemon.h \
    io_operations.h \
    log_manager.h \
    def.h \
    status.h \
    control_app.h

