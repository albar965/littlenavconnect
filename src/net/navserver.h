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

#ifndef NAVSERVER_H
#define NAVSERVER_H

#include <QMutex>
#include <QStringList>
#include <QTcpServer>

namespace atools {
namespace fs {
class SimConnectData;
}
}

class NavServerThread;

class NavServer :
  public QTcpServer
{
  Q_OBJECT

public:
  NavServer(QObject *parent = nullptr);
  virtual ~NavServer();

  bool startServer();
  void stopServer();

  void postMessage(const atools::fs::SimConnectData& dataPacket);

  bool hasConnections() const;

  void setPort(int value)
  {
    port = value;
  }

private:
  void incomingConnection(qintptr socketDescriptor) override;

  void threadFinished();

  QSet<NavServerThread *> threads;
  mutable QMutex threadsMutex;
  bool isTerminating = false;
  int port = 51968;
};

#endif // NAVSERVER_H
