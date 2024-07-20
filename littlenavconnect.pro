#*****************************************************************************
# Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
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

# =============================================================================
# Set these environment variables for configuration - do not change this .pro file
# =============================================================================
#
# ATOOLS_INC_PATH
# Optional. Path to atools include. Default is "../atools/src" if not set.
# Also reads *.qm translation files from "$ATOOLS_INC_PATH/..".
#
# ATOOLS_LIB_PATH
# Optional. Path to atools static library. Default is "../build-atools-$${CONF_TYPE}"
# ("../build-atools-$${CONF_TYPE}/$${CONF_TYPE}" on Windows) if not set.
#
# ATOOLS_GIT_PATH
# Optional. Path to GIT executable. Revision will be set to "UNKNOWN" if not set.
# Uses "git" on macOS and Linux as default if not set.
# Example: "C:\Git\bin\git"
#
# ATOOLS_SIMCONNECT_PATH
# Optional. Path to SimConnect SDK. SimConnect support will be omitted in build if not set.
# Example: "C:\Program Files (x86)\Microsoft Games\Microsoft Flight Simulator X SDK\SDK\Core Utilities Kit\SimConnect SDK"
#
# DEPLOY_BASE
# Optional. Target folder for "make deploy". Default is "../deploy" plus project name ($$TARGET_NAME).
#
# ATOOLS_QUIET
# Optional. Set this to "true" to avoid qmake messages.
#
# =============================================================================
# End of configuration documentation
# =============================================================================

# Define program version here VERSION_NUMBER_TODO
VERSION_NUMBER=3.0.5

QT += core gui xml network svg

CONFIG += build_all c++14
CONFIG -= debug_and_release debug_and_release_target

TARGET = littlenavconnect
TEMPLATE = app

win32 { contains(QT_ARCH, i386) { WINARCH = win32 } else { WINARCH = win64 } }

TARGET_NAME=Little Navconnect

# =======================================================================
# Copy environment variables into qmake variables

ATOOLS_INC_PATH=$$(ATOOLS_INC_PATH)
ATOOLS_LIB_PATH=$$(ATOOLS_LIB_PATH)
GIT_PATH=$$(ATOOLS_GIT_PATH)
SIMCONNECT_PATH_WIN32=$$(ATOOLS_SIMCONNECT_PATH_WIN32)
SIMCONNECT_PATH_WIN64=$$(ATOOLS_SIMCONNECT_PATH_WIN64)
DEPLOY_BASE=$$(DEPLOY_BASE)
QUIET=$$(ATOOLS_QUIET)

# =======================================================================
# Fill defaults for unset

CONFIG(debug, debug|release) : CONF_TYPE=debug
CONFIG(release, debug|release) : CONF_TYPE=release

isEmpty(DEPLOY_BASE) : DEPLOY_BASE=$$PWD/../deploy

isEmpty(ATOOLS_INC_PATH) : ATOOLS_INC_PATH=$$PWD/../atools/src
isEmpty(ATOOLS_LIB_PATH) : ATOOLS_LIB_PATH=$$PWD/../build-atools-$$CONF_TYPE

# =======================================================================
# Set compiler flags and paths

QMAKE_CXXFLAGS += -Wno-pragmas -Wno-unknown-warning -Wno-unknown-warning-option

# No crash handler on Linux and macOS
unix : ATOOLS_DISABLE_CRASHHANDLER = true
isEqual(ATOOLS_NO_CRASHHANDLER, "true") : ATOOLS_DISABLE_CRASHHANDLER = true

unix:!macx {
  isEmpty(GIT_PATH) : GIT_PATH=git

  QMAKE_LFLAGS += -no-pie

  # Makes the shell script and setting LD_LIBRARY_PATH redundant
  QMAKE_RPATHDIR=.
  QMAKE_RPATHDIR+=./lib
}

