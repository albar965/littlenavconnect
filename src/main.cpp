/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
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
#include "fs/sc/simconnectdata.h"
#include "fs/sc/simconnectreply.h"
#include "gui/application.h"
#include "gui/translator.h"
#include "navservercommon.h"

#if defined(Q_OS_WIN32)
#include <QSharedMemory>
#include <QMessageBox>

#endif

using atools::gui::Application;
using atools::logging::LoggingHandler;
using atools::logging::LoggingUtil;
using atools::settings::Settings;
using atools::gui::Translator;
using atools::logging::LoggingHandler;
using atools::logging::LoggingUtil;
using atools::settings::Settings;

int main(int argc, char *argv[])
{
  // Initialize the resources from atools static library
  Q_INIT_RESOURCE(atools);

  // Needed to send SimConnectData through queued connections
  qRegisterMetaType<atools::fs::sc::SimConnectData>();
  qRegisterMetaType<atools::fs::sc::SimConnectReply>();
  qRegisterMetaType<atools::fs::sc::WeatherRequest>();

  using atools::gui::Application;
  Application app(argc, argv);
  Application::setWindowIcon(QIcon(":/littlenavconnect/resources/icons/navconnect.svg"));
  Application::setApplicationName("Little Navconnect");
  Application::setOrganizationName("ABarthel");
  Application::setOrganizationDomain("abarthel.org");
  Application::setApplicationVersion("1.2.5.devel");
  Application::setEmailAddresses({"albar965@mailbox.org", "albar965@t-online.de"});

  // Initialize logging and force logfiles into the system or user temp directory
  LoggingHandler::initializeForTemp(atools::settings::Settings::getOverloadedPath(
                                      ":/littlenavconnect/resources/config/logging.cfg"));

  Application::addReportPath(QObject::tr("Log files:"), LoggingHandler::getLogFiles());

  Application::addReportPath(QObject::tr("Configuration:"), {Settings::getFilename()});

  // Print some information which can be useful for debugging
  LoggingUtil::logSystemInformation();
  LoggingUtil::logStandardPaths();
  Settings::logSettingsInformation();

  // Load local and Qt system translations from various places
  Translator::load(Settings::instance().valueStr(SETTINGS_OPTIONS_LANGUAGE, QString()));

#if defined(Q_OS_WIN32)
  // Detect other running application instance - this is unsafe on Unix since shm can remain after crashes
  QSharedMemory shared("ed1b2f62-a6b3-8c64-09b4-e4daa232ecf4"); // generated GUID
  if(!shared.create(512, QSharedMemory::ReadWrite))
  {
    QMessageBox::critical(nullptr, QObject::tr("%1 - Error").arg(QApplication::applicationName()),
                          QObject::tr("%1 is already running.").arg(QApplication::applicationName()));
    return 1;
  }
#endif

  MainWindow mainWindow;

  mainWindow.show();

  return app.exec();
}
