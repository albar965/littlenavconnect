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

#include "navservercommon.h"
#include "navserverworker.h"
#include "datareaderthread.h"
#include "settings/settings.h"

#include <QNetworkInterface>
#include <QHostInfo>

NavServer::NavServer(QObject *parent, bool verboseLog, int inetPort)
  : QTcpServer(parent), verbose(verboseLog), port(inetPort)
{
  qDebug("NavServer created");
}

NavServer::~NavServer()
{
  stopServer();
}

void NavServer::stopServer()
{
  qDebug() << "Navserver stopping";

  // Close tcp server to avoid accepting connections
  close();

  // Stop all worker threads
  QSet<NavServerWorker *> workersCopy(workers);
  for(NavServerWorker *worker : workersCopy)
  {
    worker->thread()->exit();
    worker->thread()->wait();
  }

  qDebug("NavServer deleted");
}

bool NavServer::startServer(DataReaderThread *dataReaderThread)
{
  dataReader = dataReaderThread;
  qDebug() << "Navserver starting";

  bool retval = listen(QHostAddress::AnyIPv4, static_cast<quint16>(port));

  // Get an IP address of this machine that is not localhost
  QString ipAddress;
  QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
  for(const QHostAddress& ip : ipAddressesList)
  {
    if(ip != QHostAddress::LocalHost && ip.toIPv4Address() > 0)
    {
      ipAddress = ip.toString();
      break;
    }
  }
  // if we did not find one, use IPv4 localhost
  if(ipAddress.isEmpty())
    ipAddress = QHostAddress(QHostAddress::LocalHost).toString();

  // Get hostname synchronously
  QHostInfo hostInfo = QHostInfo::fromName(ipAddress);

  if(!retval)
    qCritical(gui).noquote().nospace() << tr("Unable to start the server: %1.").arg(errorString());
  else
    qInfo(gui).noquote().nospace()
    << tr("Server is running on hostname "
          "<span style=\"color: #0000ff; font-weight:bold\">%1</span> "
            "(IP address <span style=\"color: #0000ff; font-weight:bold\">%2</span>) "
            "port <span style=\"color: #ff0000; font-weight:bold\">%3</span>.").
    arg(hostInfo.hostName()).arg(ipAddress).arg(serverPort());
  return retval;
}

void NavServer::incomingConnection(qintptr socketDescriptor)
{
  qDebug() << "Incoming connection";

  // Create a worker and set name
  NavServerWorker *worker = new NavServerWorker(socketDescriptor, nullptr, verbose);
  worker->setObjectName("SocketWorker-" + QString::number(socketDescriptor));

  // Create new thread and move the worker into the thread context
  // This allows to receive signals in the thread context instead the sender's context
  QThread *workerThread = new QThread(this);
  workerThread->setObjectName("SocketWorkerThread-" + QString::number(socketDescriptor));
  worker->moveToThread(workerThread);

  connect(workerThread, &QThread::started, worker, &NavServerWorker::threadStarted);
  connect(workerThread, &QThread::finished, [ = ]()->void
          {
            threadFinished(worker);
          });

  // Data reader will send simconnect packages through this connection
  connect(dataReader, &DataReaderThread::postSimConnectData, worker, &NavServerWorker::postSimConnectData);

  qDebug() << "Thread" << worker->objectName();
  workerThread->start();

  workers.insert(worker);
}

void NavServer::threadFinished(NavServerWorker *worker)
{
  qDebug() << "Thread" << worker->objectName() << "finished";

  // A thread has finished - lock the list so the thread can be removed from the list
  QMutexLocker locker(&threadsMutex);

  disconnect(dataReader, &DataReaderThread::postSimConnectData, worker, &NavServerWorker::postSimConnectData);

  workers.remove(worker);

  // Delete once the event loop is called the next time
  worker->deleteLater();
  worker->thread()->deleteLater();
}

bool NavServer::hasConnections() const
{
  QMutexLocker locker(&threadsMutex);
  return workers.size() > 0;
}
