#*****************************************************************************
# Copyright 2015-2019 Alexander Barthel albar965@mailbox.org
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#****************************************************************************

QT       += core gui xml network svg

CONFIG += c++14

TARGET = littlenavconnect
TEMPLATE = app


# =======================================================================
# Adapt these paths for each operating system
# =======================================================================

# Windows ==================
win32 {
  QT_HOME=C:\\Qt\\5.9.5\\mingw53_32
  OPENSSL=C:\\OpenSSL-Win32
  GIT_BIN='C:\\Git\\bin\\git'
}

# Linux ==================
unix:!macx {
  QT_HOME=/home/alex/Qt/5.9.5/gcc_64

  # Find OpenSSL location
  exists( /lib/x86_64-linux-gnu/libssl.so.1.0.0 ) {
    OPENSSL=/lib/x86_64-linux-gnu
  }
  exists( /usr/lib/x86_64-linux-gnu/libssl.so.1.0.0 ) {
    OPENSSL=/usr/lib/x86_64-linux-gnu
  }
  QMAKE_LFLAGS += -no-pie
}

macx {
  # Compatibility down to OS X 10.10
  QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.10
  QT_HOME=/Users/alex/Qt/5.9.5/clang_64
}

# End of configuration section
# =======================================================================

DEFINES += QT_NO_CAST_FROM_BYTEARRAY
DEFINES += QT_NO_CAST_TO_ASCII
#DEFINES += QT_NO_CAST_FROM_ASCII

# =====================================================================
# Dependencies
# =====================================================================

# Add dependencies to atools project and its static library to ensure relinking on changes
DEPENDPATH += $$PWD/../atools/src
INCLUDEPATH += $$PWD/../atools/src $$PWD/src

CONFIG(debug, debug|release):CONF_TYPE=debug
CONFIG(release, debug|release):CONF_TYPE=release

unix {
  LIBS += -L$$PWD/../build-atools-$${CONF_TYPE} -latools
  PRE_TARGETDEPS += $$PWD/../build-atools-$${CONF_TYPE}/libatools.a
}

unix:!macx {
  # Makes the shell script and setting LD_LIBRARY_PATH redundant
  QMAKE_RPATHDIR=.
  QMAKE_RPATHDIR+=./lib
}

win32 {
  LIBS += -L$$PWD/../build-atools-$${CONF_TYPE}/$${CONF_TYPE} -latools
  PRE_TARGETDEPS += $$PWD/../build-atools-$${CONF_TYPE}/$${CONF_TYPE}/libatools.a
  WINDEPLOY_FLAGS = --compiler-runtime

  INCLUDEPATH += "C:\Program Files (x86)\Microsoft Games\Microsoft Flight Simulator X SDK\SDK\Core Utilities Kit\SimConnect SDK\inc"
}


# Get the current GIT revision to include it into the code
win32:DEFINES += GIT_REVISION='\\"$$system($${GIT_BIN} rev-parse --short HEAD)\\"'
unix:DEFINES += GIT_REVISION='\\"$$system(git rev-parse --short HEAD)\\"'

# =====================================================================
# Files
# =====================================================================

SOURCES +=\
    src/main.cpp \
    src/mainwindow.cpp \
    src/optionsdialog.cpp \
    src/constants.cpp

HEADERS  += \
    src/mainwindow.h \
    src/optionsdialog.h \
    src/constants.h

FORMS    += mainwindow.ui \
    optionsdialog.ui

DISTFILES += \
    uncrustify.cfg \
    README.txt \
    BUILD.txt \
    CHANGELOG.txt \
    htmltidy.cfg \
    LICENSE.txt

RESOURCES += \
    littlenavconnect.qrc

ICON=resources/icons/littlenavconnect.icns

