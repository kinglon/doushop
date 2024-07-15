QT       += core gui network webenginewidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    Utility/DumpUtil.cpp \
    Utility/IcrCriticalSection.cpp \
    Utility/ImCharset.cpp \
    Utility/ImPath.cpp \
    Utility/LogBuffer.cpp \
    Utility/LogUtil.cpp \
    browserwindow.cpp \
    datamodel.cpp \
    gettextdialog.cpp \
    loginutil.cpp \
    main.cpp \
    mainwindow.cpp \
    settingmanager.cpp \
    shopitemwidget.cpp \
    shopmanager.cpp \
    uiutil.cpp

HEADERS += \
    Utility/DumpUtil.h \
    Utility/IcrCriticalSection.h \
    Utility/ImCharset.h \
    Utility/ImPath.h \
    Utility/LogBuffer.h \
    Utility/LogMacro.h \
    Utility/LogUtil.h \
    browserwindow.h \
    datamodel.h \
    gettextdialog.h \
    loginutil.h \
    mainwindow.h \
    settingmanager.h \
    shopitemwidget.h \
    shopmanager.h \
    uiutil.h

FORMS += \
    browserwindow.ui \
    gettextdialog.ui \
    mainwindow.ui \
    shopitemwidget.ui

# Enable PDB generation
QMAKE_CFLAGS_RELEASE += /Zi
QMAKE_CXXFLAGS_RELEASE += /Zi
QMAKE_LFLAGS_RELEASE += /DEBUG

# Enable log context
DEFINES += QT_MESSAGELOGCONTEXT

# QXlsx code for Application Qt project
include(../thirdparty/QXlsx/QXlsx.pri)
INCLUDEPATH += ../thirdparty/QXlsx/header/

