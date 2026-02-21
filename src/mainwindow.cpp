/*****************************************************************************
* Copyright 2015-2026 Alexander Barthel alex@littlenavmap.org
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

#include "constants.h"
#include "fs/ns/navserver.h"
#include "fs/ns/navservercommon.h"
#include "fs/sc/datareaderthread.h"
#include "fs/sc/simconnecthandler.h"
#include "fs/sc/simconnectreply.h"
#include "fs/sc/xpconnecthandler.h"
#include "geo/calculations.h"
#include "gui/application.h"
#include "gui/desktopservices.h"
#include "gui/dialog.h"
#include "gui/helphandler.h"
#include "gui/widgetstate.h"
#include "gui/widgetutil.h"
#include "logging/loggingguiabort.h"
#include "logging/logginghandler.h"
#include "optionsdialog.h"
#include "settings/settings.h"
#include "ui_mainwindow.h"
#include "util/htmlbuilder.h"
#include "util/properties.h"
#include "util/signalhandler.h"
#include "util/version.h"
#include "win/activationcontext.h"

#include <QCloseEvent>
#include <QCommandLineParser>
#include <QDir>
#include <QRegularExpression>
#include <QSystemTrayIcon>
#include <QTextDocumentFragment>
#include <QTimer>
#include <QStringBuilder>
#include <QActionGroup>

using atools::settings::Settings;
using atools::fs::sc::SimConnectData;
using atools::fs::sc::SimConnectReply;
using atools::gui::HelpHandler;
using atools::gui::Application;

atools::gui::DataExchange *MainWindow::dataExchange = nullptr;

/* Important: keep slash at the end. Otherwise browser might not display the page properly */
const static QString HELP_ONLINE_URL("https://www.littlenavmap.org/manuals/littlenavconnect/" + lnc::HELP_BRANCH + "/${LANG}/");

const static QString HELP_OFFLINE_FILE("help/little-navconnect-user-manual-${LANG}.pdf");

