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

#include "common.h"
#include "datareaderthread.h"
#include "simconnecthandler.h"
#include "logging/loggingdefs.h"
#include <net/navserver.h>
#include <fs/simconnectdata.h>
#include <QDateTime>
#include "settings/settings.h"

DataReaderThread::DataReaderThread(QObject *parent, NavServer *navServer)
  : QThread(parent), server(navServer)
{
  qDebug() << "Datareader started";
}

DataReaderThread::~DataReaderThread()
{
  qDebug() << "Datareader deleted";
}

void DataReaderThread::tryConnect(SimConnectHandler *handler)
{
  int counter = 0;
  while(!terminate)
  {
    if((counter % 20) == 0)
    {
      if(handler->connect())
        break;

      qWarning(gui) << "Error connecting to simulator. Will retry in 10 seconds.";
      counter = 0;
    }
    counter++;
    QThread::msleep(500);
  }
  qInfo(gui) << "Connected to simulator.";
}

void DataReaderThread::run()
{
  qDebug() << "Datareader run";
  SimConnectHandler handler(true);

  tryConnect(&handler);

  int i = 0;

  while(!terminate)
  {
    atools::fs::SimConnectData data;

    if(handler.fetchData(data))
    {
      if(handler.getState() != sc::OK)
      {
        qWarning(gui) << "Error fetching data from simulator.";

        if(!handler.isSimRunning())
          tryConnect(&handler);
      }
      if(terminate)
        break;

      data.setPacketId(i);
      data.setPacketTs(QDateTime::currentDateTime().toTime_t());
      server->postMessage(data);
      i++;
    }
    QThread::msleep(atools::settings::Settings::instance().getAndStoreValue("Options/UpdateRate", 500).toUInt());
  }
}

void DataReaderThread::setTerminate()
{
  terminate = true;
}
