QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
QT += printsupport

CONFIG += c++17

DESTDIR = $$PWD/bin

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    src/clprocreceiver.cpp \
    src/main.cpp \
    src/mainwindow.cpp \
    src/qcustomplot.cpp

HEADERS += \
    src/clprocreceiver.h \
    src/mainwindow.h \
    src/qcustomplot.h

OBJECTS_DIR = ./obj
MOC_DIR = ./moc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