MainWindow::MainWindow()
  : ui(new Ui::MainWindow)
{
  qDebug() << Q_FUNC_INFO;

  setWindowFlag(Qt::WindowContextHelpButtonHint, false);

  activationContext = new atools::win::ActivationContext;

  aboutMessage =
    QObject::tr("<p style='white-space:pre'>is the Flight Simulator Network agent for Little Navmap.</p>"
                  "<p>This software is licensed under "
                    "<a href=\"http://www.gnu.org/licenses/gpl-3.0\">GPL3</a> or any later version.</p>"
                      "<p>The source code for this application is available at "
                        "<a href=\"https://github.com/albar965\">GitHub</a>.</p>"
                          "<p>More about my projects at "
                            "<a href=\"https://www.littlenavmap.org\">www.littlenavmap.org</a>.</p>"
                              "<p><b>Copyright 2015-2026 Alexander Barthel</b></p>");

  // Show a dialog on fatal log events like asserts
  atools::logging::LoggingGuiAbortHandler::setGuiAbortFunction(this);

  ui->setupUi(this);
  ui->textEdit->clear();

#if defined(Q_OS_LINUX)
  // Catch Ctrl+C and other signals to avoid data loss on Linux/Unix systems
  const atools::util::SignalHandler& signalHandler = atools::util::SignalHandler::instance();
  connect(&signalHandler, &atools::util::SignalHandler::sigHupReceived, this, &MainWindow::closeFromSignal, Qt::QueuedConnection);
  connect(&signalHandler, &atools::util::SignalHandler::sigTermReceived, this, &MainWindow::closeFromSignal, Qt::QueuedConnection);
  connect(&signalHandler, &atools::util::SignalHandler::sigIntReceived, this, &MainWindow::closeFromSignal, Qt::QueuedConnection);
#endif

  restoreState();

  // Update window title ===================================================
  // Remember original title
  mainWindowTitle = windowTitle();

  QString newTitle = mainWindowTitle;
  atools::util::Version version(QApplication::applicationVersion());

  // Program version and revision ==========================================
  if(version.isStable() || version.isReleaseCandidate() || version.isBeta())
    newTitle += QStringLiteral(" %1").arg(version.getVersionString());
  else
    newTitle += QStringLiteral(" %1 (%2)").arg(version.getVersionString()).arg(GIT_REVISION_LITTLENAVCONNECT);

#if defined(WINARCH64)
  newTitle += tr(" 64-bit");
#elif defined(WINARCH32)
  newTitle += tr(" 32-bit");
#endif

#ifndef QT_NO_DEBUG
  newTitle += " - DEBUG";
#endif

  setWindowTitle(newTitle);

  // Get the online indicator file which shows which help files are available online
  QString onlineFlagFile = atools::gui::HelpHandler::getHelpFile(
    QString("help") + atools::SEP + "little-navconnect-user-manual-${LANG}.online", QLocale().name());

  // Extract language from the file
  const static QRegularExpression regexp("little-navconnect-user-manual-(.+)\\.online", QRegularExpression::CaseInsensitiveOption);
  QRegularExpressionMatch match = regexp.match(onlineFlagFile);
  if(match.hasMatch() && !match.captured(1).isEmpty())
    supportedLanguageOnlineHelp = match.captured(1);
  else
    supportedLanguageOnlineHelp = "en";

  // Process the actual command line arguments given by the user
  saveReplayFile = Application::getStartupOptionStr(lnc::STARTUP_COMMAND_SAVE_REPLAY);
  loadReplayFile = Application::getStartupOptionStr(lnc::STARTUP_COMMAND_LOAD_REPLAY);
  replaySpeed = Application::getStartupOptionStr(lnc::STARTUP_COMMAND_REPLAY_SPEED).toInt();
  replayWhazzupUpdateSpeed = Application::getStartupOptionStr(lnc::STARTUP_COMMAND_WRITE_WHAZZUP_SPEED).toInt();
  writeReplayWhazzupFile = Application::getStartupOptionStr(lnc::STARTUP_COMMAND_WRITE_WHAZZUP);

  // Add replay menu items if requested
  if(Application::hasStartupOption(lnc::STARTUP_COMMAND_REPLAY_GUI))
  {
    ui->menuTools->insertActions(ui->actionResetMessages, {ui->actionReplayFileLoad, ui->actionReplayFileSave, ui->actionReplayStop});
    ui->menuTools->insertSeparator(ui->actionResetMessages);
  }

  // Right align the help button
  QWidget *spacerWidget = new QWidget(ui->toolBar);
  spacerWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  ui->toolBar->insertWidget(ui->actionHelp, spacerWidget);

  // Bind the log function to this class for category "gui"
  using namespace std::placeholders;
  atools::logging::LoggingHandler::setLogFunction(std::bind(&MainWindow::logGuiMessage, this, _1, _2, _3));

  // Create help handler for managing the Help menu items
  helpHandler = new atools::gui::HelpHandler(this, aboutMessage, GIT_REVISION_LITTLENAVCONNECT);

  int defaultPort = Settings::instance().getAndStoreValue(lnc::SETTINGS_OPTIONS_DEFAULT_PORT, 51968).toInt();
  bool hideHostname = Settings::instance().getAndStoreValue(lnc::SETTINGS_OPTIONS_HIDE_HOSTNAME, false).toBool();

  // Create nav server but to not start it yet
  atools::fs::ns::NavServerOptions options = atools::fs::ns::NONE;
  options.setFlag(atools::fs::ns::VERBOSE, verbose);
  options.setFlag(atools::fs::ns::HIDE_HOST, hideHostname);

  navServer = new atools::fs::ns::NavServer(this, options, defaultPort);

#if defined(SIMCONNECT_BUILD_WIN64)
  ui->actionConnectFsx->setText(tr("MSFS"));
  ui->actionConnectFsx->setToolTip(tr("Connect to Microsoft Flight Simulator 2020 using SimConnect."));
#elif defined(SIMCONNECT_BUILD_WIN32)
  ui->actionConnectFsx->setText(tr("FSX or Prepar3D"));
  ui->actionConnectFsx->setToolTip(tr("Connect to FSX or Prepar3D using SimConnect."));
#else
  ui->actionConnectFsx->setText(tr("FSX, Prepar3D or MSFS"));
  ui->actionConnectFsx->setToolTip(tr("Connect to FSX, Prepar3D or Microsoft Flight Simulator 2020 using SimConnect."));
#endif

  // Create a group to turn the simulator actions into mutual exclusive ones
  simulatorActionGroup = new QActionGroup(ui->menuTools);
  simulatorActionGroup->addAction(ui->actionConnectFsx);
  simulatorActionGroup->addAction(ui->actionConnectXplane);

  connect(ui->actionConnectFsx, &QAction::triggered, this, &MainWindow::simulatorSelectionTriggered);
  connect(ui->actionConnectXplane, &QAction::triggered, this, &MainWindow::simulatorSelectionTriggered);
  connect(ui->actionQuit, &QAction::triggered, this, &MainWindow::quitFromTrayOrAction);

  if(Application::hasStartupOption(lnc::STARTUP_COMMAND_REPLAY_GUI))
  {
    connect(ui->actionReplayFileLoad, &QAction::triggered, this, &MainWindow::loadReplayFileTriggered);
    connect(ui->actionReplayFileSave, &QAction::triggered, this, &MainWindow::saveReplayFileTriggered);
    connect(ui->actionReplayStop, &QAction::triggered, this, &MainWindow::stopReplay);
  }

  connect(ui->actionResetMessages, &QAction::triggered, this, &MainWindow::resetMessages);
  connect(ui->actionOptions, &QAction::triggered, this, &MainWindow::options);
  connect(ui->actionHelp, &QAction::triggered, this, &MainWindow::showOnlineHelp);
  connect(ui->actionHelpOffline, &QAction::triggered, this, &MainWindow::showOfflineHelp);
  connect(ui->actionAbout, &QAction::triggered, helpHandler, &atools::gui::HelpHandler::about);
  connect(ui->actionAboutQt, &QAction::triggered, helpHandler, &atools::gui::HelpHandler::aboutQt);

  connect(ui->actionMinimizeTray, &QAction::toggled, this, &MainWindow::actionTrayToggled);
  connect(ui->actionStartMinimizeTray, &QAction::toggled, this, &MainWindow::actionTrayToggled);

  // Log messages have to be redirected through a message so that QTextEdit::append is not called on
  // a thread context different than main
  connect(this, &MainWindow::appendLogMessage, ui->textEdit, &QTextEdit::append, Qt::QueuedConnection);

  connect(atools::gui::Application::applicationInstance(), &atools::gui::Application::applicationAboutToQuit, this, &MainWindow::deInit);

  if(QSystemTrayIcon::isSystemTrayAvailable())
  {
    if(ui->actionMinimizeTray->isChecked() || ui->actionStartMinimizeTray->isChecked())
      // Create and show tray and menus if any of the menu options are enabled
      createTrayIcon();
  }
  else
  {
    ui->actionMinimizeTray->setDisabled(true);
    ui->actionMinimizeTray->setChecked(false);
    ui->actionStartMinimizeTray->setDisabled(true);
    ui->actionStartMinimizeTray->setChecked(false);
  }
}