win32 {
  contains(QT_ARCH, i386) {
  # FSX or P3D
    WINARCH = win32
    !isEmpty(SIMCONNECT_PATH_WIN32) {
      DEFINES += SIMCONNECT_BUILD_WIN32 WINARCH32
      INCLUDEPATH += $$SIMCONNECT_PATH_WIN32"\inc"
      LIBS += $$SIMCONNECT_PATH_WIN32"\lib\SimConnect.lib"
    }
    ATOOLS_DISABLE_CRASHHANDLER = true
  } else {
  # MSFS
    WINARCH = win64
    !isEmpty(SIMCONNECT_PATH_WIN64) {
      DEFINES += SIMCONNECT_BUILD_WIN64 WINARCH64
      INCLUDEPATH += $$SIMCONNECT_PATH_WIN64"\include"
      LIBS += $$SIMCONNECT_PATH_WIN64"\lib\SimConnect.lib"
    }
  }

  WINDEPLOY_FLAGS = --compiler-runtime
  CONFIG(debug, debug|release) : WINDEPLOY_FLAGS += --debug

  DEFINES += _USE_MATH_DEFINES

  LIBS += -L$$ATOOLS_LIB_PATH -latools -lz
}

macx {
  isEmpty(GIT_PATH) : GIT_PATH=git
}

isEmpty(GIT_PATH) {
  GIT_REVISION=UNKNOWN
  GIT_REVISION_FULL=UNKNOWN
} else {
  GIT_REVISION=$$system('$$GIT_PATH' rev-parse --short HEAD)
  GIT_REVISION_FULL=$$system('$$GIT_PATH' rev-parse HEAD)
}

LIBS += -L$$ATOOLS_LIB_PATH -latools

# Cpptrace ==========================
!isEqual(ATOOLS_DISABLE_CRASHHANDLER, "true") {
  DEFINES += CPPTRACE_STATIC_DEFINE
  win32 : LIBS += -L$$PWD/../cpptrace-$$CONF_TYPE-$$WINARCH/lib -lcpptrace -ldbghelp -ldwarf -lz -lzstd
  unix:!macx : LIBS += -L$$PWD/../cpptrace-$$CONF_TYPE/lib -lcpptrace -ldwarf -lz -lzstd
  CONFIG += force_debug_info
} else {
  DEFINES += DISABLE_CRASHHANDLER
}

PRE_TARGETDEPS += $$ATOOLS_LIB_PATH/libatools.a
DEPENDPATH += $$ATOOLS_INC_PATH
INCLUDEPATH += $$PWD/src $$ATOOLS_INC_PATH
DEFINES += VERSION_NUMBER_LITTLENAVCONNECT='\\"$$VERSION_NUMBER\\"'
DEFINES += GIT_REVISION_LITTLENAVCONNECT='\\"$$GIT_REVISION\\"'
DEFINES += QT_NO_CAST_FROM_BYTEARRAY
DEFINES += QT_NO_CAST_TO_ASCII

# =======================================================================
# Include build_options.pro with additional variables

exists($$PWD/../build_options.pro) {
   include($$PWD/../build_options.pro)

   !isEqual(QUIET, "true") {
     message($$PWD/../build_options.pro found.)
   }
} else {
   !isEqual(QUIET, "true") {
     message($$PWD/../build_options.pro not found.)
   }
}

# =======================================================================
# Print values when running qmake

