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
#include "navservercommon.h"
#include "fs/sc/simconnectreply.h"

#include <QThread>
#include <QTcpSocket>

NavServerWorker::NavServerWorker(qintptr socketDescriptor, NavServer *parent, bool verboseLog)
  : QObject(parent), socketDescr(socketDescriptor), verbose(verboseLog)
{
  qDebug() << "NavServerThread created" << QThread::currentThread()->objectName();
}

NavServerWorker::~NavServerWorker()
{
  qDebug() << "NavServerThread deleted" << QThread::currentThread()->objectName();
}

void NavServerWorker::threadStarted()
{
  qDebug() << "threadStarted" << QThread::currentThread()->objectName();

  if(socket == nullptr)
  {
    socket = new QTcpSocket();
    connect(socket, &QTcpSocket::disconnected, this, &NavServerWorker::socketDisconnected);
    connect(socket, &QTcpSocket::readyRead, this, &NavServerWorker::readyRead);
    connect(socket, &QTcpSocket::bytesWritten, this, &NavServerWorker::bytesWritten);
  }

  if(!socket->setSocketDescriptor(socketDescr, QAbstractSocket::ConnectedState, QIODevice::ReadWrite))
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
}

void NavServerWorker::socketDisconnected()
{
  qInfo(gui).noquote().nospace() << "Connection from " << hostInfo.hostName()
                                 << " (" << peerAddr << ") " << " closed.";

  socket->deleteLater();
  socket = nullptr;
  thread()->exit();
}

void NavServerWorker::readyRead()
{
  if(verbose)
    qDebug() << "Ready read" << QThread::currentThread()->objectName();

  atools::fs::sc::SimConnectReply reply;

  if(!reply.read(socket))
    handleDropped("Incomplete reply");
  else
    readReply = true;

  if(reply.getStatus() != atools::fs::sc::OK)
  {
    qWarning(gui).noquote().nospace() << "Error reading reply: " << reply.getStatusText()
                                      << ". Closing connection.";
    socket->abort();
  }

}

void NavServerWorker::bytesWritten(qint64 bytes)
{
  if(verbose)
    qDebug() << "Bytes written" << bytes << "thread" << QThread::currentThread()->objectName();
}

void NavServerWorker::postSimConnectData(atools::fs::sc::SimConnectData dataPacket)
{
  if(verbose)
    qDebug() << "postSimConnectData" << QThread::currentThread()->objectName();

  if(!readReply)
  {
    handleDropped("Missing reply");
    return;
  }

  if(inPost)
    qCritical() << "Nested post";

  if(dataPacket.getPacketId() == lastPacketId)
    qCritical() << "Duplicate packet in post";

  lastPacketId = dataPacket.getPacketId();
  inPost = true;
  readReply = false;

  int written;
  written = dataPacket.write(socket);
  if(dataPacket.getStatus() != atools::fs::sc::OK)
    qWarning(gui).noquote().nospace() << "Error writing data: " << dataPacket.getStatusText();

  if(!socket->flush())
    qWarning() << "Reply to client not flushed";

  if(verbose)
    qDebug() << "written" << written << "flush" << flush;

  inPost = false;
}

void NavServerWorker::handleDropped(const QString& reason)
{
  droppedPackages++;
  if(droppedPackages > MAX_DROPPED_PACKAGES)
  {
    qWarning(gui).noquote().nospace() << "Dropped more than " << MAX_DROPPED_PACKAGES
                                      << " packages. Reason: " << reason << "."
                                      << "Decrease number of updates per second.";
    droppedPackages = 0;
  }
  qWarning() << "No reply - ignoring package. Currently dropped" << droppedPackages << "Reason:" << reason;
}