MainWindow::~MainWindow()
{
  qDebug() << Q_FUNC_INFO;
  deInit();
}

void MainWindow::deInit()
{
  qDebug() << Q_FUNC_INFO << "Enter deInitCalled" << deInitCalled;

  if(!deInitCalled)
  {
    deInitCalled = true;
    deleteTrayIcon();

    if(navServer != nullptr)
    {
      qDebug() << Q_FUNC_INFO << "Stopping NavServer";
      navServer->stopServer();
      ATOOLS_DELETE_LATER_LOG(navServer);
    }

    if(dataReaderThread != nullptr)
    {
      qDebug() << Q_FUNC_INFO << "Terminating DataReaderThread";
      dataReaderThread->terminateThread();
      ATOOLS_DELETE_LATER_LOG(dataReaderThread);
    }

    if(simConnectHandler != nullptr)
    {
      simConnectHandler->close();
      simConnectHandler->releaseSimConnect();
    }

    ATOOLS_DELETE_LOG(simConnectHandler);
    ATOOLS_DELETE_LOG(xpConnectHandler);

    qDebug() << Q_FUNC_INFO << "reset logging";
    atools::logging::LoggingHandler::setLogFunction(nullptr);

    ATOOLS_DELETE_LOG(helpHandler);
    ATOOLS_DELETE_LOG(simulatorActionGroup);
    ATOOLS_DELETE_LOG(ui);
    ATOOLS_DELETE_LOG(activationContext);

#if defined(Q_OS_LINUX)
    // Remove signal handler
    atools::util::SignalHandler::deleteInstance();
#endif

    atools::logging::LoggingGuiAbortHandler::resetGuiAbortFunction();
  }

  qDebug() << Q_FUNC_INFO << "Exit";
}

