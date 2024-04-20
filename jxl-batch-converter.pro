QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    conversionthread.cpp \
    main.cpp \
    mainwindow.cpp \
    utils/folderselectiondialog.cpp \
    utils/logstats.cpp

HEADERS += \
    conversionthread.h \
    logcodes.h \
    mainwindow.h \
    utils/folderselectiondialog.h \
    utils/logstats.h

FORMS += \
    mainwindow.ui \
    utils/folderselectiondialog.ui

VERSION = 0.5.1
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
