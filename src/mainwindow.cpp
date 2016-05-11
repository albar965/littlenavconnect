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
#include "net/navserver.h"
#include "datareaderthread.h"
#include "ui_mainwindow.h"
#include "settings/settings.h"
#include "common.h"
#include "optionsdialog.h"

#include <gui/dialog.h>
#include <gui/helphandler.h>
#include <gui/widgetstate.h>

#include <QCloseEvent>
#include <QDateTime>
#include <QThread>
#include <QSettings>
#include <logging/logginghandler.h>

using atools::settings::Settings;

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent), ui(new Ui::MainWindow)
{
  ui->setupUi(this);
  readSettings();

  using namespace std::placeholders;
  atools::logging::LoggingHandler::setLogFunction(std::bind(&MainWindow::logGuiMessage, this, _1, _2, _3));

  QString aboutMessage =
    tr("<p>is the FSX Network connector for Little Navmap.</p>"
         "<p>This software is licensed under "
           "<a href=\"http://www.gnu.org/licenses/gpl-3.0\">GPL3</a> or any later version.</p>"
             "<p>The source code for this application is available at "
               "<a href=\"https://github.com/albar965\">Github</a>.</p>"
                 "<p><b>Copyright 2015-2016 Alexander Barthel (albar965@mailbox.org).</b></p>");

  helpHandler = new atools::gui::HelpHandler(this, aboutMessage, GIT_REVISION);
  navServer = new NavServer(this);

  connect(ui->actionQuit, &QAction::triggered, this, &QMainWindow::close);
  connect(ui->actionResetMessages, &QAction::triggered, this, &MainWindow::resetMessages);
  connect(ui->actionOptions, &QAction::triggered, this, &MainWindow::options);
  connect(ui->actionContents, &QAction::triggered, helpHandler, &atools::gui::HelpHandler::help);
  connect(ui->actionAbout, &QAction::triggered, helpHandler, &atools::gui::HelpHandler::about);
  connect(ui->actionAboutQt, &QAction::triggered, helpHandler, &atools::gui::HelpHandler::aboutQt);
  connect(this, &MainWindow::appendLogMessage, ui->textEdit, &QTextEdit::append);

  connect(this, &MainWindow::windowShown, this, &MainWindow::mainWindowShown, Qt::QueuedConnection);

}

MainWindow::~MainWindow()
{
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
  unsigned int updateRateMs = settings.getAndStoreValue("Options/UpdateRate", 500).toUInt();
  int port = settings.getAndStoreValue("Options/DefaultPort", 51968).toInt();

  dialog.setUpdateRate(updateRateMs);
  dialog.setPort(port);

  int result = dialog.exec();

  if(result == QDialog::Accepted)
  {
    if(dialog.getUpdateRate() != updateRateMs)
    {
      settings->setValue("Options/UpdateRate", dialog.getUpdateRate());

      dataReader->setTerminate();
      dataReader->wait();
      dataReader->setUpdateRate(dialog.getUpdateRate());
      dataReader->start();
    }

    if(dialog.getPort() != port)
    {
      int result2 = QMessageBox::Yes;
      if(navServer->hasConnections())
        result2 = atools::gui::Dialog(this).showQuestionMsgBox("Actions/ShowPortChange",
                                                               tr(
                                                                 "There are still applications connected.\n"
                                                                 "Really change the Network Port?"),
                                                               tr("Do not &show this dialog again."),
                                                               QMessageBox::Yes | QMessageBox::No,
                                                               QMessageBox::No, QMessageBox::Yes);

      if(result2 == QMessageBox::Yes)
      {
        settings->setValue("Options/DefaultPort", dialog.getPort());

        navServer->stopServer();
        navServer->setPort(dialog.getPort());
        navServer->startServer();
      }
    }
  }
}

void MainWindow::resetMessages()
{
  Settings& settings = Settings::instance();
  settings->setValue("Actions/ShowQuit", true);
  settings->setValue("Actions/Actions/ShowPortChange", true);
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

  atools::gui::WidgetState ws("MainWindow/Widget");
  ws.restore(this);

}

void MainWindow::writeSettings()
{
  qDebug() << "writeSettings";

  atools::gui::WidgetState ws("MainWindow/Widget");
  ws.save(this);
  ws.syncSettings();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
  // Catch all close events like Ctrl-Q or Menu/Exit or clicking on the
  // close button on the window frame
  qDebug() << "closeEvent";

  if(navServer->hasConnections())
  {
    int result = atools::gui::Dialog(this).showQuestionMsgBox("Actions/ShowQuit",
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
  qInfo(gui).noquote().nospace() << "Version " << QApplication::applicationVersion()
                                 << " (revision " << GIT_REVISION << ")";

  navServer->startServer();

  dataReader = new DataReaderThread(this, navServer);
  dataReader->start();
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