void MainWindow::dataExchangeDataFetched(atools::util::Properties properties)
{
  // Check for message from other instance
  if(!properties.isEmpty())
  {
    // Found message
    qDebug() << Q_FUNC_INFO << properties;

    if(properties.contains(lnc::STARTUP_COMMAND_QUIT))
      // Quit without activate =====================================================
      quitFromTrayOrAction();
    else
    {
      // Activate window - always sent by other instance =====================================================
      if(properties.getPropertyBool(lnc::STARTUP_COMMAND_ACTIVATE))
      {
        setVisible(true);
        activateWindow();
        raise();
      }
    }
  }
  else
    qDebug() << Q_FUNC_INFO << "properties empty";
}

bool MainWindow::initDataExchange()
{
  if(dataExchange == nullptr)
    dataExchange = new atools::gui::DataExchange(false, lnc::PROGRAM_GUID);

  return dataExchange->isExit();
}

void MainWindow::deInitDataExchange()
{
  ATOOLS_DELETE_LOG(dataExchange);
}

void MainWindow::showOnlineHelp()
{
  HelpHandler::openHelpUrlWeb(this, HELP_ONLINE_URL, supportedLanguageOnlineHelp);
}

void MainWindow::showOfflineHelp()
{
  atools::gui::DesktopServices::openFile(this, HelpHandler::getHelpFile(HELP_OFFLINE_FILE, QLocale().name()),
                                         false /* showInFileManager */);
}

atools::fs::sc::ConnectHandler *MainWindow::handlerForSelection()
{
  if(ui->actionConnectFsx->isChecked())
    return simConnectHandler;
  else
    return xpConnectHandler;
}

void MainWindow::handlerChanged()
{
  if(ui != nullptr && ui->actionConnectFsx->isChecked())
  {
#if defined(SIMCONNECT_BUILD_WIN64)
    qInfo(atools::fs::ns::gui).noquote().nospace() << tr("Connecting to MSFS using SimConnect.");
#elif defined(SIMCONNECT_BUILD_WIN32)
    qInfo(atools::fs::ns::gui).noquote().nospace() << tr("Connecting to FSX or Prepar3D using SimConnect.");
#endif

    Settings::instance().setValue(lnc::SETTINGS_OPTIONS_SIMULATOR_FSX, true);
  }
  else
  {
    qInfo(atools::fs::ns::gui).noquote().nospace() << tr("Connecting to X-Plane using the Little Xpconnect plugin.");
    Settings::instance().setValue(lnc::SETTINGS_OPTIONS_SIMULATOR_FSX, false);
  }
  Settings::syncSettings();
}

void MainWindow::simulatorSelectionTriggered()
{
  // Update rate changed - restart data readers
  dataReaderThread->terminateThread();
  dataReaderThread->setHandler(handlerForSelection());
  dataReaderThread->start();
  handlerChanged();
}

void MainWindow::saveReplayFileTriggered()
{
  QString filepath = atools::gui::Dialog(this).saveFileDialog(tr("Save Replay"), tr("Replay Files (*.replay);;All Files (*)"),
                                                              "replay", "Replay/", QString(), "littlenavconnect.replay");

  if(!filepath.isEmpty())
  {
    dataReaderThread->terminateThread();
    dataReaderThread->setSaveReplayFilepath(filepath);
    dataReaderThread->setLoadReplayFilepath(QString());
    dataReaderThread->start();
  }
}

void MainWindow::loadReplayFileTriggered()
{
  QString filepath = atools::gui::Dialog(this).openFileDialog(
    tr("Open Replay"), tr("Replay Files (*.replay);;All Files (*)"), "Replay/", QString());

  if(!filepath.isEmpty())
  {
    dataReaderThread->terminateThread();
    dataReaderThread->setSaveReplayFilepath(QString());
    dataReaderThread->setLoadReplayFilepath(filepath);
    dataReaderThread->start();
  }
}

void MainWindow::stopReplay()
{
  dataReaderThread->terminateThread();
  dataReaderThread->setSaveReplayFilepath(QString());
  dataReaderThread->setLoadReplayFilepath(QString());
  dataReaderThread->start();
}

