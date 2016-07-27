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
    connect(socket, &QTcpSocket::readyRead, this, &NavServerWorker::readyReadReply);
  }

  if(!socket->setSocketDescriptor(socketDescr, QAbstractSocket::ConnectedState, QIODevice::ReadWrite))
  {
    qCritical(gui).noquote().nospace() << tr("Error creating network socket: %1.").arg(socket->errorString());
    return;
  }

  peerAddr = socket->peerAddress().toString();
  hostInfo = QHostInfo::fromName(peerAddr);

  qInfo(gui).noquote().nospace() << tr("Connection from %1 (%2).").arg(hostInfo.hostName()).arg(peerAddr);

  qDebug() << "Connection from " << hostInfo.hostName() << " (" << peerAddr << ") "
           << "port " << socket->peerPort();
}

void NavServerWorker::socketDisconnected()
{
  qInfo(gui).noquote().nospace() << tr("Connection from %1 (%2) closed.").
  arg(hostInfo.hostName()).arg(peerAddr);

  socket->deleteLater();
  socket = nullptr;
  thread()->exit();
}

void NavServerWorker::readyReadReply()
{
  if(verbose)
    qDebug() << "Ready read" << QThread::currentThread()->objectName();

  // Read the reply from littlenavmap
  atools::fs::sc::SimConnectReply reply;
  if(!reply.read(socket))
    handleDroppedPackages(tr("Incomplete reply"));
  else
    // Indicate that a reply was successfully read
    readReply = true;

  if(reply.getStatus() != atools::fs::sc::OK)
  {
    // Not fully read or malformed  content
    qWarning(gui).noquote().nospace() << tr("Error reading reply: %1. Closing connection.").
    arg(reply.getStatusText());
    socket->abort();
  }
}

void NavServerWorker::postSimConnectData(atools::fs::sc::SimConnectData dataPacket)
{
  if(verbose)
    qDebug() << "postSimConnectData" << QThread::currentThread()->objectName();

  if(!readReply)
  {
    // No reply received in the meantime - count it as dropped package
    handleDroppedPackages(tr("Missing reply"));
    return;
  }

  if(inPost)
    // We're already posting
    qCritical() << "Nested post";

  if(dataPacket.getPacketId() == lastPacketId)
    qCritical() << "Duplicate packet in post";

  lastPacketId = dataPacket.getPacketId();
  inPost = true;
  readReply = false;

  int written;
  written = dataPacket.write(socket);
  if(dataPacket.getStatus() != atools::fs::sc::OK)
    qWarning(gui).noquote().nospace() << tr("Error writing data: %1.").arg(dataPacket.getStatusText());

  if(!socket->flush())
    qWarning() << "Reply to client not flushed";

  if(verbose)
    qDebug() << "written" << written << "flush" << flush;

  inPost = false;
}

void NavServerWorker::handleDroppedPackages(const QString& reason)
{
  droppedPackages++;
  if(droppedPackages > MAX_DROPPED_PACKAGES)
  {
    qWarning(gui).noquote().nospace() << tr("Dropped more than %1 packages. Reason: %2. "
                                            "Increase update time interval.").
    arg(MAX_DROPPED_PACKAGES).arg(reason);

    droppedPackages = 0;
  }
  qWarning() << "No reply - ignoring package. Currently dropped" << droppedPackages << "Reason:" << reason;
}
