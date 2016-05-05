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

#include "navserver.h"
#include "navserverthread.h"
#include "common.h"
#include "fs/simconnectdata.h"

#include <QtNetwork>

NavServerThread::NavServerThread(qintptr socketDescriptor, NavServer *parent)
  : QThread(parent), socket(socketDescriptor), server(parent)
{
  qDebug() << "NavServerThread created" << objectName();
}

NavServerThread::~NavServerThread()
{
  qDebug() << "NavServerThread deleted" << objectName();
}

void NavServerThread::run()
{
  QTcpSocket tcpSocket;
  if(!tcpSocket.setSocketDescriptor(socket))
  {
    qCritical(gui) << "Error creating socket" << tcpSocket.errorString();
    return;
  }
  QString peerAddr = tcpSocket.peerAddress().toString();
  QHostInfo hostInfo = QHostInfo::fromName(peerAddr);

  qInfo(gui).noquote().nospace() << "Connection from " << hostInfo.hostName()
                                 << " (" << peerAddr << ") "
                                 << "port " << tcpSocket.peerPort();

  atools::fs::SimConnectData dataPacket;

  while(!terminate)
  {
    mutex.lock();
    waitCondition.wait(&mutex);
    dataPacket = data;
    mutex.unlock();

    if(!tcpSocket.isOpen())
    {
      qInfo(gui).noquote().nospace() << "Connection from " << hostInfo.hostName()
                                     << " (" << peerAddr << ") " << " closed by peer";
      break;
    }

    dataPacket.write(&tcpSocket);
    tcpSocket.flush();
  }
  tcpSocket.disconnectFromHost();

  if(tcpSocket.state() != QAbstractSocket::UnconnectedState)
    tcpSocket.waitForDisconnected(10000);
}

void NavServerThread::postMessage(const atools::fs::SimConnectData& dataPacket)
{
  QMutexLocker locker(&mutex);
  data = dataPacket;
  waitCondition.wakeAll();
}

void NavServerThread::setTerminate()
{
  QMutexLocker locker(&mutex);
  terminate = true;
  waitCondition.wakeAll();
}