void MainWindow::options()
{
  qDebug(atools::fs::ns::gui) << Q_FUNC_INFO;

  OptionsDialog dialog(this);

  Settings& settings = Settings::instance();
  int updateRateMs = settings.getAndStoreValue(lnc::SETTINGS_OPTIONS_UPDATE_RATE, 500).toInt();
  int port = settings.getAndStoreValue(lnc::SETTINGS_OPTIONS_DEFAULT_PORT, 51968).toInt();

  dialog.setUpdateRate(updateRateMs);
  dialog.setPort(port);
  dialog.setHideHostname(settings.getAndStoreValue(lnc::SETTINGS_OPTIONS_HIDE_HOSTNAME, false).toBool());
  dialog.setFetchAiAircraft(settings.getAndStoreValue(lnc::SETTINGS_OPTIONS_FETCH_AI_AIRCRAFT, true).toBool());
  dialog.setFetchAiShip(settings.getAndStoreValue(lnc::SETTINGS_OPTIONS_FETCH_AI_SHIP, true).toBool());
  dialog.setFetchAiRadius(settings.getAndStoreValue(lnc::SETTINGS_OPTIONS_FETCH_AI_RADIUS, 105).toInt());

  int result = dialog.exec();

  if(result == QDialog::Accepted)
  {
    qDebug(atools::fs::ns::gui) << Q_FUNC_INFO << "options accepted";
    settings.setValue(lnc::SETTINGS_OPTIONS_HIDE_HOSTNAME, dialog.isHideHostname());
    settings.setValue(lnc::SETTINGS_OPTIONS_UPDATE_RATE, dialog.getUpdateRate());
    settings.setValue(lnc::SETTINGS_OPTIONS_DEFAULT_PORT, dialog.getPort());
    settings.setValue(lnc::SETTINGS_OPTIONS_FETCH_AI_AIRCRAFT, dialog.isFetchAiAircraft());
    settings.setValue(lnc::SETTINGS_OPTIONS_FETCH_AI_SHIP, dialog.isFetchAiShip());
    settings.setValue(lnc::SETTINGS_OPTIONS_FETCH_AI_RADIUS, dialog.getAiFetchRadiusNm());

    Settings::syncSettings();

    atools::fs::sc::Options options = atools::fs::sc::NO_OPTION;
    options.setFlag(atools::fs::sc::FETCH_AI_AIRCRAFT, dialog.isFetchAiAircraft());
    options.setFlag(atools::fs::sc::FETCH_AI_BOAT, dialog.isFetchAiShip());

    dataReaderThread->setSimconnectOptions(options);
    dataReaderThread->setAiFetchRadius(atools::geo::nmToKm(dialog.getAiFetchRadiusNm()));

    if(dialog.getUpdateRate() != updateRateMs)
    {
      // Update rate changed - restart data readers
      dataReaderThread->terminateThread();
      dataReaderThread->setUpdateRate(dialog.getUpdateRate());
      dataReaderThread->start();
    }

    if(dialog.getPort() != port)
    {

      // Restart navserver on port change
      int result2 = QMessageBox::Yes;
      if(navServer->hasConnections())
        result2 = atools::gui::Dialog(this).showQuestionMsgBox(
          lnc::SETTINGS_ACTIONS_SHOW_PORT_CHANGE,
          tr("There are still applications connected.\n"
             "Really change the Network Port?"),
          tr("Do not &show this dialog again."),
          QMessageBox::Yes | QMessageBox::No,
          QMessageBox::No, QMessageBox::Yes);

      if(result2 == QMessageBox::Yes)
      {
        navServer->stopServer();
        navServer->setPort(dialog.getPort());
        navServer->startServer(dataReaderThread);
      }
    }
  }
  else
    qDebug(atools::fs::ns::gui) << Q_FUNC_INFO << "options not accepted";

  qDebug(atools::fs::ns::gui) << Q_FUNC_INFO << "options exit";
}

void MainWindow::resetMessages()
{
  Settings& settings = Settings::instance();
  settings.setValue(lnc::SETTINGS_ACTIONS_SHOW_QUIT, true);
  settings.setValue(lnc::SETTINGS_ACTIONS_SHOW_PORT_CHANGE, true);
}

