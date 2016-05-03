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

#include <QThreadPool>
#include <stdlib.h>

NavServer::NavServer(QObject *parent)
  : QTcpServer(parent)
{
  qDebug("NavServer created");
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

  qDebug("NavServer deleted");
}

bool NavServer::startServer()
{
  bool retval = listen(QHostAddress::AnyIPv4, 51968);

  if(!retval)
    qCritical(gui) << "Unable to start the server:" << errorString();
  else
    qInfo(gui) << "Server is running on IP" << serverAddress().toString()
             << "port" << QString::number(serverPort());

  return retval;
}

void NavServer::incomingConnection(qintptr socketDescriptor)
{
  qDebug() << "Incoming connection";

  NavServerThread *thread = new NavServerThread(socketDescriptor, this);
  thread->setObjectName("SocketWorker-" + QString::number(socketDescriptor));
  threads.insert(thread);

  qDebug() << "Thread" << thread->objectName();

  connect(thread, &NavServerThread::finished, [ = ]()->void
          {
            threadFinished(thread);
          });

  thread->start();
}

void NavServer::threadFinished(NavServerThread *thread)
{
  QMutexLocker locker(&threadsMutex);

  qDebug() << "Thread" << thread->objectName() << "finished";
  threads.remove(thread);

  if(!isTerminating)
    thread->deleteLater();
}

void NavServer::postMessage(const QString& message)
{
  QMutexLocker locker(&threadsMutex);

  for(NavServerThread *thread : threads)
    thread->postMessage(message);
}
