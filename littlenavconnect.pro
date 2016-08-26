#-------------------------------------------------
#
# Project created by QtCreator 2016-05-01T19:15:00
#
#-------------------------------------------------

QT       += core gui network svg

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

TARGET = littlenavconnect
TEMPLATE = app

# Adapt these variables to compile on Windows
win32 {
  QT_BIN=C:\\Qt\\5.5\\msvc2013\\bin
  GIT_BIN='C:\\Git\\bin\\git'
  SIMCONNECT="C:\Program Files (x86)\Microsoft Games\Microsoft Flight Simulator X SDK"
}

# Get the current GIT revision to include it into the code
win32:DEFINES += GIT_REVISION='\\"$$system($${GIT_BIN} rev-parse --short HEAD)\\"'
# Disable microsofts min/max defines
win32:DEFINES += NOMINMAX

unix:DEFINES += GIT_REVISION='\\"$$system(git rev-parse --short HEAD)\\"'


SOURCES +=\
    src/main.cpp \
    src/mainwindow.cpp \
    src/datareaderthread.cpp \
    src/simconnecthandler.cpp \
    src/optionsdialog.cpp \
    src/navserver.cpp \
    src/navserverworker.cpp \
    src/navservercommon.cpp

HEADERS  += \
    src/mainwindow.h \
    src/datareaderthread.h \
    src/simconnecthandler.h \
    src/optionsdialog.h \
    src/navservercommon.h \
    src/navserver.h \
    src/navserverworker.h

FORMS    += mainwindow.ui \
    optionsdialog.ui

RESOURCES += \
    littlenavconnect.qrc

DISTFILES += \
    uncrustify.cfg \
    README.txt \
    BUILD.txt \
    CHANGELOG.txt \
    htmltidy.cfg \
    LICENSE.txt \
    help/en/index.html \
    help/en/images/gpl-v3-logo.svg \
    htmltidy.cfg \
    BUILD.txt \
    help/en/css/style.css

# Add dependencies to atools project and its static library to ensure relinking on changes
DEPENDPATH += $$PWD/../atools/src
INCLUDEPATH += $$PWD/../atools/src $$PWD/src

unix {
CONFIG(debug, debug|release) {
  LIBS += -L$$PWD/../build-atools-debug -latools
  PRE_TARGETDEPS += $$PWD/../build-atools-debug/libatools.a
}
CONFIG(release, debug|release) {
  LIBS += -L$$PWD/../build-atools-release -latools
  PRE_TARGETDEPS += $$PWD/../build-atools-release/libatools.a
}
}

win32 {
CONFIG(debug, debug|release) {
  LIBS += -L$$PWD/../build-atools-debug/debug -latools
  PRE_TARGETDEPS += $$PWD/../build-atools-debug/debug/atools.lib
  WINDEPLOY_FLAGS = --no-system-d3d-compiler --debug --compiler-runtime
}
CONFIG(release, debug|release) {
  LIBS += -L$$PWD/../build-atools-release/release -latools
  PRE_TARGETDEPS += $$PWD/../build-atools-release/release/atools.lib
  WINDEPLOY_FLAGS = --no-system-d3d-compiler --release --compiler-runtime
}

INCLUDEPATH += "C:\Program Files (x86)\Microsoft Games\Microsoft Flight Simulator X SDK\SDK\Core Utilities Kit\SimConnect SDK\inc"
LIBS += "C:\Program Files (x86)\Microsoft Games\Microsoft Flight Simulator X SDK\SDK\Core Utilities Kit\SimConnect SDK\lib\SimConnect.lib"
}

# Create additional makefile targets to copy help files
unix {
  copydata.commands = cp -avfu $$PWD/help $$OUT_PWD
  cleandata.commands = rm -Rvf $$OUT_PWD/help
}

# Windows specific deploy target
win32 {
  RC_ICONS = resources/icons/navconnect.ico

  # Create backslashed path
  WINPWD=$${PWD}
  WINPWD ~= s,/,\\,g
  WINOUT_PWD=$${OUT_PWD}
  WINOUT_PWD ~= s,/,\\,g
  DEPLOY_DIR_NAME=Little Navconnect
  DEPLOY_DIR_WIN=\"$${WINPWD}\\..\\deploy\\$${DEPLOY_DIR_NAME}\"

  copydata.commands = xcopy /i /s /e /f /y $${WINPWD}\\help $${WINOUT_PWD}\\help

  cleandata.commands = del /s /q $${WINOUT_PWD}\\help

  deploy.commands = rmdir /s /q $${DEPLOY_DIR_WIN} &
  deploy.commands += mkdir $${DEPLOY_DIR_WIN} &&
  deploy.commands += xcopy $${WINOUT_PWD}\\release\\littlenavconnect.exe $${DEPLOY_DIR_WIN} &&
  deploy.commands += xcopy $${WINPWD}\\CHANGELOG.txt $${DEPLOY_DIR_WIN} &&
  deploy.commands += xcopy $${WINPWD}\\README.txt $${DEPLOY_DIR_WIN} &&
  deploy.commands += xcopy $${WINPWD}\\LICENSE.txt $${DEPLOY_DIR_WIN} &&
  deploy.commands += xcopy /i /s /e /f /y $${WINPWD}\\help $${DEPLOY_DIR_WIN}\\help &&
  deploy.commands += $${QT_BIN}\\windeployqt $${WINDEPLOY_FLAGS} $${DEPLOY_DIR_WIN}
}

QMAKE_EXTRA_TARGETS += deploy

first.depends = $(first) copydata
QMAKE_EXTRA_TARGETS += first copydata

clean.depends = $(clean) cleandata
QMAKE_EXTRA_TARGETS += clean cleandata