void MainWindow::logGuiMessage(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
  const static QString MESSAGEPATTERN("[%1] %2");

  if(type == QtFatalMsg)
    // Fatal is a crash anyway - bail out to avoid follow up errors
    return;

  // Do not print debug messages in release mode
#ifdef QT_NO_DEBUG
  if(type == QtDebugMsg)
    return;

#endif

  QString htmlMessage;
  if(context.category != nullptr && QString(context.category) == "gui")
  {
    // Define colors
    switch(type)
    {
      case QtDebugMsg:
        htmlMessage = atools::util::HtmlBuilder::textMessage(message, atools::util::html::NO_ENTITIES, Qt::darkGray);
        break;

      case QtWarningMsg:
        htmlMessage = atools::util::HtmlBuilder::warningMessage(message);
        break;

      case QtFatalMsg:
      case QtCriticalMsg:
        htmlMessage = atools::util::HtmlBuilder::errorMessage(message);
        break;

      case QtInfoMsg:
        htmlMessage = atools::util::HtmlBuilder::textMessage(message, atools::util::html::NO_ENTITIES);
        break;
    }

    // Add five last messages to tray
    if(type != QtDebugMsg && trayIcon != nullptr)
    {
      QStringList tooltip = trayIcon->toolTip().split('\n');
      tooltip.append(tr("- %1").arg(QTextDocumentFragment::fromHtml(message).toPlainText()));
      if(tooltip.size() > 5)
        tooltip.removeFirst();
      trayIcon->setToolTip(tooltip.join('\n'));
    }

    // Use a signal to update the text edit in the main thread context
    emit appendLogMessage(MESSAGEPATTERN.arg(QDateTime::currentDateTime().toString("yyyy-MM-dd h:mm:ss")).arg(htmlMessage));
  }
}

void MainWindow::postLogMessage(QString message, bool warning, bool error)
{
  if(error)
    qCritical(atools::fs::ns::gui).noquote().nospace() << message;
  else if(warning)
    qWarning(atools::fs::ns::gui).noquote().nospace() << message;
  else
    qInfo(atools::fs::ns::gui).noquote().nospace() << message;
}

void MainWindow::showInitial()
{
  // Start timer early to update timestamp and avoid double instances
  dataExchange->startTimer();

  if(ui->actionStartMinimizeTray->isChecked() && trayIcon != nullptr)
    hide();
  else
    show();

  if(dataExchange != nullptr)
    connect(dataExchange, &atools::gui::DataExchange::dataFetched, this, &MainWindow::dataExchangeDataFetched);

  QTimer::singleShot(10, this, &MainWindow::mainWindowShownDelayed);
}

void MainWindow::restoreState()
{
  qDebug() << Q_FUNC_INFO;

  verbose = Settings::instance().getAndStoreValue(lnc::SETTINGS_OPTIONS_VERBOSE, false).toBool();

  atools::gui::WidgetState widgetState(lnc::SETTINGS_MAINWINDOW_WIDGET);
  widgetState.restore(this);
  windowPosition = pos(); // Remember position to set in mainWindowShownDelayed()

  widgetState.setBlockSignals(true);
  widgetState.restore({ui->actionMinimizeTray, ui->actionStartMinimizeTray});
}

void MainWindow::saveState() const
{
  qDebug() << Q_FUNC_INFO;

  if(ui != nullptr)
  {
    atools::gui::WidgetState widgetState(lnc::SETTINGS_MAINWINDOW_WIDGET);
    widgetState.save({this, ui->actionMinimizeTray, ui->actionStartMinimizeTray});
    widgetState.syncSettings();
  }
}

