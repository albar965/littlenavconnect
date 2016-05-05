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

#ifndef NAVSERVERTHREAD_H
#define NAVSERVERTHREAD_H

#include <QThread>
#include <QWaitCondition>
#include <QMutex>

#include "fs/simconnectdata.h"

class NavServer;

class NavServerThread :
  public QThread
{
  Q_OBJECT

public:
  NavServerThread(qintptr socketDescriptor, NavServer *parent);
  virtual ~NavServerThread();

  void postMessage(const atools::fs::SimConnectData& dataPacket);

  void setTerminate();

private:
  virtual void run() override;

  bool terminate = false;
  qintptr socket;
  NavServer *server;
  atools::fs::SimConnectData data;
  QWaitCondition waitCondition;
  mutable QMutex mutex;

};

#endif // NAVSERVERTHREAD_H
