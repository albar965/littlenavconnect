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

#include "datareaderthread.h"
#include "simconnecthandler.h"
#include "logging/loggingdefs.h"
#include <net/navserver.h>
#include <fs/simconnectdata.h>
#include <QDateTime>

DataReaderThread::DataReaderThread(QObject *parent, NavServer *navServer)
  : QThread(parent), server(navServer)
{
  qDebug() << "Datareader started";
}

DataReaderThread::~DataReaderThread()
{
  qDebug() << "Datareader deleted";
}

void DataReaderThread::run()
{
  SimConnectHandler handler;

  handler.initialize();

  int i = 0;

  while(!terminate)
  {
    atools::fs::SimConnectData data;

    data.setPacketId(i);
    data.setPacketTs(QDateTime::currentDateTime().toTime_t());

    handler.fetchData(data);

    server->postMessage(data);
    i++;
    QThread::msleep(500);
  }
}

void DataReaderThread::setTerminate()
{
  terminate = true;
}
