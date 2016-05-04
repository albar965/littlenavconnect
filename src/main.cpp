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
#include <QApplication>
#include <logging/logginghandler.h>
#include <logging/loggingutil.h>
#include <settings/settings.h>

int main(int argc, char *argv[])
{
  // Initialize the resources from atools static library
  Q_INIT_RESOURCE(atools);

  QApplication app(argc, argv);
  QApplication::setWindowIcon(QIcon(":/littlenavconnect/resources/icons/navroute.svg"));
  QCoreApplication::setApplicationName("Little Navconnect");
  QCoreApplication::setOrganizationName("ABarthel");
  QCoreApplication::setOrganizationDomain("abarthel.org");
  QCoreApplication::setApplicationVersion("0.5.0.develop");

  using atools::logging::LoggingHandler;
  using atools::logging::LoggingUtil;
  using atools::settings::Settings;

  // Initialize logging and force logfiles into the system or user temp directory
  LoggingHandler::initializeForTemp(atools::settings::Settings::getOverloadedPath(
                                      ":/littlenavconnect/resources/config/logging.cfg"));

  // Print some information which can be useful for debugging
  LoggingUtil::logSystemInformation();
  LoggingUtil::logStandardPaths();
  Settings::logSettingsInformation();

  MainWindow w;
  w.show();

  return app.exec();
}
