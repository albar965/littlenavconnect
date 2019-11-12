/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel alex@littlenavmap.org
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

#include "ui_mainwindow.h"
#include "fs/ns/navserver.h"
#include "fs/ns/navservercommon.h"
#include "optionsdialog.h"

#include "settings/settings.h"
#include "gui/dialog.h"
#include "gui/helphandler.h"
#include "gui/widgetstate.h"
#include "logging/logginghandler.h"
#include "logging/loggingguiabort.h"
#include "fs/sc/simconnectreply.h"
#include "fs/sc/datareaderthread.h"
#include "constants.h"
#include "fs/sc/simconnecthandler.h"
#include "fs/sc/xpconnecthandler.h"

#include <QMessageBox>
#include <QCloseEvent>
#include <QCommandLineParser>
#include <QActionGroup>
#include <QDir>
#include <QRegularExpression>

using atools::settings::Settings;
using atools::fs::sc::SimConnectData;
using atools::fs::sc::SimConnectReply;
using atools::gui::HelpHandler;

// "master" or "release/1.4"
const QString HELP_BRANCH = "develop/2.5"; // VERSION_NUMBER

/* Important: keep slash at the end. Otherwise Gitbook will not display the page properly */
const QString HELP_ONLINE_URL(
  "https://www.littlenavmap.org/manuals/littlenavconnect/" + HELP_BRANCH + "/${LANG}/");

const QString HELP_OFFLINE_FILE("help/little-navconnect-user-manual-${LANG}.pdf");

