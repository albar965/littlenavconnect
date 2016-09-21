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

#include "ui_mainwindow.h"
#include "navserver.h"
#include "datareaderthread.h"
#include "settings/settings.h"
#include "navservercommon.h"
#include "optionsdialog.h"

#include "gui/dialog.h"
#include "gui/helphandler.h"
#include "gui/widgetstate.h"
#include "logging/logginghandler.h"
#include "fs/sc/simconnectreply.h"

#include <QMessageBox>
#include <QCloseEvent>

static QString ABOUT_MESSAGE =
  QObject::tr("<p>is the Fligh Simulator Network agent for Little Navmap.</p>"
                "<p>This software is licensed under "
                  "<a href=\"http://www.gnu.org/licenses/gpl-3.0\">GPL3</a> or any later version.</p>"
                    "<p>The source code for this application is available at "
                      "<a href=\"https://github.com/albar965\">Github</a>.</p>"
                        "<p>More about my projects at "
                          "<a href=\"https://albar965.github.io\">albar965.github.io</a>.</p>"
                            "<p><b>Copyright 2015-2016 Alexander Barthel</b></p> "
                              "<p><a href=\"mailto:albar965@mailbox.org\">albar965@mailbox.org</a> or "
                                "<a href=\"mailto:albar965@t-online.de\">albar965@t-online.de</a></p>");

using atools::settings::Settings;
using atools::fs::sc::SimConnectData;
using atools::fs::sc::SimConnectReply;

MainWindow::MainWindow()
  : ui(new Ui::MainWindow)
{
  ui->setupUi(this);
  readSettings();

  // Bind the log function to this class for category "gui"
  using namespace std::placeholders;
  atools::logging::LoggingHandler::setLogFunction(std::bind(&MainWindow::logGuiMessage, this, _1, _2, _3));

  // Create help handler for managing the Help menu items
  helpHandler = new atools::gui::HelpHandler(this, ABOUT_MESSAGE, GIT_REVISION);

  // Create nav server but to not start it yet
  navServer = new NavServer(this, verbose,
                            Settings::instance().getAndStoreValue(SETTINGS_OPTIONS_DEFAULT_PORT, 51968).toInt());

  connect(ui->actionQuit, &QAction::triggered, this, &QMainWindow::close);
  connect(ui->actionResetMessages, &QAction::triggered, this, &MainWindow::resetMessages);
  connect(ui->actionOptions, &QAction::triggered, this, &MainWindow::options);
  connect(ui->actionContents, &QAction::triggered, helpHandler, &atools::gui::HelpHandler::help);
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
  // Terminate data reader thread
  dataReader->setTerminate();
  dataReader->wait();

  atools::logging::LoggingHandler::setLogFunction(nullptr);

  delete helpHandler;
  delete ui;
}

void MainWindow::options()
{
  OptionsDialog dialog;

  Settings& settings = Settings::instance();
  unsigned int updateRateMs = settings.getAndStoreValue(SETTINGS_OPTIONS_UPDATE_RATE, 500).toUInt();
  int port = settings.getAndStoreValue(SETTINGS_OPTIONS_DEFAULT_PORT, 51968).toInt();

  dialog.setUpdateRate(updateRateMs);
  dialog.setPort(port);

  int result = dialog.exec();

  if(result == QDialog::Accepted)
  {
    if(dialog.getUpdateRate() != updateRateMs)
    {
      settings.setValue(SETTINGS_OPTIONS_UPDATE_RATE, static_cast<int>(dialog.getUpdateRate()));

      // Update rate changed - restart data readers
      dataReader->setTerminate();
      dataReader->wait();
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
        settings.setValue(SETTINGS_OPTIONS_DEFAULT_PORT, dialog.getPort());

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

void MainWindow::readSettings()
{
  qDebug() << "readSettings";

  verbose = Settings::instance().getAndStoreValue(SETTINGS_OPTIONS_VERBOSE, false).toBool();

  atools::gui::WidgetState(SETTINGS_MAINWINDOW_WIDGET).restore(this);
}

void MainWindow::writeSettings()
{
  qDebug() << "writeSettings";

  atools::gui::WidgetState widgetState(SETTINGS_MAINWINDOW_WIDGET);
  widgetState.save(this);
  widgetState.syncSettings();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
  // Catch all close events like Ctrl-Q or Menu/Exit or clicking on the
  // close button on the window frame
  qDebug() << "closeEvent";

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
  qDebug() << "MainWindow::mainWindowShown()";

  qInfo(gui).noquote().nospace() << QApplication::applicationName();
  qInfo(gui).noquote().nospace() << tr("Version %1 (revision %2).").
  arg(QApplication::applicationVersion()).arg(GIT_REVISION);

  qInfo(gui).noquote().nospace()
  << tr("Data Version %1. Reply Version %2.").arg(SimConnectData::getDataVersion()).arg(
    SimConnectReply::getReplyVersion());

  dataReader = new DataReaderThread(this, verbose);
  dataReader->setReconnectRateSec(Settings::instance().
                                  getAndStoreValue(SETTINGS_OPTIONS_RECONNECT_RATE, 10).toInt());
  dataReader->setUpdateRate(Settings::instance().
                            getAndStoreValue(SETTINGS_OPTIONS_UPDATE_RATE, 500).toUInt());

  qInfo(gui).noquote().nospace() << tr("Starting server ...");

  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

  dataReader->start();

  navServer->startServer(dataReader);

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