void MainWindow::mainWindowShownDelayed()
{
  qDebug() << Q_FUNC_INFO;
  qDebug(atools::fs::ns::gui) << Q_FUNC_INFO;

  atools::settings::Settings& settings = Settings::instance();

#if defined(WINARCH64)
  QString applicationVersion = QApplication::applicationVersion() + tr(" 64-bit");
#elif defined(WINARCH32)
  QString applicationVersion = QApplication::applicationVersion() + tr(" 32-bit");
#else
  QString applicationVersion = QApplication::applicationVersion();
#endif

  qInfo(atools::fs::ns::gui).noquote().nospace() << QCoreApplication::applicationName();
  qInfo(atools::fs::ns::gui).noquote().nospace() << tr("Version %1 (revision %2).").
    arg(applicationVersion).arg(GIT_REVISION_LITTLENAVCONNECT);

  qInfo(atools::fs::ns::gui).noquote().nospace()
    << tr("Data Version %1. Reply Version %2.").arg(SimConnectData::getDataVersion()).arg(SimConnectReply::getReplyVersion());

  // Build the handler classes which are an abstraction to SimConnect and the Little Xpconnect shared memory
  simConnectHandler = new atools::fs::sc::SimConnectHandler(verbose);

  simConnectHandler->loadSimConnect(activationContext, lnc::SIMCONNECT_DLL_NAME);
  xpConnectHandler = new atools::fs::sc::XpConnectHandler();

#ifdef Q_OS_WIN32
  // Show toolbar with both buttons
  bool fsx = true;

  // Check the first time if SimConnect is available - if yes use FSX settings
  // Otherwise fall back to stored value or X-Plane
  fsx = settings.getAndStoreValue(lnc::SETTINGS_OPTIONS_SIMULATOR_FSX, simConnectHandler->isLoaded()).toBool();

  if(!simConnectHandler->isLoaded())
    // No SimConnect switch to X-Plane
    fsx = false;

  qDebug() << "FSX status" << fsx;

  if(simConnectHandler->isLoaded())
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

  ui->menuWindow->addAction(ui->toolBar->toggleViewAction());

  // Build the thread which will read the data from the interfaces
  dataReaderThread = new atools::fs::sc::DataReaderThread(this, verbose);
  dataReaderThread->setHandler(handlerForSelection());
  handlerChanged();

  dataReaderThread->setReconnectRateSec(settings.getAndStoreValue(lnc::SETTINGS_OPTIONS_RECONNECT_RATE, 15).toInt());
  dataReaderThread->setUpdateRate(settings.getAndStoreValue(lnc::SETTINGS_OPTIONS_UPDATE_RATE, 500).toUInt());
  dataReaderThread->setLoadReplayFilepath(loadReplayFile);
  dataReaderThread->setSaveReplayFilepath(saveReplayFile);
  dataReaderThread->setReplayWhazzupFile(writeReplayWhazzupFile);
  dataReaderThread->setWhazzupUpdateSeconds(replayWhazzupUpdateSpeed);
  dataReaderThread->setReplaySpeed(replaySpeed);

  atools::fs::sc::Options options = atools::fs::sc::NO_OPTION;
  if(settings.getAndStoreValue(lnc::SETTINGS_OPTIONS_FETCH_AI_AIRCRAFT, true).toBool())
    options |= atools::fs::sc::FETCH_AI_AIRCRAFT;
  if(settings.getAndStoreValue(lnc::SETTINGS_OPTIONS_FETCH_AI_SHIP, true).toBool())
    options |= atools::fs::sc::FETCH_AI_BOAT;
  dataReaderThread->setSimconnectOptions(options);
  dataReaderThread->setAiFetchRadius(atools::geo::nmToKm(settings.getAndStoreValue(lnc::SETTINGS_OPTIONS_FETCH_AI_RADIUS, 105).toInt()));

  connect(dataReaderThread, &atools::fs::sc::DataReaderThread::postLogMessage, this, &MainWindow::postLogMessage);

  qInfo(atools::fs::ns::gui).noquote().nospace() << tr("Starting server. This can take some time ...");

  // Set remembered position and make sure that it is visible on screen
  atools::gui::Application::processEventsExtended();
  move(windowPosition);
  atools::gui::util::ensureVisibility(this);

  QGuiApplication::setOverrideCursor(Qt::WaitCursor);

  dataReaderThread->start();

  navServer->startServer(dataReaderThread);

  QGuiApplication::restoreOverrideCursor();

  qInfo(atools::fs::ns::gui).noquote().nospace() << tr("Server running.");
  qDebug(atools::fs::ns::gui) << Q_FUNC_INFO << "exit";

  // Log startup time
  Application::setStartupFinished(Q_FUNC_INFO);
}

void MainWindow::showEvent(QShowEvent *)
{
  updateTrayActions();
}

void MainWindow::hideEvent(QHideEvent *)
{
  windowPosition = pos();
  updateTrayActions();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
  // Catch all close events like Ctrl-Q or Menu/Exit or clicking on the
  // close button on the window frame
  qDebug() << Q_FUNC_INFO;
  bool askClose = false;
  if(windowCloseButtonClicked)
  {
    // Close button on window frame clicked ================================================

    // Check if tray is valid in case OS does not support it
    if(trayIcon != nullptr && trayIcon->isVisible() && ui->actionMinimizeTray->isChecked())
    {
      // Show hint only once per session and not if startup option is checked
      if(!trayHintShown && !ui->actionStartMinimizeTray->isChecked())
      {
        atools::gui::Dialog(this).showInfoMsgBox(lnc::SETTINGS_ACTIONS_SHOW_TRAY_HINT,
                                                 tr("The program will keep running in the system tray.\n"
                                                    "Select \"Quit\" in the context menu of the system tray entry to terminate the program."),
                                                 tr("Do not &show this dialog again."));

        // Show only once per session
        trayHintShown = true;
      }
    } // else keep closing
    else
      // No tray - close as usual
      askClose = true;
  }
  else
    // From tray menu or close action
    askClose = true;

  if(askClose)
  {
    if(askCloseApplication())
    {
      // Required here since QApplication::quitOnLastWindowClosed() is set to false
      saveState();
      deInit();
      QApplication::quit();
    }
    else
      // Do not quit
      event->ignore();
  }

  windowCloseButtonClicked = true;
  saveState();
}

