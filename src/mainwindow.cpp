/*****************************************************************************
* Copyright 2015-2017 Alexander Barthel albar965@mailbox.org
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
#include "navserver.h"
#include "navservercommon.h"
#include "optionsdialog.h"

#include "settings/settings.h"
#include "gui/dialog.h"
#include "gui/helphandler.h"
#include "gui/widgetstate.h"
#include "logging/logginghandler.h"
#include "fs/sc/simconnectreply.h"
#include "fs/sc/datareaderthread.h"

#include <QMessageBox>
#include <QCloseEvent>
#include <QCommandLineParser>

static QString ABOUT_MESSAGE =
  QObject::tr("<p>is the Fligh Simulator Network agent for Little Navmap.</p>"
                "<p>This software is licensed under "
                  "<a href=\"http://www.gnu.org/licenses/gpl-3.0\">GPL3</a> or any later version.</p>"
                    "<p>The source code for this application is available at "
                      "<a href=\"https://github.com/albar965\">Github</a>.</p>"
                        "<p>More about my projects at "
                          "<a href=\"https://albar965.github.io\">albar965.github.io</a>.</p>"
                            "<p><b>Copyright 2015-2017 Alexander Barthel</b></p> "
                              "<p><a href=\"mailto:albar965@mailbox.org\">albar965@mailbox.org</a> or "
                                "<a href=\"mailto:albar965@t-online.de\">albar965@t-online.de</a></p>");

using atools::settings::Settings;
using atools::fs::sc::SimConnectData;
using atools::fs::sc::SimConnectReply;
using atools::gui::HelpHandler;

// "master" or "release/1.4"
const QString HELP_BRANCH = "release/1.4";

/* Important: keep slash at the end. Otherwise Gitbook will not display the page properly */
const QString HELP_ONLINE_URL(
  "https://albar965.gitbooks.io/little-navconnect-user-manual/content/v/" + HELP_BRANCH + "/${LANG}/");

const QString HELP_OFFLINE_URL("help/little-navconnect-user-manual-${LANG}.pdf");

MainWindow::MainWindow()
  : ui(new Ui::MainWindow)
{
  qDebug() << Q_FUNC_INFO;

  ui->setupUi(this);
  readSettings();

  supportedLanguages = atools::gui::HelpHandler::getInstalledLanguages(
    "help", "little-navconnect-user-manual-([a-z]{2})\\.pdf");

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

  QCommandLineOption showGuid({"g", "replay-gui"},
                              QObject::tr("Show replay menu items."));
  parser.addOption(showGuid);

  // Process the actual command line arguments given by the user
  parser.process(*QCoreApplication::instance());
  saveReplayFile = parser.value(saveReplayOpt);
  loadReplayFile = parser.value(loadReplayOpt);
  replaySpeed = parser.value(replaySpeedOpt).toInt();
  if(parser.isSet(showGuid))
  {
    ui->menuTools->insertActions(ui->actionResetMessages,
                                 {ui->actionReplayFileLoad, ui->actionReplayFileSave, ui->actionReplayStop});
    ui->menuTools->insertSeparator(ui->actionResetMessages);
  }

  // Bind the log function to this class for category "gui"
  using namespace std::placeholders;
  atools::logging::LoggingHandler::setLogFunction(std::bind(&MainWindow::logGuiMessage, this, _1, _2, _3));

  // Create help handler for managing the Help menu items
  helpHandler = new atools::gui::HelpHandler(this, ABOUT_MESSAGE, GIT_REVISION);

  // Create nav server but to not start it yet
  navServer = new NavServer(this, verbose,
                            Settings::instance().getAndStoreValue(SETTINGS_OPTIONS_DEFAULT_PORT,
                                                                  51968).toInt());

  connect(ui->actionQuit, &QAction::triggered, this, &QMainWindow::close);

  if(parser.isSet(showGuid))
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

  // Terminate data reader thread
  dataReader->setTerminate(true);
  dataReader->wait();
  dataReader->setTerminate(false);
  qDebug() << "MainWindow destructor dataReader terminated";

  atools::logging::LoggingHandler::setLogFunction(nullptr);
  qDebug() << "MainWindow destructor logging reset";

  delete helpHandler;
  qDebug() << "MainWindow destructor help handler deleted";
  delete ui;
  qDebug() << "MainWindow destructor ui deleted";
}

void MainWindow::showOnlineHelp()
{
  HelpHandler::openHelpUrl(this, HELP_ONLINE_URL, supportedLanguages);
}

void MainWindow::showOfflineHelp()
{
  HelpHandler::openHelpUrl(this, HELP_OFFLINE_URL, supportedLanguages);
}

void MainWindow::saveReplayFileTriggered()
{
  QString filepath = atools::gui::Dialog(this).saveFileDialog(
    tr("Save Replay"), tr("Replay Files (*.replay);;All Files (*)"), "replay", "Replay/",
    QString(), "littlenavconnect.replay");

  if(!filepath.isEmpty())
  {
    dataReader->setTerminate();
    dataReader->wait();
    dataReader->setTerminate(false);

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
    dataReader->setTerminate();
    dataReader->wait();
    dataReader->setTerminate(false);

    dataReader->setSaveReplayFilepath(QString());
    dataReader->setLoadReplayFilepath(filepath);

    dataReader->start();
  }
}

void MainWindow::stopReplay()
{
  dataReader->setTerminate();
  dataReader->wait();
  dataReader->setTerminate(false);

  dataReader->setSaveReplayFilepath(QString());
  dataReader->setLoadReplayFilepath(QString());

  dataReader->start();
}

