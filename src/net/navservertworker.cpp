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
#include "navserverworker.h"
#include "common.h"
#include "fs/simconnectdata.h"

#include <QtNetwork>

NavServerWorker::NavServerWorker(qintptr socketDescriptor, NavServer *parent)
  : QObject(parent), socketDescr(socketDescriptor), server(parent)
{
  qDebug() << "NavServerThread created" << objectName();
}

NavServerWorker::~NavServerWorker()
{
  qDebug() << "NavServerThread deleted" << objectName();
}

void NavServerWorker::work()
{
  if(socket == nullptr)
  {
    socket = new QTcpSocket();
    connect(socket, &QTcpSocket::disconnected, this, &NavServerWorker::socketDisconnected);
    connect(socket, &QTcpSocket::readyRead, this, &NavServerWorker::readyRead);
    connect(socket, &QTcpSocket::bytesWritten, this, &NavServerWorker::bytesWritten);
  }

  if(!socket->setSocketDescriptor(socketDescr))
  {
    qCritical(gui).noquote().nospace() << "Error creating network socket " << socket->errorString() << ".";
    return;
  }
  peerAddr = socket->peerAddress().toString();
  hostInfo = QHostInfo::fromName(peerAddr);

  qInfo(gui).noquote().nospace() << "Connection from " << hostInfo.hostName()
                                 << " (" << peerAddr << ")";
  qDebug() << "Connection from " << hostInfo.hostName()
           << " (" << peerAddr << ") "
           << "port " << socket->peerPort();

  atools::fs::SimConnectData dataPacket;

  while(!terminate)
  {
    mutex.lock();
    bool waitOk = waitCondition.wait(&mutex, 2000);
    dataPacket = data;
    mutex.unlock();

    if(socket == nullptr || !socket->isOpen())
      break;

    if(waitOk)
      dataPacket.write(socket);
    else
      atools::fs::SimConnectData().write(socket);
    socket->flush();

    // waitReadCondition.wait(&mutexRead);
  }

  if(socket != nullptr && socket->isOpen())
    socket->disconnectFromHost();
}

void NavServerWorker::socketDisconnected()
{
  qInfo(gui).noquote().nospace() << "Connection from " << hostInfo.hostName()
                                 << " (" << peerAddr << ") " << " closed.";

  setTerminate();
  wait();

  socket->deleteLater();
  socket = nullptr;
  thread()->exit();
}

void NavServerWorker::readyRead()
{
  qDebug() << "Ready read";

  QByteArray bytes = socket->readAll();
  qDebug() << "Bytes read" << bytes.size();

  // waitReadCondition.wakeAll();
}

void NavServerWorker::bytesWritten(qint64 bytes)
{
  qDebug() << "Bytes written" << bytes;
}

void NavServerWorker::postMessage(const atools::fs::SimConnectData& dataPacket)
{
  QMutexLocker locker(&mutex);
  data = dataPacket;
  waitCondition.wakeAll();
}

void NavServerWorker::setTerminate()
{
  QMutexLocker locker(&mutex);
  terminate = true;
  waitCondition.wakeAll();
}
