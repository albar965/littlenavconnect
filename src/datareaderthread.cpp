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
  int i = 0;
  atools::fs::SimConnectData data;

  while(!terminate)
  {
    QString num = QString::number(i);

    data.setPacketId(i);
    data.setPacketTs(QDateTime::currentDateTime().toTime_t());

#if defined(Q_OS_WIN32)

#else
    data.setAirplaneName("Airplane " + num);
    data.setAirplaneReg("Airplane Registration " + num);
    data.setAirplaneType("Airplane Type " + num);

    // 200 kts: 0.0555 nm per second / 0.0277777 nm per cycle
    data.getPosition().setLonX(8.f + (i * 0.0277777f / 60.f));
    data.getPosition().setLatY(51.f + (i * 0.0277777f / 60.f));
    data.getPosition().setAltitude(qrand() * 100.f / RAND_MAX + 10000.f);
    data.setCourseMag(45.f);
    data.setCourseTrue(45.5f);
    data.setGroundSpeed(250.f);
    data.setIndicatedSpeed(200.f);
    data.setWindDirection(180.f);
    data.setWindSpeed(25.f);
#endif

    server->postMessage(data);
    i++;
    QThread::msleep(500);
  }
}

void DataReaderThread::setTerminate()
{
  terminate = true;
}