MainWindow::MainWindow()
  : ui(new Ui::MainWindow)
{
  qDebug() << Q_FUNC_INFO;

  aboutMessage =
    QObject::tr("<p>is the Fligh Simulator Network agent for Little Navmap.</p>"
                  "<p>This software is licensed under "
                    "<a href=\"http://www.gnu.org/licenses/gpl-3.0\">GPL3</a> or any later version.</p>"
                      "<p>The source code for this application is available at "
                        "<a href=\"https://github.com/albar965\">Github</a>.</p>"
                          "<p>More about my projects at "
                            "<a href=\"https://www.littlenavmap.org\">www.littlenavmap.org</a>.</p>"
                              "<p><b>Copyright 2015-2019 Alexander Barthel</b></p>");

  // Show a dialog on fatal log events like asserts
  atools::logging::LoggingGuiAbortHandler::setGuiAbortFunction(this);

  ui->setupUi(this);

  readSettings();

  // Get the online indicator file which shows which help files are available online
  QString onlineFlagFile = atools::gui::HelpHandler::getHelpFile(
    QString("help") + QDir::separator() + "little-navconnect-user-manual-${LANG}.online", false /*override*/);

  // Extract language from the file
  QRegularExpression regexp("little-navconnect-user-manual-(.+)\\.online", QRegularExpression::CaseInsensitiveOption);
  QRegularExpressionMatch match = regexp.match(onlineFlagFile);
  if(match.hasMatch() && !match.captured(1).isEmpty())
    supportedLanguageOnlineHelp = match.captured(1);
  else
    supportedLanguageOnlineHelp = "en";

  QCommandLineParser parser;
  parser.addHelpOption();
  parser.addVersionOption();

  QCommandLineOption saveReplayOpt({"s", "save-replay"},
                                   QObject::tr("Save replay data to <file>."),
                                   QObject::tr("file"));
  parser.addOption(saveReplayOpt);

  QCommandLineOption loadReplayOpt({"l", "load-replay"},
                                   QObject::tr("Load replay data from <file>."),
                                   QObject::tr("file"));
  parser.addOption(loadReplayOpt);

  QCommandLineOption replaySpeedOpt({"r", "replay-speed"},
                                    QObject::tr("Use speed factor <speed> for replay."),
                                    QObject::tr("speed"));
  parser.addOption(replaySpeedOpt);

  QCommandLineOption showReplay({"g", "replay-gui"},
                                QObject::tr("Show replay menu items."));
  parser.addOption(showReplay);

  // Process the actual command line arguments given by the user
  parser.process(*QCoreApplication::instance());
  saveReplayFile = parser.value(saveReplayOpt);
  loadReplayFile = parser.value(loadReplayOpt);
  replaySpeed = parser.value(replaySpeedOpt).toInt();
  if(parser.isSet(showReplay))
  {
    ui->menuTools->insertActions(ui->actionResetMessages,
                                 {ui->actionReplayFileLoad, ui->actionReplayFileSave, ui->actionReplayStop});
    ui->menuTools->insertSeparator(ui->actionResetMessages);
  }

  // Right align the help button
  QWidget *spacerWidget = new QWidget(ui->toolBar);
  spacerWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  ui->toolBar->insertWidget(ui->actionContents, spacerWidget);

  // Bind the log function to this class for category "gui"
  using namespace std::placeholders;
  atools::logging::LoggingHandler::setLogFunction(std::bind(&MainWindow::logGuiMessage, this, _1, _2, _3));

  // Create help handler for managing the Help menu items
  helpHandler = new atools::gui::HelpHandler(this, aboutMessage, GIT_REVISION);

  int defaultPort = Settings::instance().getAndStoreValue(lnc::SETTINGS_OPTIONS_DEFAULT_PORT, 51968).toInt();
  bool hideHostname =
    Settings::instance().getAndStoreValue(lnc::SETTINGS_OPTIONS_HIDE_HOSTNAME, false).toBool();
  // Create nav server but to not start it yet
  atools::fs::ns::NavServerOptions options = atools::fs::ns::NONE;
  if(verbose)
    options |= atools::fs::ns::VERBOSE;
  if(hideHostname)
    options |= atools::fs::ns::HIDE_HOST;

  navServer = new atools::fs::ns::NavServer(this, options, defaultPort);

  // Create a group to turn the simulator actions into mutual exclusive ones
  simulatorActionGroup = new QActionGroup(ui->menuTools);
  simulatorActionGroup->addAction(ui->actionConnectFsx);
  simulatorActionGroup->addAction(ui->actionConnectXplane);

  connect(ui->actionConnectFsx, &QAction::triggered, this, &MainWindow::simulatorSelectionTriggered);
  connect(ui->actionConnectXplane, &QAction::triggered, this, &MainWindow::simulatorSelectionTriggered);

  connect(ui->actionQuit, &QAction::triggered, this, &QMainWindow::close);

  if(parser.isSet(showReplay))
  {
    connect(ui->actionReplayFileLoad, &QAction::triggered, this, &MainWindow::loadReplayFileTriggered);
    connect(ui->actionReplayFileSave, &QAction::triggered, this, &MainWindow::saveReplayFileTriggered);
    connect(ui->actionReplayStop, &QAction::triggered, this, &MainWindow::stopReplay);
  }

  connect(ui->actionResetMessages, &QAction::triggered, this, &MainWindow::resetMessages);
  connect(ui->actionOptions, &QAction::triggered, this, &MainWindow::options);
  connect(ui->actionContents, &QAction::triggered, this, &MainWindow::showOnlineHelp);
  connect(ui->actionContentsOffline, &QAction::triggered, this, &MainWindow::showOfflineHelp);
  connect(ui->actionAbout, &QAction::triggered, helpHandler, &atools::gui::HelpHandler::about);
  connect(ui->actionAboutQt, &QAction::triggered, helpHandler, &atools::gui::HelpHandler::aboutQt);

  // Log messages have to be redirected through a message so that QTextEdit::append is not called on
  // a thread context different than main
  connect(this, &MainWindow::appendLogMessage, ui->textEdit, &QTextEdit::append);

  // Once visible start server and log messagess
  connect(this, &MainWindow::windowShown, this, &MainWindow::mainWindowShown, Qt::QueuedConnection);
}

MainWindow::~MainWindow()
{
  qDebug() << Q_FUNC_INFO;

  navServer->stopServer();
  qDebug() << Q_FUNC_INFO << "navServer stopped";

  delete navServer;
  qDebug() << Q_FUNC_INFO << "navServer deleted";

  dataReader->terminateThread();
  qDebug() << Q_FUNC_INFO << "dataReader terminated";

  delete dataReader;
  qDebug() << Q_FUNC_INFO << "dataReader deleted";

  delete fsxConnectHandler;
  qDebug() << Q_FUNC_INFO << "fsxConnectHandler deleted";

  delete xpConnectHandler;
  qDebug() << Q_FUNC_INFO << "xpConnectHandler deleted";

  atools::logging::LoggingHandler::setLogFunction(nullptr);
  qDebug() << Q_FUNC_INFO << "logging reset";

  delete helpHandler;
  qDebug() << Q_FUNC_INFO << "help handler deleted";

  delete simulatorActionGroup;
  qDebug() << Q_FUNC_INFO << "fsActionGroup deleted";

  delete ui;
  qDebug() << Q_FUNC_INFO << "ui deleted";

  atools::logging::LoggingGuiAbortHandler::resetGuiAbortFunction();

  qDebug() << "MainWindow destructor about to shut down logging";
  atools::logging::LoggingHandler::shutdown();
}

