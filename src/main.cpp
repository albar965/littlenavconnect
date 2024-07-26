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

#include "atools.h"
#include "constants.h"
#include "exception.h"
#include "fs/fspaths.h"
#include "fs/sc/simconnecttypes.h"
#include "geo/calculations.h"
#include "gui/application.h"
#include "gui/translator.h"
#include "logging/logginghandler.h"
#include "logging/loggingutil.h"
#include "settings/settings.h"
#include "util/crashhandler.h"

#include <QSslSocket>
#include <QStyleFactory>
#include <QFile>
#include <QCommandLineParser>

using atools::gui::Application;
using atools::logging::LoggingHandler;
using atools::logging::LoggingUtil;
using atools::settings::Settings;
using atools::gui::Translator;
using atools::gui::Application;

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
    Application app(argc, argv);

    Application::setWindowIcon(QIcon(":/littlenavconnect/resources/icons/navconnect.svg"));
    Application::setApplicationName(lnc::OPTIONS_APPLICATION);
    Application::setOrganizationName(lnc::OPTIONS_APPLICATION_ORGANIZATION);
    Application::setOrganizationDomain(lnc::OPTIONS_APPLICATION_DOMAIN);

    Application::setApplicationVersion(VERSION_NUMBER_LITTLENAVCONNECT);
    Application::setEmailAddresses({"alex@littlenavmap.org"});

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption saveReplayOpt({"s", lnc::STARTUP_COMMAND_SAVE_REPLAY}, QObject::tr("Save replay data to <file>."),
                                     QObject::tr("file"));
    parser.addOption(saveReplayOpt);

    QCommandLineOption loadReplayOpt({"l", lnc::STARTUP_COMMAND_LOAD_REPLAY}, QObject::tr("Load replay data from <file>."),
                                     QObject::tr("file"));
    parser.addOption(loadReplayOpt);

    QCommandLineOption replaySpeedOpt({"r", lnc::STARTUP_COMMAND_REPLAY_SPEED}, QObject::tr("Use speed factor <speed> for replay."),
                                      QObject::tr("speed"));
    parser.addOption(replaySpeedOpt);

    QCommandLineOption replayWhazzupOpt({"w", lnc::STARTUP_COMMAND_WRITE_WHAZZUP},
                                        QObject::tr("Update whazzup file <file> using VATSIM format during replay."),
                                        QObject::tr("file"));
    parser.addOption(replayWhazzupOpt);

    QCommandLineOption replayWhazzupUpdateOpt({"z", lnc::STARTUP_COMMAND_WRITE_WHAZZUP_SPEED},
                                              QObject::tr("Update whazzup file every <seconds> during replay."),
                                              QObject::tr("seconds"));
    parser.addOption(replayWhazzupUpdateOpt);

    QCommandLineOption showReplay({"g", lnc::STARTUP_COMMAND_REPLAY_GUI}, QObject::tr("Show replay menu items."));
    parser.addOption(showReplay);

    QCommandLineOption quitCmd({"q", lnc::STARTUP_COMMAND_QUIT}, QObject::tr("Quit an already running instance."));
    parser.addOption(quitCmd);

    // Process the actual command line arguments given by the user
    parser.process(*QCoreApplication::instance());

    Application::addStartupOptionStrIf(lnc::STARTUP_COMMAND_SAVE_REPLAY, parser.value(saveReplayOpt));
    Application::addStartupOptionStrIf(lnc::STARTUP_COMMAND_LOAD_REPLAY, parser.value(loadReplayOpt));
    Application::addStartupOptionStrIf(lnc::STARTUP_COMMAND_REPLAY_SPEED, parser.value(replaySpeedOpt));
    Application::addStartupOptionStrIf(lnc::STARTUP_COMMAND_WRITE_WHAZZUP, parser.value(replayWhazzupOpt));
    Application::addStartupOptionStrIf(lnc::STARTUP_COMMAND_WRITE_WHAZZUP_SPEED, parser.value(replayWhazzupUpdateOpt));
    Application::addStartupOptionBoolIf(lnc::STARTUP_COMMAND_REPLAY_GUI, parser.isSet(showReplay));
    Application::addStartupOptionBoolIf(lnc::STARTUP_COMMAND_QUIT, parser.isSet(quitCmd));

    if(!MainWindow::initDataExchange())
    {
      // Initialize logging and force logfiles into the system or user temp directory
      LoggingHandler::initializeForTemp(atools::settings::Settings::getOverloadedPath(":/littlenavconnect/resources/config/logging.cfg"));

      Application::addReportPath(QObject::tr("Log files:"), LoggingHandler::getLogFiles(false /* includeBackups */));

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

      // Initialize crashhandler - disable on Linux to get core files
      if(Settings::instance().valueBool("Options/PrintStackTrace", true))
      {
        atools::util::crashhandler::init();
        atools::util::crashhandler::setStackTraceLog(Settings::getConfigFilename(lnc::STACKTRACE_SUFFIX, lnc::CRASHREPORTS_DIR));
      }

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

#ifndef DEBUG_DISABLE_CRASH_REPORT
      QStringList crashReportFiles;

      // Settings and files have to be saved before
      crashReportFiles.append(Settings::getConfigFilename(lnc::STACKTRACE_SUFFIX, lnc::CRASHREPORTS_DIR));
      crashReportFiles.append(Settings::getFilename());

      // Add all log files last to catch any error which appear while compressing
      crashReportFiles.append(atools::logging::LoggingHandler::getLogFiles(true /* includeBackups */));

      QString reportFilename = Settings::getConfigFilename(lnc::CRASHREPORT_SUFFIX, lnc::CRASHREPORTS_DIR);

      // Remove not existing files =================================
      for(QString& filename : crashReportFiles)
      {
        if(!filename.isEmpty() && !QFile::exists(filename))
          filename.clear();
      }
      crashReportFiles.removeAll(QString());

      Application::recordStartAndDetectCrash(nullptr, Settings::getConfigFilename(".running"), reportFilename, crashReportFiles,
                                             QString(), QString(), QString());

      if(Application::isSafeMode())
        Settings::clearSettings();
#endif

      // Put in separate block to destroy main window before shutting down logging
      MainWindow mainWindow;

      mainWindow.showInitial();
      retval = Application::exec();
    }
    else
      // Got quit command from other instance
      atools::gui::Application::recordExit();
  }
  catch(atools::Exception& e)
  {
    MainWindow::deInitDataExchange();
    ATOOLS_PRINT_STACK_CRITICAL("Caught exception in main");
    ATOOLS_HANDLE_EXCEPTION(e);
    // Does not return in case of fatal error
  }
  catch(...)
  {
    MainWindow::deInitDataExchange();
    ATOOLS_PRINT_STACK_CRITICAL("Caught exception in main");
    ATOOLS_HANDLE_UNKNOWN_EXCEPTION;
    // Does not return in case of fatal error
  }

  atools::util::crashhandler::clearStackTrace(Settings::getConfigFilename(lnc::STACKTRACE_SUFFIX, lnc::CRASHREPORTS_DIR));
  MainWindow::deInitDataExchange();

  qDebug() << "app.exec() done, retval is" << retval << (retval == 0 ? "(ok)" : "(error)");
  qInfo() << "About to shut down logging";
  atools::logging::LoggingHandler::shutdown();

  return retval;
}
