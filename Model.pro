QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

QMAKE_CXXFLAGS += -finput-charset=UTF-8 -fexec-charset=UTF-8

SOURCES += \
    bank.cpp \
    generators.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    bank.h \
    generators.h \
    mainwindow.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