!isEqual(QUIET, "true") {
message(-----------------------------------)
message(VERSION_NUMBER: $$VERSION_NUMBER)
message(GIT_REVISION: $$GIT_REVISION)
message(GIT_REVISION_FULL: $$GIT_REVISION_FULL)
message(GIT_PATH: $$GIT_PATH)
message(WINARCH: $$WINARCH)
message(ATOOLS_INC_PATH: $$ATOOLS_INC_PATH)
message(ATOOLS_LIB_PATH: $$ATOOLS_LIB_PATH)
message(SIMCONNECT_PATH_WIN32: $$SIMCONNECT_PATH_WIN32)
message(SIMCONNECT_PATH_WIN64: $$SIMCONNECT_PATH_WIN64)
message(DEPLOY_BASE: $$DEPLOY_BASE)
message(DEFINES: $$DEFINES)
message(INCLUDEPATH: $$INCLUDEPATH)
message(LIBS: $$LIBS)
message(TARGET_NAME: $$TARGET_NAME)
message(QT_INSTALL_PREFIX: $$[QT_INSTALL_PREFIX])
message(QT_INSTALL_LIBS: $$[QT_INSTALL_LIBS])
message(QT_INSTALL_PLUGINS: $$[QT_INSTALL_PLUGINS])
message(QT_INSTALL_TRANSLATIONS: $$[QT_INSTALL_TRANSLATIONS])
message(QT_INSTALL_BINS: $$[QT_INSTALL_BINS])
message(CONFIG: $$CONFIG)
message(QT: $$QT)
message(-----------------------------------)
}

# =====================================================================
# Files

SOURCES +=\
  src/main.cpp \
  src/mainwindow.cpp \
  src/optionsdialog.cpp

HEADERS  += \
  src/constants.h \
  src/mainwindow.h \
  src/optionsdialog.h

FORMS    += mainwindow.ui \
  optionsdialog.ui

RESOURCES += \
  littlenavconnect.qrc

ICON = resources/icons/littlenavconnect.icns

TRANSLATIONS = \
  littlenavconnect_fr.ts \
  littlenavconnect_it.ts \
  littlenavconnect_de.ts \
  littlenavconnect_pt_BR.ts

# littlenavconnect_nl.ts
# littlenavconnect_zh.ts
# littlenavconnect_es.ts

