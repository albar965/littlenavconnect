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

#include <QNetworkInterface>
#include <QHostInfo>
#include <datareaderthread.h>

#include <settings/settings.h>

NavServer::NavServer(QObject *parent, bool verboseLog)
  : QTcpServer(parent), verbose(verboseLog)
{
  qDebug("NavServer created");

  using atools::settings::Settings;
  port = Settings::instance().getAndStoreValue("Options/DefaultPort", 51968).toInt();
}

NavServer::~NavServer()
{
  stopServer();
}

void NavServer::stopServer()
{
  qDebug() << "Navserver stopping";

  close();

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

  QHostInfo hostInfo = QHostInfo::fromName(ipAddress);

  if(!retval)
    qCritical(gui).noquote().nospace() << "Unable to start the server: " << errorString() << ".";
  else
    qInfo(gui).noquote().nospace() << "Server is running on "
                                   << hostInfo.hostName() << " (" << ipAddress << ") "
                                   << "port " << serverPort() << ".";

  return retval;
}

void NavServer::incomingConnection(qintptr socketDescriptor)
{
  qDebug() << "Incoming connection";

  NavServerWorker *worker = new NavServerWorker(socketDescriptor, nullptr, verbose);
  worker->setObjectName("SocketWorker-" + QString::number(socketDescriptor));

  QThread *workerThread = new QThread(this);
  workerThread->setObjectName("SocketWorkerThread-" + QString::number(socketDescriptor));
  worker->moveToThread(workerThread);

  connect(workerThread, &QThread::started, worker, &NavServerWorker::threadStarted);
  connect(workerThread, &QThread::finished, [ = ]()->void {threadFinished(worker);
          });
  connect(dataReader, &DataReaderThread::postSimConnectData,
          worker, &NavServerWorker::postSimConnectData /*,
                                                        *  Qt::BlockingQueuedConnection*/);

  qDebug() << "Thread" << worker->objectName();
  workerThread->start();

  workers.insert(worker);
}

void NavServer::threadFinished(NavServerWorker *worker)
{
  QMutexLocker locker(&threadsMutex);

  qDebug() << "Thread" << worker->objectName() << "finished";

  disconnect(dataReader, &DataReaderThread::postSimConnectData,
             worker, &NavServerWorker::postSimConnectData);

  workers.remove(worker);

  worker->deleteLater();
  worker->thread()->deleteLater();
}

bool NavServer::hasConnections() const
{
  QMutexLocker locker(&threadsMutex);
  return workers.size() > 0;
}
