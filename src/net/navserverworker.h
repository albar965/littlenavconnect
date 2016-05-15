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

#ifndef NAVSERVERTHREAD_H
#define NAVSERVERTHREAD_H

#include <QThread>
#include <QWaitCondition>
#include <QMutex>
#include <QAbstractSocket>
#include <QHostInfo>

#include "fs/simconnectdata.h"

class NavServer;
class QTcpSocket;

class NavServerWorker :
  public QObject
{
  Q_OBJECT

public:
  NavServerWorker(qintptr socketDescriptor, NavServer *parent);
  virtual ~NavServerWorker();

  void postMessage(const atools::fs::SimConnectData& dataPacket);

  void setTerminate();

  void work();

private:
  bool terminate = false;
  qintptr socketDescr;
  NavServer *server;
  atools::fs::SimConnectData data;
  QWaitCondition waitCondition;
  mutable QMutex mutex;
  QTcpSocket *socket = nullptr;

  QWaitCondition waitReadCondition;
  mutable QMutex mutexRead;

  QString peerAddr;
  QHostInfo hostInfo;

  void socketDisconnected();
  void readyRead();
  void bytesWritten(qint64 bytes);

};

#endif // NAVSERVERTHREAD_H