# Create additional makefile targets to copy help files
unix:!macx {
  copydata.commands = cp -avfu $$PWD/help $$OUT_PWD &&
  copydata.commands += mkdir -p $$OUT_PWD/translations &&
  copydata.commands += cp -avfu $$PWD/*.qm $$OUT_PWD/translations &&
  copydata.commands += cp -avfu $$PWD/../atools/*.qm $$OUT_PWD/translations &&
  copydata.commands += cp -vf $$PWD/desktop/littlenavconnect*.sh $$OUT_PWD &&
  copydata.commands += chmod -v a+x $$OUT_PWD/littlenavconnect*.sh

  cleandata.commands = rm -Rvf $$OUT_PWD/help
}

# Mac OS X - Copy help and Marble plugins and data
macx {
  copydata.commands += cp -Rv $$PWD/help $$OUT_PWD/littlenavconnect.app/Contents/MacOS &&
  copydata.commands += cp -vf $$PWD/*.qm $$OUT_PWD/littlenavconnect.app/Contents/MacOS &&
  copydata.commands += cp -vf $$PWD/../atools/*.qm $$OUT_PWD/littlenavconnect.app/Contents/MacOS

  cleandata.commands = rm -Rvf $$OUT_PWD/help
}

# =====================================================================
# Deployment commands
# =====================================================================

# Linux specific deploy target
unix:!macx {
  DEPLOY_DIR=\"$$PWD/../deploy/Little Navconnect\"
  DEPLOY_DIR_LIB=\"$$PWD/../deploy/Little Navconnect/lib\"

  deploy.commands = rm -Rfv $${DEPLOY_DIR} &&
  deploy.commands += mkdir -pv $${DEPLOY_DIR_LIB} &&
  deploy.commands += mkdir -pv $${DEPLOY_DIR_LIB}/iconengines &&
  deploy.commands += mkdir -pv $${DEPLOY_DIR_LIB}/imageformats &&
  deploy.commands += mkdir -pv $${DEPLOY_DIR_LIB}/platforms &&
  deploy.commands += mkdir -pv $${DEPLOY_DIR_LIB}/platformthemes &&
  deploy.commands += cp -Rvf $${OUT_PWD}/help $${DEPLOY_DIR} &&
  deploy.commands += cp -Rvf $${OUT_PWD}/translations $${DEPLOY_DIR} &&
  deploy.commands += cp -Rvf $${OUT_PWD}/littlenavconnect $${DEPLOY_DIR} &&
  deploy.commands += cp -vf $$PWD/desktop/qt.conf $${DEPLOY_DIR} &&
  deploy.commands += cp -vf $${PWD}/CHANGELOG.txt $${DEPLOY_DIR} &&
  deploy.commands += cp -vf $${PWD}/README.txt $${DEPLOY_DIR} &&
  deploy.commands += cp -vf $${PWD}/LICENSE.txt $${DEPLOY_DIR} &&
  deploy.commands += cp -vf $${PWD}/resources/icons/navconnect.svg $${DEPLOY_DIR}/littlenavconnect.svg &&
  deploy.commands += cp -vfa $${QT_HOME}/translations/qt_??.qm  $${DEPLOY_DIR}/translations &&
  deploy.commands += cp -vfa $${QT_HOME}/translations/qt_??_??.qm  $${DEPLOY_DIR}/translations &&
  deploy.commands += cp -vfa $${QT_HOME}/translations/qtbase*.qm  $${DEPLOY_DIR}/translations &&
  deploy.commands += cp -vfa $${QT_HOME}/plugins/iconengines/libqsvgicon.so*  $${DEPLOY_DIR_LIB}/iconengines &&
  deploy.commands += cp -vfa $${QT_HOME}/plugins/imageformats/libqgif.so*  $${DEPLOY_DIR_LIB}/imageformats &&
  deploy.commands += cp -vfa $${QT_HOME}/plugins/imageformats/libqjpeg.so*  $${DEPLOY_DIR_LIB}/imageformats &&
  deploy.commands += cp -vfa $${QT_HOME}/plugins/imageformats/libqsvg.so*  $${DEPLOY_DIR_LIB}/imageformats &&
  deploy.commands += cp -vfa $${QT_HOME}/plugins/imageformats/libqwbmp.so*  $${DEPLOY_DIR_LIB}/imageformats &&
  deploy.commands += cp -vfa $${QT_HOME}/plugins/imageformats/libqwebp.so*  $${DEPLOY_DIR_LIB}/imageformats &&
  deploy.commands += cp -vfa $${QT_HOME}/plugins/platforms/libqeglfs.so*  $${DEPLOY_DIR_LIB}/platforms &&
  deploy.commands += cp -vfa $${QT_HOME}/plugins/platforms/libqlinuxfb.so*  $${DEPLOY_DIR_LIB}/platforms &&
  deploy.commands += cp -vfa $${QT_HOME}/plugins/platforms/libqminimal.so*  $${DEPLOY_DIR_LIB}/platforms &&
  deploy.commands += cp -vfa $${QT_HOME}/plugins/platforms/libqminimalegl.so*  $${DEPLOY_DIR_LIB}/platforms &&
  deploy.commands += cp -vfa $${QT_HOME}/plugins/platforms/libqoffscreen.so*  $${DEPLOY_DIR_LIB}/platforms &&
  deploy.commands += cp -vfa $${QT_HOME}/plugins/platforms/libqxcb.so*  $${DEPLOY_DIR_LIB}/platforms &&
  deploy.commands += cp -vfa $${QT_HOME}/plugins/platformthemes/libqgtk*.so*  $${DEPLOY_DIR_LIB}/platformthemes &&
  deploy.commands += cp -vfa $${OPENSSL}/libssl.so.1.0.0 $${DEPLOY_DIR_LIB} &&
  deploy.commands += cp -vfa $${OPENSSL}/libcrypto.so.1.0.0 $${DEPLOY_DIR_LIB} &&
  deploy.commands += cp -vfa $${QT_HOME}/lib/libicudata.so*  $${DEPLOY_DIR_LIB} &&
  deploy.commands += cp -vfa $${QT_HOME}/lib/libicui18n.so*  $${DEPLOY_DIR_LIB} &&
  deploy.commands += cp -vfa $${QT_HOME}/lib/libicuuc.so*  $${DEPLOY_DIR_LIB} &&
  deploy.commands += cp -vfa $${QT_HOME}/lib/libQt5Core.so*  $${DEPLOY_DIR_LIB} &&
  deploy.commands += cp -vfa $${QT_HOME}/lib/libQt5DBus.so*  $${DEPLOY_DIR_LIB} &&
  deploy.commands += cp -vfa $${QT_HOME}/lib/libQt5Gui.so*  $${DEPLOY_DIR_LIB} &&
  deploy.commands += cp -vfa $${QT_HOME}/lib/libQt5Network.so*  $${DEPLOY_DIR_LIB} &&
  deploy.commands += cp -vfa $${QT_HOME}/lib/libQt5Svg.so*  $${DEPLOY_DIR_LIB} &&
  deploy.commands += cp -vfa $${QT_HOME}/lib/libQt5Widgets.so*  $${DEPLOY_DIR_LIB} &&
  deploy.commands += cp -vfa $${QT_HOME}/lib/libQt5X11Extras.so*  $${DEPLOY_DIR_LIB} &&
  deploy.commands += cp -vfa $${QT_HOME}/lib/libQt5XcbQpa.so*  $${DEPLOY_DIR_LIB} &&
  deploy.commands += cp -vfa $${QT_HOME}/lib/libQt5Xml.so* $${DEPLOY_DIR_LIB}
}

# Mac specific deploy target
macx {
  DEPLOY_APP=\"$$PWD/../deploy/Little Navconnect.app\"
  DEPLOY_DIR=\"$$PWD/../deploy\"

  deploy.commands = rm -Rfv $${DEPLOY_APP} &&
  deploy.commands += macdeployqt littlenavconnect.app -always-overwrite &&
  deploy.commands += cp -rfv $$OUT_PWD/littlenavconnect.app $${DEPLOY_APP} &&
  deploy.commands += cp -fv $${QT_HOME}/translations/qt_??.qm  $${DEPLOY_APP}/Contents/MacOS &&
  deploy.commands += cp -fv $${QT_HOME}/translations/qt_??_??.qm  $${DEPLOY_APP}/Contents/MacOS &&
  deploy.commands += cp -fv $${QT_HOME}/translations/qtbase*.qm  $${DEPLOY_APP}/Contents/MacOS &&
  deploy.commands += cp -fv $$PWD/LICENSE.txt $${DEPLOY_DIR} &&
  deploy.commands += cp -fv $$PWD/README.txt $${DEPLOY_DIR}/README-LittleNavconnect.txt &&
  deploy.commands += cp -fv $$PWD/CHANGELOG.txt $${DEPLOY_DIR}/CHANGELOG-LittleNavconnect.txt
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

  deploy.commands = rmdir /s /q $${DEPLOY_DIR_WIN} &
  deploy.commands += mkdir $${DEPLOY_DIR_WIN}\\translations &&
  deploy.commands += xcopy $${WINOUT_PWD}\\$${CONF_TYPE}\\littlenavconnect.exe $${DEPLOY_DIR_WIN} &&
  deploy.commands += xcopy $${WINPWD}\\CHANGELOG.txt $${DEPLOY_DIR_WIN} &&
  deploy.commands += xcopy $${WINPWD}\\README.txt $${DEPLOY_DIR_WIN} &&
  deploy.commands += xcopy $${WINPWD}\\LICENSE.txt $${DEPLOY_DIR_WIN} &&
  deploy.commands += xcopy $${WINPWD}\\*.qm $${DEPLOY_DIR_WIN}\\translations &&
  deploy.commands += xcopy $${WINPWD}\\..\\atools\\*.qm $${DEPLOY_DIR_WIN}\\translations &&
  deploy.commands += xcopy $${WINPWD}\\littlenavconnect.exe.simconnect $${DEPLOY_DIR_WIN} &&
  deploy.commands += xcopy $${OPENSSL}\\bin\\libeay32.dll $${DEPLOY_DIR_WIN} &&
  deploy.commands += xcopy $${OPENSSL}\\bin\\ssleay32.dll $${DEPLOY_DIR_WIN} &&
  deploy.commands += xcopy $${OPENSSL}\\libssl32.dll $${DEPLOY_DIR_WIN} &&
  deploy.commands += xcopy $${QT_HOME}\\bin\\libgcc*.dll $${DEPLOY_DIR_WIN} &&
  deploy.commands += xcopy $${QT_HOME}\\bin\\libstdc*.dll $${DEPLOY_DIR_WIN} &&
  deploy.commands += xcopy $${QT_HOME}\\bin\\libwinpthread*.dll $${DEPLOY_DIR_WIN} &&
  deploy.commands += xcopy /i /s /e /f /y $${WINPWD}\\help $${DEPLOY_DIR_WIN}\\help &&
  deploy.commands += $${QT_HOME}\\bin\\windeployqt $${WINDEPLOY_FLAGS} $${DEPLOY_DIR_WIN}
}

QMAKE_EXTRA_TARGETS += deploy

first.depends = $(first) copydata
QMAKE_EXTRA_TARGETS += first copydata

clean.depends = $(clean) cleandata
QMAKE_EXTRA_TARGETS += clean cleandata


TRANSLATIONS = littlenavconnect_fr.ts \
               littlenavconnect_it.ts \
               littlenavconnect_nl.ts \
               littlenavconnect_de.ts \
               littlenavconnect_es.ts \
               littlenavconnect_pt_BR.ts