OTHER_FILES += \
  $$files(desktop/*, true) \
  $$files(help/*, true) \
  $$files(simconnect/*, true) \
  .gitignore \
  *.ts \
  BUILD.txt \
  CHANGELOG.txt \
  LICENSE.txt \
  README.txt \
  htmltidy.cfg \
  uncrustify.cfg

# =====================================================================
# Local deployment commands for development

# Create additional makefile targets to copy help files
unix:!macx {
  copydata.commands = cp -avfu $$PWD/help $$OUT_PWD &&
  copydata.commands += mkdir -p $$OUT_PWD/translations &&
  copydata.commands += cp -avfu $$PWD/*.qm $$OUT_PWD/translations &&
  copydata.commands += cp -avfu $$ATOOLS_INC_PATH/../*.qm $$OUT_PWD/translations &&
  copydata.commands += cp -vf $$PWD/desktop/littlenavconnect*.sh $$OUT_PWD &&
  copydata.commands += chmod -v a+x $$OUT_PWD/littlenavconnect*.sh
}

# Mac OS X - Copy help and Marble plugins and data
macx {
  copydata.commands += cp -Rv $$PWD/help $$OUT_PWD/littlenavconnect.app/Contents/MacOS &&
  copydata.commands += cp -vf $$PWD/*.qm $$OUT_PWD/littlenavconnect.app/Contents/MacOS &&
  copydata.commands += cp -vf $$ATOOLS_INC_PATH/../*.qm $$OUT_PWD/littlenavconnect.app/Contents/MacOS
}

# =====================================================================
# Deployment commands

# Linux specific deploy target
unix:!macx {
  DEPLOY_DIR=\"$$DEPLOY_BASE/$$TARGET_NAME\"
  DEPLOY_DIR_LIB=\"$$DEPLOY_BASE/$$TARGET_NAME/lib\"

  deploy.commands = rm -Rfv $$DEPLOY_DIR &&
  deploy.commands += mkdir -pv $$DEPLOY_DIR_LIB &&
  deploy.commands += mkdir -pv $$DEPLOY_DIR_LIB/iconengines &&
  deploy.commands += mkdir -pv $$DEPLOY_DIR_LIB/imageformats &&
  deploy.commands += mkdir -pv $$DEPLOY_DIR_LIB/platforms &&
  deploy.commands += mkdir -pv $$DEPLOY_DIR_LIB/platformthemes &&
  deploy.commands += echo $$VERSION_NUMBER > $$DEPLOY_DIR/version.txt &&
  deploy.commands += echo $$GIT_REVISION_FULL > $$DEPLOY_DIR/revision.txt &&
  deploy.commands += cp -Rvf $$OUT_PWD/littlenavconnect $$DEPLOY_DIR &&
  deploy.commands += cp -Rvf $$OUT_PWD/help $$DEPLOY_DIR &&
  deploy.commands += cp -Rvf $$OUT_PWD/translations $$DEPLOY_DIR &&
  deploy.commands += cp -vf $$PWD/desktop/qt.conf $$DEPLOY_DIR &&
  deploy.commands += cp -vf $$PWD/CHANGELOG.txt $$DEPLOY_DIR &&
  deploy.commands += cp -vf $$PWD/README.txt $$DEPLOY_DIR &&
  deploy.commands += cp -vf $$PWD/LICENSE.txt $$DEPLOY_DIR &&
  deploy.commands += cp -vf $$PWD/resources/icons/navconnect.svg $$DEPLOY_DIR/littlenavconnect.svg &&
  deploy.commands += cp -vf \"$$PWD/desktop/Little Navconnect.desktop\" $$DEPLOY_DIR &&
  deploy.commands += cp -vfa $$[QT_INSTALL_TRANSLATIONS]/qt_??.qm  $$DEPLOY_DIR/translations &&
  deploy.commands += cp -vfa $$[QT_INSTALL_TRANSLATIONS]/qt_??_??.qm  $$DEPLOY_DIR/translations &&
  deploy.commands += cp -vfa $$[QT_INSTALL_TRANSLATIONS]/qtbase*.qm  $$DEPLOY_DIR/translations &&
  deploy.commands += cp -vfa $$[QT_INSTALL_PLUGINS]/iconengines/libqsvgicon.so*  $$DEPLOY_DIR_LIB/iconengines &&
  deploy.commands += cp -vfa $$[QT_INSTALL_PLUGINS]/imageformats/libqgif.so*  $$DEPLOY_DIR_LIB/imageformats &&
  deploy.commands += cp -vfa $$[QT_INSTALL_PLUGINS]/imageformats/libqjpeg.so*  $$DEPLOY_DIR_LIB/imageformats &&
  deploy.commands += cp -vfa $$[QT_INSTALL_PLUGINS]/imageformats/libqsvg.so*  $$DEPLOY_DIR_LIB/imageformats &&
  deploy.commands += cp -vfa $$[QT_INSTALL_PLUGINS]/platforms/libqeglfs.so*  $$DEPLOY_DIR_LIB/platforms &&
  deploy.commands += cp -vfa $$[QT_INSTALL_PLUGINS]/platforms/libqlinuxfb.so*  $$DEPLOY_DIR_LIB/platforms &&
  deploy.commands += cp -vfa $$[QT_INSTALL_PLUGINS]/platforms/libqminimal.so*  $$DEPLOY_DIR_LIB/platforms &&
  deploy.commands += cp -vfa $$[QT_INSTALL_PLUGINS]/platforms/libqminimalegl.so*  $$DEPLOY_DIR_LIB/platforms &&
  deploy.commands += cp -vfa $$[QT_INSTALL_PLUGINS]/platforms/libqoffscreen.so*  $$DEPLOY_DIR_LIB/platforms &&
  deploy.commands += cp -vfa $$[QT_INSTALL_PLUGINS]/platforms/libqxcb.so*  $$DEPLOY_DIR_LIB/platforms &&
  deploy.commands += cp -vfa $$[QT_INSTALL_PLUGINS]/platformthemes/libqgtk*.so*  $$DEPLOY_DIR_LIB/platformthemes &&
  deploy.commands += cp -vfa $$[QT_INSTALL_LIBS]/libicudata.so*  $$DEPLOY_DIR_LIB &&
  deploy.commands += cp -vfa $$[QT_INSTALL_LIBS]/libicui18n.so*  $$DEPLOY_DIR_LIB &&
  deploy.commands += cp -vfa $$[QT_INSTALL_LIBS]/libicuuc.so*  $$DEPLOY_DIR_LIB &&
  deploy.commands += cp -vfa $$[QT_INSTALL_LIBS]/libQt5Core.so*  $$DEPLOY_DIR_LIB &&
  deploy.commands += cp -vfa $$[QT_INSTALL_LIBS]/libQt5DBus.so*  $$DEPLOY_DIR_LIB &&
  deploy.commands += cp -vfa $$[QT_INSTALL_LIBS]/libQt5Gui.so*  $$DEPLOY_DIR_LIB &&
  deploy.commands += cp -vfa $$[QT_INSTALL_LIBS]/libQt5Network.so*  $$DEPLOY_DIR_LIB &&
  deploy.commands += cp -vfa $$[QT_INSTALL_LIBS]/libQt5Svg.so*  $$DEPLOY_DIR_LIB &&
  deploy.commands += cp -vfa $$[QT_INSTALL_LIBS]/libQt5Widgets.so*  $$DEPLOY_DIR_LIB &&
  deploy.commands += cp -vfa $$[QT_INSTALL_LIBS]/libQt5X11Extras.so*  $$DEPLOY_DIR_LIB &&
  deploy.commands += cp -vfa $$[QT_INSTALL_LIBS]/libQt5XcbQpa.so*  $$DEPLOY_DIR_LIB &&
  deploy.commands += cp -vfa $$[QT_INSTALL_LIBS]/libQt5Xml.so* $$DEPLOY_DIR_LIB &&
  deploy.commands += rm -fv $$DEPLOY_DIR_LIB/lib*.so.*.debug $$DEPLOY_DIR_LIB/*/lib*.so.*.debug
}