void MainWindow::showOnlineHelp()
{
  HelpHandler::openHelpUrlWeb(this, HELP_ONLINE_URL, supportedLanguageOnlineHelp);
}

void MainWindow::showOfflineHelp()
{
  HelpHandler::openFile(this, HelpHandler::getHelpFile(HELP_OFFLINE_FILE, false /* override */));
}

atools::fs::sc::ConnectHandler *MainWindow::handlerForSelection()
{
  atools::fs::sc::ConnectHandler *handler;
  if(ui->actionConnectFsx->isChecked())
    handler = fsxConnectHandler;
  else
    handler = xpConnectHandler;

  return handler;
}

void MainWindow::handlerChanged()
{
  if(ui->actionConnectFsx->isChecked())
  {
    qInfo(atools::fs::ns::gui).noquote().nospace()
      << tr("Connecting to FSX or Prepar3D using SimConnect.");
    Settings::instance().setValue(lnc::SETTINGS_OPTIONS_SIMULATOR_FSX, true);
  }
  else
  {
    qInfo(atools::fs::ns::gui).noquote().nospace()
      << tr("Connecting to X-Plane using the Little Xpconnect plugin.");
    Settings::instance().setValue(lnc::SETTINGS_OPTIONS_SIMULATOR_FSX, false);
  }
  Settings::instance().syncSettings();
}

void MainWindow::simulatorSelectionTriggered()
{
  // Update rate changed - restart data readers
  dataReader->terminateThread();
  dataReader->setHandler(handlerForSelection());
  dataReader->start();
  handlerChanged();
}

void MainWindow::saveReplayFileTriggered()
{
  QString filepath = atools::gui::Dialog(this).saveFileDialog(
    tr("Save Replay"), tr("Replay Files (*.replay);;All Files (*)"), "replay", "Replay/",
    QString(), "littlenavconnect.replay");

  if(!filepath.isEmpty())
  {
    dataReader->terminateThread();
    dataReader->setSaveReplayFilepath(filepath);
    dataReader->setLoadReplayFilepath(QString());

    dataReader->start();
  }
}

void MainWindow::loadReplayFileTriggered()
{
  QString filepath = atools::gui::Dialog(this).openFileDialog(
    tr("Open Replay"), tr("Replay Files (*.replay);;All Files (*)"), "Replay/", QString());

  if(!filepath.isEmpty())
  {
    dataReader->terminateThread();
    dataReader->setSaveReplayFilepath(QString());
    dataReader->setLoadReplayFilepath(filepath);

    dataReader->start();
  }
}

void MainWindow::stopReplay()
{
  dataReader->terminateThread();
  dataReader->setSaveReplayFilepath(QString());
  dataReader->setLoadReplayFilepath(QString());

  dataReader->start();
}

