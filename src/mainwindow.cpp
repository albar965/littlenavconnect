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

#include <gui/helphandler.h>

#include <QThread>
#include <QtConcurrent/QtConcurrentRun>
#include <logging/logginghandler.h>

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent), ui(new Ui::MainWindow)
{
  ui->setupUi(this);

  using namespace std::placeholders;
  atools::logging::LoggingHandler::setLogFunction(std::bind(&MainWindow::logMessage, this, _1, _2, _3));

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
  connect(ui->actionContents, &QAction::triggered, helpHandler, &atools::gui::HelpHandler::help);
  connect(ui->actionAbout, &QAction::triggered, helpHandler, &atools::gui::HelpHandler::about);
  connect(ui->actionAboutQt, &QAction::triggered, helpHandler, &atools::gui::HelpHandler::aboutQt);
  connect(this, &MainWindow::appendLogMessage, ui->textEdit, &QTextEdit::append);
  navServer->startServer();

  dataReader = new DataReaderThread(this, navServer);
  dataReader->start();

}

MainWindow::~MainWindow()
{
  dataReader->setTerminate();
  dataReader->wait();

  atools::logging::LoggingHandler::setLogFunction(nullptr);

  delete helpHandler;
  delete ui;
}

void MainWindow::logMessage(QtMsgType type, const QMessageLogContext& context,
                            const QString& message)
{
  if(context.category != nullptr && QString(context.category) == "gui")
  {
    QString style = "black";
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
        style = "color:black";
        break;
    }
    emit appendLogMessage("<span style=\"" + style + "\">" + message + "</span>");
  }
}
