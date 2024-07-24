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

#ifndef LITTLENAVCONNECT_MAINWINDOW_H
#define LITTLENAVCONNECT_MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>

namespace Ui {
class MainWindow;
}

namespace atools {

namespace util {
class Properties;
}
namespace gui {

class DataExchange;
class HelpHandler;
}
namespace fs {
namespace sc {
class DataReaderThread;
class SimConnectHandler;
class XpConnectHandler;
class ConnectHandler;
}
namespace ns {
class NavServer;

}
}
}

class QActionGroup;

class MainWindow :
  public QMainWindow
{
  Q_OBJECT

public:
  MainWindow();
  virtual ~MainWindow() override;

  /* Connected from DataReaderThread::postLogMessage */
  void postLogMessage(QString message, bool warning, bool error);

  /* Shows or hides the window initally and calls mainWindowShownDelayed() later */
  void showInitial();

  /* Returns true if application should exit */
  static bool initDataExchange();
  static void deInitDataExchange();

signals:
  /* Append a log message to the gui log. */
  void appendLogMessage(const QString& message);

private:
  /* Loggin handler will send log messages of category gui to this method which will emit
   * appendLogMessage to ensure that the message is appended using the main thread context. */
  void logGuiMessage(QtMsgType type, const QMessageLogContext& context, const QString& message);

  virtual void showEvent(QShowEvent *) override;
  virtual void hideEvent(QHideEvent *) override;
  virtual void closeEvent(QCloseEvent *event) override;

  /* Initializes server after window or tray shown */
  void mainWindowShownDelayed();

  void saveState() const;
  void restoreState();

  /* Reset all "do not show again" messages */
  void resetMessages();

  /* Options dialog */
  void options();

  /* Received command line options from another instance */
  void dataExchangeDataFetched(atools::util::Properties properties);

  /* Methods calles from actions */
  void saveReplayFileTriggered();
  void loadReplayFileTriggered();
  void stopReplay();
  void showOnlineHelp();
  void showOfflineHelp();
  void simulatorSelectionTriggered();

  void handlerChanged();

  /* Creates the tray icon and menu */
  void createTrayIcon();
  void deleteTrayIcon();

  /* Clicked */
  void trayActivated(QSystemTrayIcon::ActivationReason reason);

  /* One of the minimize to tray changed */
  void actionTrayToggled(bool);

  /* Returns true if shutdown can be continued */
  bool askCloseApplication();

  /* Either from action quit or tray menu quit */
  void updateTrayActions();
  void showHideFromTray();
  void closeFromTrayOrAction();

  bool trayHintShown = false, /* Show hint only once per session */
       windowCloseButtonClicked = true; /* Avoid close to tray notification in closeEvent */

  Ui::MainWindow *ui = nullptr;

  /* Tray and menus */
  QSystemTrayIcon *trayIcon = nullptr;
  QMenu *trayIconMenu = nullptr;
  QAction *trayRestoreHideAction = nullptr;

  // Navserver that waits and accepts tcp connections. Starts a NavServerWorker in a thread for each connection.
  atools::fs::ns::NavServer *navServer = nullptr;

  // Runs in background and fetches data from simulator - signals are sent to NavServerWorker threads
  atools::fs::sc::DataReaderThread *dataReader = nullptr;
  atools::fs::sc::SimConnectHandler *fsxConnectHandler = nullptr;
  atools::fs::sc::XpConnectHandler *xpConnectHandler = nullptr;
  QActionGroup *simulatorActionGroup = nullptr;
  atools::fs::sc::ConnectHandler *handlerForSelection();

  atools::gui::HelpHandler *helpHandler = nullptr;
  bool firstStart = true; // Used to emit the first windowShown signal
  bool verbose = false;

  QString mainWindowTitle, writeReplayWhazzupFile, supportedLanguageOnlineHelp, aboutMessage;

  QString saveReplayFile, loadReplayFile;
  int replaySpeed = 1, replayWhazzupUpdateSpeed = 15;

  static atools::gui::DataExchange *dataExchange;

};

#endif // LITTLENAVCONNECT_MAINWINDOW_H