void MainWindow::options()
{
  OptionsDialog dialog;

  Settings& settings = Settings::instance();
  unsigned int updateRateMs = settings.getAndStoreValue(SETTINGS_OPTIONS_UPDATE_RATE, 500).toUInt();
  int port = settings.getAndStoreValue(SETTINGS_OPTIONS_DEFAULT_PORT, 51968).toInt();
  bool hideHostname = settings.getAndStoreValue(SETTINGS_OPTIONS_HIDE_HOSTNAME, false).toBool();

  bool fetchAiAircraft = settings.getAndStoreValue(SETTINGS_OPTIONS_FETCH_AI_AIRCRAFT, true).toBool();
  bool fetchAiShip = settings.getAndStoreValue(SETTINGS_OPTIONS_FETCH_AI_SHIP, true).toBool();

  dialog.setUpdateRate(updateRateMs);
  dialog.setPort(port);
  dialog.setHideHostname(hideHostname);
  dialog.setFetchAiAircraft(fetchAiAircraft);
  dialog.setFetchAiShip(fetchAiShip);

  int result = dialog.exec();

  if(result == QDialog::Accepted)
  {
    settings.setValue(SETTINGS_OPTIONS_HIDE_HOSTNAME, static_cast<int>(dialog.isHideHostname()));
    settings.setValue(SETTINGS_OPTIONS_UPDATE_RATE, static_cast<int>(dialog.getUpdateRate()));
    settings.setValue(SETTINGS_OPTIONS_DEFAULT_PORT, dialog.getPort());
    settings.setValue(SETTINGS_OPTIONS_FETCH_AI_AIRCRAFT, dialog.isFetchAiAircraft());
    settings.setValue(SETTINGS_OPTIONS_FETCH_AI_SHIP, dialog.isFetchAiShip());

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
      dataReader->setTerminate();
      dataReader->wait();
      dataReader->setTerminate(false);

      dataReader->setUpdateRate(dialog.getUpdateRate());
      dataReader->start();
    }

    if(dialog.getPort() != port)
    {

      // Restart navserver on port change
      int result2 = QMessageBox::Yes;
      if(navServer->hasConnections())
        result2 = atools::gui::Dialog(this).showQuestionMsgBox(SETTINGS_ACTIONS_SHOW_PORT_CHANGE,
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
  settings.setValue(SETTINGS_ACTIONS_SHOW_QUIT, true);
  settings.setValue(SETTINGS_ACTIONS_SHOW_PORT_CHANGE, true);
}

void MainWindow::logGuiMessage(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
  if(context.category != nullptr && QString(context.category) == "gui")
  {
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
    qWarning(gui).noquote().nospace() << message;
  else
    qInfo(gui).noquote().nospace() << message;
}

void MainWindow::readSettings()
{
  qDebug() << Q_FUNC_INFO;

  verbose = Settings::instance().getAndStoreValue(SETTINGS_OPTIONS_VERBOSE, false).toBool();

  atools::gui::WidgetState(SETTINGS_MAINWINDOW_WIDGET).restore(this);
}

void MainWindow::writeSettings()
{
  qDebug() << Q_FUNC_INFO;

  atools::gui::WidgetState widgetState(SETTINGS_MAINWINDOW_WIDGET);
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
    int result = atools::gui::Dialog(this).showQuestionMsgBox(SETTINGS_ACTIONS_SHOW_QUIT,
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

  qInfo(gui).noquote().nospace() << QApplication::applicationName();
  qInfo(gui).noquote().nospace() << tr("Version %1 (revision %2).").
    arg(QApplication::applicationVersion()).arg(GIT_REVISION);

  qInfo(gui).noquote().nospace()
    << tr("Data Version %1. Reply Version %2.").arg(SimConnectData::getDataVersion()).arg(
    SimConnectReply::getReplyVersion());

  atools::settings::Settings& settings = Settings::instance();
  dataReader = new atools::fs::sc::DataReaderThread(this, verbose);

#if defined(Q_OS_WIN32)
  if(!dataReader->isSimconnectAvailable())
  {
    QMessageBox::warning(this, QApplication::applicationName(),
                         tr("No Flight Simulator installation found.<br/>"
                            "Could not load SimConnect.<br/><br/>"
                            "Exiting now."));
    close();
  }
#endif

  dataReader->setReconnectRateSec(settings.getAndStoreValue(SETTINGS_OPTIONS_RECONNECT_RATE, 10).toInt());
  dataReader->setUpdateRate(settings.getAndStoreValue(SETTINGS_OPTIONS_UPDATE_RATE, 500).toUInt());
  dataReader->setLoadReplayFilepath(loadReplayFile);
  dataReader->setSaveReplayFilepath(saveReplayFile);
  dataReader->setReplaySpeed(replaySpeed);

  atools::fs::sc::Options options = atools::fs::sc::NO_OPTION;
  if(settings.getAndStoreValue(SETTINGS_OPTIONS_FETCH_AI_AIRCRAFT, true).toBool())
    options |= atools::fs::sc::FETCH_AI_AIRCRAFT;
  if(settings.getAndStoreValue(SETTINGS_OPTIONS_FETCH_AI_SHIP, true).toBool())
    options |= atools::fs::sc::FETCH_AI_BOAT;
  dataReader->setSimconnectOptions(options);

  connect(dataReader, &atools::fs::sc::DataReaderThread::postLogMessage, this, &MainWindow::postLogMessage);

  qInfo(gui).noquote().nospace() << tr("Starting server. This can take up to a minute ...");

  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

  QGuiApplication::setOverrideCursor(Qt::WaitCursor);

  dataReader->start();

  navServer->startServer(dataReader);

  QGuiApplication::restoreOverrideCursor();

  qInfo(gui).noquote().nospace() << tr("Server running.");
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