# Mac specific deploy target
macx {
  DEPLOY_APP=\"$$DEPLOY_BASE/$${TARGET_NAME}.app\"
  DEPLOY_DIR=\"$$DEPLOY_BASE/\"

  deploy.commands = rm -Rfv $$DEPLOY_APP &&
  deploy.commands += $$[QT_INSTALL_BINS]/macdeployqt littlenavconnect.app -always-overwrite &&
  deploy.commands += cp -rfv $$OUT_PWD/littlenavconnect.app $$DEPLOY_APP &&
  deploy.commands += cp -fv $$[QT_INSTALL_TRANSLATIONS]/qt_??.qm  $$DEPLOY_APP/Contents/MacOS &&
  deploy.commands += cp -fv $$[QT_INSTALL_TRANSLATIONS]/qt_??_??.qm  $$DEPLOY_APP/Contents/MacOS &&
  deploy.commands += cp -fv $$[QT_INSTALL_TRANSLATIONS]/qtbase*.qm  $$DEPLOY_APP/Contents/MacOS &&
  deploy.commands += cp -fv $$PWD/build/mac/Info.plist $$DEPLOY_APP/Contents &&
  deploy.commands += echo $$VERSION_NUMBER > $$DEPLOY_DIR/version-LittleNavconnect.txt &&
  deploy.commands += echo $$GIT_REVISION_FULL > $$DEPLOY_DIR/revision-LittleNavconnect.txt &&
  deploy.commands += cp -fv $$PWD/LICENSE.txt $$DEPLOY_DIR &&
  deploy.commands += cp -fv $$PWD/README.txt $$DEPLOY_DIR/README-LittleNavconnect.txt &&
  deploy.commands += cp -fv $$PWD/CHANGELOG.txt $$DEPLOY_DIR/CHANGELOG-LittleNavconnect.txt
}


