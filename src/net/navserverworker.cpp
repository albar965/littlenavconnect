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

#include <QtNetwork>
#include <QApplication>

NavServerWorker::NavServerWorker(qintptr socketDescriptor, NavServer *parent)
  : QObject(parent), socketDescr(socketDescriptor), server(parent)
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
  qDebug() << "Ready read" << QThread::currentThread()->objectName();

  QByteArray bytes = socket->readAll();

  qDebug() << "Bytes read" << bytes.size();
  readReply = true;
}

void NavServerWorker::bytesWritten(qint64 bytes)
{
  qDebug() << "Bytes written" << bytes << "thread" << QThread::currentThread()->objectName();
}

void NavServerWorker::postSimConnectData(atools::fs::SimConnectData dataPacket)
{
  qDebug() << "postSimConnectData" << QThread::currentThread()->objectName();

  if(!readReply)
  {
    qWarning() << "No reply - ignoring package";
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
  bool flush = socket->flush();
  qDebug() << "written" << written << "flush" << flush;
  inPost = false;
}