void MainWindow::options()
{
  OptionsDialog dialog;

  Settings& settings = Settings::instance();
  unsigned int updateRateMs =
    settings.getAndStoreValue(lnc::SETTINGS_OPTIONS_UPDATE_RATE, 500).toUInt();
  int port = settings.getAndStoreValue(lnc::SETTINGS_OPTIONS_DEFAULT_PORT, 51968).toInt();
  bool hideHostname = settings.getAndStoreValue(lnc::SETTINGS_OPTIONS_HIDE_HOSTNAME, false).toBool();

  bool fetchAiAircraft = settings.getAndStoreValue(lnc::SETTINGS_OPTIONS_FETCH_AI_AIRCRAFT, true).toBool();
  bool fetchAiShip = settings.getAndStoreValue(lnc::SETTINGS_OPTIONS_FETCH_AI_SHIP, true).toBool();

  dialog.setUpdateRate(updateRateMs);
  dialog.setPort(port);
  dialog.setHideHostname(hideHostname);
  dialog.setFetchAiAircraft(fetchAiAircraft);
  dialog.setFetchAiShip(fetchAiShip);

  int result = dialog.exec();

  if(result == QDialog::Accepted)
  {
    settings.setValue(lnc::SETTINGS_OPTIONS_HIDE_HOSTNAME, static_cast<int>(dialog.isHideHostname()));
    settings.setValue(lnc::SETTINGS_OPTIONS_UPDATE_RATE, static_cast<int>(dialog.getUpdateRate()));
    settings.setValue(lnc::SETTINGS_OPTIONS_DEFAULT_PORT, dialog.getPort());
    settings.setValue(lnc::SETTINGS_OPTIONS_FETCH_AI_AIRCRAFT, dialog.isFetchAiAircraft());
    settings.setValue(lnc::SETTINGS_OPTIONS_FETCH_AI_SHIP, dialog.isFetchAiShip());

    settings.syncSettings();

    atools::fs::sc::Options options = atools::fs::sc::NO_OPTION;
    if(dialog.isFetchAiAircraft())
      options |= atools::fs::sc::FETCH_AI_AIRCRAFT;
    if(dialog.isFetchAiShip())
      options |= atools::fs::sc::FETCH_AI_BOAT;

    dataReader->setSimconnectOptions(options);

    if(dialog.getUpdateRate() != updateRateMs)
    {
      // Update rate changed - restart data readers
      dataReader->terminateThread();
      dataReader->setUpdateRate(dialog.getUpdateRate());
      dataReader->start();
    }

    if(dialog.getPort() != port)
    {

      // Restart navserver on port change
      int result2 = QMessageBox::Yes;
      if(navServer->hasConnections())
        result2 = atools::gui::Dialog(this).showQuestionMsgBox(
          lnc::SETTINGS_ACTIONS_SHOW_PORT_CHANGE,
          tr(
            "There are still applications connected.\n"
            "Really change the Network Port?"),
          tr("Do not &show this dialog again."),
          QMessageBox::Yes | QMessageBox::No,
          QMessageBox::No, QMessageBox::Yes);

      if(result2 == QMessageBox::Yes)
      {
        navServer->stopServer();
        navServer->setPort(dialog.getPort());
        navServer->startServer(dataReader);
      }
    }
  }
}

void MainWindow::resetMessages()
{
  Settings& settings = Settings::instance();
  settings.setValue(lnc::SETTINGS_ACTIONS_SHOW_QUIT, true);
  settings.setValue(lnc::SETTINGS_ACTIONS_SHOW_PORT_CHANGE, true);
}

void MainWindow::logGuiMessage(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
  if(type == QtFatalMsg)
    // Fatal will look like a crash anyway - bail out to avoid follow up errors
    return;

  if(context.category != nullptr && QString(context.category) == "gui")
  {
    // Define colors
    QString style;
    switch(type)
    {
      case QtDebugMsg:
        style = "color:darkgrey";
        break;
      case QtWarningMsg:
        style = "color:orange;font-weight:bold";
        break;
      case QtFatalMsg:
      case QtCriticalMsg:
        style = "color:red;font-weight:bold";
        break;
      case QtInfoMsg:
        break;
    }

    QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd h:mm:ss");
    // Use a signal to update the text edit in the main thread context
    emit appendLogMessage("[" + now + "] <span style=\"" + style + "\">" + message + "</span>");
  }
}

void MainWindow::postLogMessage(QString message, bool warning)
{
  if(warning)
    qWarning(atools::fs::ns::gui).noquote().nospace() << message;
  else
    qInfo(atools::fs::ns::gui).noquote().nospace() << message;
}

void MainWindow::readSettings()
{
  qDebug() << Q_FUNC_INFO;

  verbose = Settings::instance().getAndStoreValue(lnc::SETTINGS_OPTIONS_VERBOSE, false).toBool();

  atools::gui::WidgetState(lnc::SETTINGS_MAINWINDOW_WIDGET).restore(this);
}

void MainWindow::writeSettings()
{
  qDebug() << Q_FUNC_INFO;

  atools::gui::WidgetState widgetState(lnc::SETTINGS_MAINWINDOW_WIDGET);
  widgetState.save(this);
  widgetState.syncSettings();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
  // Catch all close events like Ctrl-Q or Menu/Exit or clicking on the
  // close button on the window frame
  qDebug() << Q_FUNC_INFO;

  if(navServer->hasConnections())
  {
    int result = atools::gui::Dialog(this).showQuestionMsgBox(lnc::SETTINGS_ACTIONS_SHOW_QUIT,
                                                              tr("There are still applications connected.\n"
                                                                 "Really Quit?"),
                                                              tr("Do not &show this dialog again."),
                                                              QMessageBox::Yes | QMessageBox::No,
                                                              QMessageBox::No, QMessageBox::Yes);

    if(result != QMessageBox::Yes)
      event->ignore();
  }

  writeSettings();
}