void MainWindow::closeFromSignal()
{
  if(askCloseApplication())
    QApplication::quit();
}

bool MainWindow::askCloseApplication()
{
  if(navServer->hasConnections())
  {
    int result = atools::gui::Dialog(this).showQuestionMsgBox(lnc::SETTINGS_ACTIONS_SHOW_QUIT,
                                                              tr("There are still applications connected.\n"
                                                                 "Really Quit?"),
                                                              tr("Do not &show this dialog again."),
                                                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No, QMessageBox::Yes);

    return result == QMessageBox::Yes;
  }

  return true;
}

void MainWindow::updateTrayActions()
{
  if(trayRestoreHideAction != nullptr)
    trayRestoreHideAction->setText(isVisible() ? tr("&Hide Window") : tr("&Restore Window"));
}

void MainWindow::quitFromTrayOrAction()
{
  // Signal for close event
  windowCloseButtonClicked = false;

  if(askCloseApplication())
    QApplication::quit();
}

void MainWindow::showHideFromTrayAction()
{
  qDebug() << Q_FUNC_INFO;

  setVisible(!isVisible());

  QTimer::singleShot(10, [this]()->void {
    if(isVisible())
    {
      // Set remembered position and make sure that it is visible on screen
      atools::gui::Application::processEventsExtended();
      move(windowPosition);
      atools::gui::util::ensureVisibility(this);
    }
  });
}

void MainWindow::deleteTrayIcon()
{
  ATOOLS_DELETE_LOG(trayIcon);
  ATOOLS_DELETE_LOG(trayIconMenu);

  // Icon menu also deletes action
  trayRestoreHideAction = nullptr;
}

void MainWindow::createTrayIcon()
{
  if(trayIcon == nullptr && QSystemTrayIcon::isSystemTrayAvailable())
  {
    // Context menu takes ownership of actions
    trayIconMenu = new QMenu(this);

    // Copy text and icon from main but not shortcuts
    QAction *trayOptionsAction = new QAction(ui->actionOptions->icon(), ui->actionOptions->text(), trayIconMenu);
    QAction *trayHelpAction = new QAction(ui->actionHelp->icon(), ui->actionHelp->text(), trayIconMenu);
    QAction *trayQuitAction = new QAction(ui->actionQuit->icon(), ui->actionQuit->text(), trayIconMenu);

    // Use member variable to allow changing text
    trayRestoreHideAction = new QAction(tr("&Restore"), trayIconMenu); // Text toggles depending on window state

    trayIconMenu->addAction(trayRestoreHideAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(trayOptionsAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(trayHelpAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(trayQuitAction);

    // Create tray
    trayIcon = new QSystemTrayIcon(QIcon(":/littlenavconnect/resources/icons/navconnect.svg"), this);
    trayIcon->setContextMenu(trayIconMenu); // trayIcon does not take ownership

    connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::trayActivated);
    connect(trayRestoreHideAction, &QAction::triggered, this, &MainWindow::showHideFromTrayAction);
    connect(trayOptionsAction, &QAction::triggered, this, &MainWindow::options);
    connect(trayHelpAction, &QAction::triggered, this, &MainWindow::showOnlineHelp);
    connect(trayQuitAction, &QAction::triggered, this, &MainWindow::quitFromTrayOrAction);

    trayIcon->show();
  }
}

void MainWindow::trayActivated(QSystemTrayIcon::ActivationReason reason)
{
  qDebug() << Q_FUNC_INFO;

  // Toggle main window visibility on click
  if(reason == QSystemTrayIcon::Trigger)
    showHideFromTrayAction();
}

void MainWindow::actionTrayToggled(bool)
{
  if(ui->actionMinimizeTray->isChecked() || ui->actionStartMinimizeTray->isChecked())
    createTrayIcon();
  else if(!ui->actionMinimizeTray->isChecked() && !ui->actionStartMinimizeTray->isChecked())
    deleteTrayIcon();
}
