/*****************************************************************************
* Copyright 2015-2024 Alexander Barthel alex@littlenavmap.org
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#include "mainwindow.h"

#include "logging/logginghandler.h"
#include "logging/loggingutil.h"
#include "settings/settings.h"
#include "fs/sc/simconnecttypes.h"
#include "gui/application.h"
#include "gui/translator.h"
#include "constants.h"
#include "atools.h"
#include "geo/calculations.h"
#include "fs/fspaths.h"
#include "exception.h"
#include "gui/dialog.h"

#include <QSslSocket>
#include <QStyleFactory>
#include <QSharedMemory>

using atools::gui::Application;
using atools::logging::LoggingHandler;
using atools::logging::LoggingUtil;
using atools::settings::Settings;
using atools::gui::Translator;

int main(int argc, char *argv[])
{
  // Initialize the resources from atools static library
  Q_INIT_RESOURCE(atools);

  // Needed to send SimConnectData through queued connections
  atools::fs::sc::registerMetaTypes();
  atools::geo::registerMetaTypes();
  atools::fs::FsPaths::intitialize();

  int retval = 0;

  Settings::setOrganizationName(lnc::OPTIONS_APPLICATION_ORGANIZATION);
  Settings::setApplicationName(lnc::OPTIONS_APPLICATION);

  try
  {
    using atools::gui::Application;
    Application app(argc, argv);
    Application::setWindowIcon(QIcon(":/littlenavconnect/resources/icons/navconnect.svg"));
    Application::setApplicationName(lnc::OPTIONS_APPLICATION);
    Application::setOrganizationName(lnc::OPTIONS_APPLICATION_ORGANIZATION);
    Application::setOrganizationDomain(lnc::OPTIONS_APPLICATION_DOMAIN);

    Application::setApplicationVersion(VERSION_NUMBER_LITTLENAVCONNECT);
    Application::setEmailAddresses({"alex@littlenavmap.org"});

    // Initialize logging and force logfiles into the system or user temp directory
    LoggingHandler::initializeForTemp(atools::settings::Settings::getOverloadedPath(":/littlenavconnect/resources/config/logging.cfg"));

    Application::addReportPath(QObject::tr("Log files:"), LoggingHandler::getLogFiles());

    Application::addReportPath(QObject::tr("Configuration:"), {Settings::getFilename()});

    // Disable tooltip effects since these do not work well with tooltip updates while displaying
    QApplication::setEffectEnabled(Qt::UI_FadeTooltip, false);
    QApplication::setEffectEnabled(Qt::UI_AnimateTooltip, false);

    // Avoid closing if options dialog is closed with main window hidden to tray
    QApplication::setQuitOnLastWindowClosed(false);

#if QT_VERSION > QT_VERSION_CHECK(5, 10, 0)
    QApplication::setAttribute(Qt::AA_DisableWindowContextHelpButton);
#endif

    // Print some information which can be useful for debugging
    Settings::logMessages();
    LoggingUtil::logSystemInformation();
    LoggingUtil::logStandardPaths();

    qInfo().noquote().nospace() << "atools revision " << atools::gitRevision() << " "
                                << Application::applicationName() << " revision " << GIT_REVISION_LITTLENAVCONNECT;

    qInfo() << "SSL supported" << QSslSocket::supportsSsl()
            << "build library" << QSslSocket::sslLibraryBuildVersionString()
            << "library" << QSslSocket::sslLibraryVersionString();

    qInfo() << "Available styles" << QStyleFactory::keys();

    // Load simulator paths =================================
    atools::fs::FsPaths::loadAllPaths();
    atools::fs::FsPaths::logAllPaths();

    // Load local and Qt system translations from various places
    Translator::load(Settings::instance().valueStr(lnc::SETTINGS_OPTIONS_LANGUAGE, QString()));

    // Detect other running application instance - this is unsafe on Unix since shared memory can remain after crashes
    QSharedMemory shared("ed1b2f62-a6b3-8c64-09b4-e4daa232ecf4"); // generated GUID
    if(!shared.create(64, QSharedMemory::ReadOnly))
    {
      shared.detach();
      atools::gui::Dialog::critical(nullptr, QObject::tr("%1 is already running.").arg(QCoreApplication::applicationName()));
      return 1;
    }

    // Put in separate block to destroy main window before shutting down logging
    MainWindow mainWindow;
    mainWindow.showInitial();
    retval = QCoreApplication::exec();
  }
  catch(atools::Exception& e)
  {
    ATOOLS_HANDLE_EXCEPTION(e);
    // Does not return in case of fatal error
  }
  catch(...)
  {
    ATOOLS_HANDLE_UNKNOWN_EXCEPTION;
    // Does not return in case of fatal error
  }

  qDebug() << "app.exec() done, retval is" << retval << (retval == 0 ? "(ok)" : "(error)");
  qInfo() << "About to shut down logging";
  atools::logging::LoggingHandler::shutdown();

  return retval;
}