# Windows specific deploy target
win32 {
  defineReplace(p){return ($$shell_quote($$shell_path($$1)))}
  RC_ICONS = resources/icons/navconnect.ico

  WIN_TARGET_NAME="$$TARGET_NAME $$WINARCH"

  deploy.commands = rmdir /s /q $$p($$DEPLOY_BASE/$$WIN_TARGET_NAME) &
  deploy.commands += mkdir $$p($$DEPLOY_BASE/$$WIN_TARGET_NAME/translations) &&
  deploy.commands += echo $$WINARCH-$$VERSION_NUMBER > $$p($$DEPLOY_BASE/$$WIN_TARGET_NAME/version.txt) &&
  deploy.commands += echo $$GIT_REVISION_FULL > $$p($$DEPLOY_BASE/$$WIN_TARGET_NAME/revision.txt) &&
  deploy.commands += xcopy $$p($$OUT_PWD/littlenavconnect.exe) $$p($$DEPLOY_BASE/$$WIN_TARGET_NAME) &&
  deploy.commands += xcopy $$p($$PWD/CHANGELOG.txt) $$p($$DEPLOY_BASE/$$WIN_TARGET_NAME) &&
  deploy.commands += xcopy $$p($$PWD/README.txt) $$p($$DEPLOY_BASE/$$WIN_TARGET_NAME) &&
  deploy.commands += xcopy $$p($$PWD/LICENSE.txt) $$p($$DEPLOY_BASE/$$WIN_TARGET_NAME) &&
  deploy.commands += xcopy $$p($$PWD/*.qm) $$p($$DEPLOY_BASE/$$WIN_TARGET_NAME/translations) &&
  deploy.commands += xcopy $$p($$ATOOLS_INC_PATH/../*.qm) $$p($$DEPLOY_BASE/$$WIN_TARGET_NAME/translations) &&
  deploy.commands += xcopy /i /s /e /f /y $$p($$PWD/simconnect) $$p($$DEPLOY_BASE/$$WIN_TARGET_NAME/simconnect) &&
  contains(QT_ARCH, i386) { # 32 Bit build
    deploy.commands += move /Y $$p($$DEPLOY_BASE/$$WIN_TARGET_NAME/simconnect/SimConnect.dll) $$p($$DEPLOY_BASE/$$WIN_TARGET_NAME) &&
  } else { # 64 Bit build
    deploy.commands += del /f /q $$p($$DEPLOY_BASE/$$WIN_TARGET_NAME/simconnect/SimConnect.dll) &&
    deploy.commands += del /f /q $$p($$DEPLOY_BASE/$$WIN_TARGET_NAME/simconnect/simconnect.manifest) &&
    deploy.commands += xcopy $$p($$SIMCONNECT_PATH_WIN64/lib/SimConnect.dll) $$p($$DEPLOY_BASE/$$WIN_TARGET_NAME) &&
  }
  deploy.commands += xcopy $$p($$[QT_INSTALL_BINS]/libgcc*.dll) $$p($$DEPLOY_BASE/$$WIN_TARGET_NAME) &&
  deploy.commands += xcopy $$p($$[QT_INSTALL_BINS]/libstdc*.dll) $$p($$DEPLOY_BASE/$$WIN_TARGET_NAME) &&
  deploy.commands += xcopy $$p($$[QT_INSTALL_BINS]/libwinpthread*.dll) $$p($$DEPLOY_BASE/$$WIN_TARGET_NAME) &&
  deploy.commands += xcopy /i /s /e /f /y $$p($$PWD/help) $$p($$DEPLOY_BASE/$$WIN_TARGET_NAME/help) &&
  deploy.commands += $$p($$[QT_INSTALL_BINS]/windeployqt) $$WINDEPLOY_FLAGS $$p($$DEPLOY_BASE/$$WIN_TARGET_NAME)
}

# =====================================================================
# Additional targets

# Need to copy data when compiling
all.depends = copydata

# Deploy needs compiling before
deploy.depends = all

QMAKE_EXTRA_TARGETS += deploy copydata all
