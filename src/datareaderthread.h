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

#ifndef LITTLENAVCONNECT_DATAREADERTHREAD_H
#define LITTLENAVCONNECT_DATAREADERTHREAD_H

#include "fs/sc/simconnectdata.h"

#include <QThread>

class NavServer;
class SimConnectHandler;

/* Actively reads flight simulator data using the simconnect interface in background and sends a
 * signal for each data package. */
class DataReaderThread :
  public QThread
{
  Q_OBJECT

public:
  DataReaderThread(QObject *parent, bool verboseLog);
  virtual ~DataReaderThread();

  /* Thread will terminate after the next iteration. */
  void setTerminate(bool terminateFlag = true);

  /* Read data and send a signal every updateRateMs */
  void setUpdateRate(unsigned int updateRateMs);

  /* If simulator connection is lost try to reconnect every reconnectSec seconds. */
  void setReconnectRateSec(int reconnectSec);

signals:
  /* Send on each received data package from the simconnect interface */
  void postSimConnectData(atools::fs::sc::SimConnectData dataPacket);

private:
  void connectToSimulator(SimConnectHandler *handler);
  virtual void run() override;

  bool terminate = false, verbose = false;
  unsigned int updateRate = 500;
  int reconnectRateSec = 10;
};

#endif // LITTLENAVCONNECT_DATAREADERTHREAD_H