void MainWindow::mainWindowShown()
{
  qDebug() << Q_FUNC_INFO;

  atools::settings::Settings& settings = Settings::instance();

  qInfo(atools::fs::ns::gui).noquote().nospace() << QApplication::applicationName();
  qInfo(atools::fs::ns::gui).noquote().nospace() << tr("Version %1 (revision %2).").
    arg(QApplication::applicationVersion()).arg(GIT_REVISION);

  qInfo(atools::fs::ns::gui).noquote().nospace()
    << tr("Data Version %1. Reply Version %2.").arg(SimConnectData::getDataVersion()).arg(
    SimConnectReply::getReplyVersion());

  // Build the handler classes which are an abstraction to SimConnect and the Little Xpconnect shared memory
  fsxConnectHandler = new atools::fs::sc::SimConnectHandler(verbose);
  fsxConnectHandler->loadSimConnect(QApplication::applicationFilePath() + ".simconnect");
  xpConnectHandler = new atools::fs::sc::XpConnectHandler();

#ifdef Q_OS_WIN32
  // Show toolbar with both buttons
  bool fsx = true;

  // Check the first time if SimConnect is available - if yes use FSX settings
  // Otherwise fall back to stored value or X-Plane
  fsx = settings.getAndStoreValue(lnc::SETTINGS_OPTIONS_SIMULATOR_FSX, fsxConnectHandler->isLoaded()).toBool();

  if(!fsxConnectHandler->isLoaded())
    // No SimConnect switch to X-Plane
    fsx = false;

  qDebug() << "FSX status" << fsx;

  if(fsxConnectHandler->isLoaded())
  {
    ui->toolBar->insertAction(ui->actionOptions, ui->actionConnectFsx);
    ui->toolBar->insertAction(ui->actionOptions, ui->actionConnectXplane);
    ui->toolBar->insertSeparator(ui->actionOptions);

    ui->menuTools->insertAction(ui->actionResetMessages, ui->actionConnectFsx);
    ui->menuTools->insertAction(ui->actionResetMessages, ui->actionConnectXplane);
    ui->menuTools->insertSeparator(ui->actionResetMessages);
  }

  ui->actionConnectFsx->setChecked(fsx);
  ui->actionConnectXplane->setChecked(!fsx);

#else
  // Activate X-Plane on non windows
  settings.setValue(lnc::SETTINGS_OPTIONS_SIMULATOR_FSX, false);
  ui->actionConnectXplane->setChecked(true);
  ui->actionConnectFsx->setChecked(false);
#endif

  ui->menuTools->insertAction(ui->actionOptions, ui->toolBar->toggleViewAction());

  // Build the thread which will read the data from the interfaces
  dataReader = new atools::fs::sc::DataReaderThread(this, verbose);
  dataReader->setHandler(handlerForSelection());
  handlerChanged();

  dataReader->setReconnectRateSec(settings.getAndStoreValue(lnc::SETTINGS_OPTIONS_RECONNECT_RATE, 10).toInt());
  dataReader->setUpdateRate(settings.getAndStoreValue(lnc::SETTINGS_OPTIONS_UPDATE_RATE, 500).toUInt());
  dataReader->setLoadReplayFilepath(loadReplayFile);
  dataReader->setSaveReplayFilepath(saveReplayFile);
  dataReader->setReplaySpeed(replaySpeed);

  atools::fs::sc::Options options = atools::fs::sc::NO_OPTION;
  if(settings.getAndStoreValue(lnc::SETTINGS_OPTIONS_FETCH_AI_AIRCRAFT, true).toBool())
    options |= atools::fs::sc::FETCH_AI_AIRCRAFT;
  if(settings.getAndStoreValue(lnc::SETTINGS_OPTIONS_FETCH_AI_SHIP, true).toBool())
    options |= atools::fs::sc::FETCH_AI_BOAT;
  dataReader->setSimconnectOptions(options);

  connect(dataReader, &atools::fs::sc::DataReaderThread::postLogMessage, this, &MainWindow::postLogMessage);

  qInfo(atools::fs::ns::gui).noquote().nospace() << tr("Starting server. This can take up to a minute ...");

  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

  QGuiApplication::setOverrideCursor(Qt::WaitCursor);

  dataReader->start();

  navServer->startServer(dataReader);

  QGuiApplication::restoreOverrideCursor();

  qInfo(atools::fs::ns::gui).noquote().nospace() << tr("Server running.");
}

void MainWindow::showEvent(QShowEvent *event)
{
  if(firstStart)
  {
    emit windowShown();
    firstStart = false;
  }

  event->ignore();
}
