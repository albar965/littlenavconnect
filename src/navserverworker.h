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

#ifndef LITTLENAVCONNECT_NAVSERVERTHREAD_H
#define LITTLENAVCONNECT_NAVSERVERTHREAD_H

#include "fs/sc/simconnectdata.h"

#include <QHostInfo>

class NavServer;
class QTcpSocket;

class NavServerWorker :
  public QObject
{
  Q_OBJECT

public:
  NavServerWorker(qintptr socketDescriptor, NavServer *parent, bool verboseLog);
  virtual ~NavServerWorker();

  void postSimConnectData(atools::fs::sc::SimConnectData dataPacket);

  void threadStarted();

private:
  qintptr socketDescr;
  atools::fs::sc::SimConnectData data;
  QTcpSocket *socket = nullptr;

  const int MAX_DROPPED_PACKAGES = 20;
  int droppedPackages = 0;
  bool verbose = false;
  bool inPost = false;
  int lastPacketId = -1;
  QString peerAddr;
  QHostInfo hostInfo;
  bool readReply = true;
  void socketDisconnected();
  void readyRead();
  void bytesWritten(qint64 bytes);

  void handleDropped(const QString& reason);

};

#endif // LITTLENAVCONNECT_NAVSERVERTHREAD_H
