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
    afterselldatacollector.cpp \
    browserwindow.cpp \
    collectcontroller.cpp \
    collectorbase.cpp \
    collectstatusmanager.cpp \
    commentdatacollector.cpp \
    datacollector.cpp \
    datamodel.cpp \
    exceldialog.cpp \
    excelhandler.cpp \
    filedownloader.cpp \
    gettextdialog.cpp \
    httpthread.cpp \
    loginutil.cpp \
    main.cpp \
    mainwindow.cpp \
    myprogressdialog.cpp \
    paycollector.cpp \
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
    afterselldatacollector.h \
    browserwindow.h \
    collectcontroller.h \
    collectorbase.h \
    collectstatusmanager.h \
    commentdatacollector.h \
    datacollector.h \
    datamodel.h \
    exceldialog.h \
    excelhandler.h \
    filedownloader.h \
    gettextdialog.h \
    httpthread.h \
    loginutil.h \
    mainwindow.h \
    myprogressdialog.h \
    paycollector.h \
    settingmanager.h \
    shopitemwidget.h \
    shopmanager.h \
    uiutil.h

FORMS += \
    browserwindow.ui \
    exceldialog.ui \
    gettextdialog.ui \
    mainwindow.ui \
    shopitemwidget.ui

# Enable PDB generation
QMAKE_CFLAGS_RELEASE += /Zi
QMAKE_CXXFLAGS_RELEASE += /Zi
QMAKE_LFLAGS_RELEASE += /DEBUG

# Enable log context
DEFINES += QT_MESSAGELOGCONTEXT

# QXlsx
include(../thirdparty/QXlsx/QXlsx.pri)
INCLUDEPATH += ../thirdparty/QXlsx/header/

# brotli
INCLUDEPATH += ../thirdparty/brotli/include
LIBS += -L"$$_PRO_FILE_PWD_/../thirdparty/brotli/lib" -lbrotlidec

# curl
INCLUDEPATH += ../curl/include
LIBS += -L"$$_PRO_FILE_PWD_/../curl/lib" -llibcurl

