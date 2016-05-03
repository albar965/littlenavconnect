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

#include <QThreadPool>
#include <stdlib.h>

NavServer::NavServer(QObject *parent)
  : QTcpServer(parent)
{
  log("NavServer created", QtInfoMsg);
}

NavServer::~NavServer()
{
  isTerminating = true;
  QSet<NavServerThread *> threadsCopy(threads);
  for(NavServerThread *thread : threadsCopy)
  {
    thread->setTerminate();
    thread->wait();
  }

  log("NavServer deleted", QtInfoMsg);
}

bool NavServer::startServer()
{
  bool retval = listen(QHostAddress::AnyIPv4, 51968);

  if(!retval)
    log("Unable to start the server: " + errorString(), QtCriticalMsg);
  else
    log("Server is running on IP " + serverAddress().toString() +
        " port " + QString::number(serverPort()), QtInfoMsg);

  return retval;
}

void NavServer::incomingConnection(qintptr socketDescriptor)
{
  log("Incoming connection", QtInfoMsg);

  NavServerThread *thread = new NavServerThread(socketDescriptor, this);
  thread->setObjectName("SocketWorker-" + QString::number(socketDescriptor));
  threads.insert(thread);

  log("Thread " + thread->objectName(), QtInfoMsg);

  connect(thread, &NavServerThread::finished, [ = ]()->void
          {
            threadFinished(thread);
          });

  thread->start();
}

void NavServer::threadFinished(NavServerThread *thread)
{
  QMutexLocker locker(&threadsMutex);

  log("Thread " + thread->objectName() + " finished", QtInfoMsg);
  threads.remove(thread);

  if(!isTerminating)
    thread->deleteLater();
}

void NavServer::log(const QString& message, int type)
{
  QtMsgType msgType = static_cast<QtMsgType>(type);
  QDebug dbg(msgType);
  dbg << message;
  emit logMessage(message, msgType);
}

void NavServer::postMessage(const QString& message)
{
  QMutexLocker locker(&threadsMutex);

  for(NavServerThread *thread : threads)
    thread->postMessage(message);
}
