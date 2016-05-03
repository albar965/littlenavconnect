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

DataReaderThread::DataReaderThread(QObject *parent, NavServer *navServer)
  : QThread(parent), server(navServer)
{
  server->log("Datareader started", QtInfoMsg);
}

DataReaderThread::~DataReaderThread()
{
  server->log("Datareader deleted", QtInfoMsg);
}

void DataReaderThread::run()
{
  int i = 0;
  while(!terminate)
  {
    server->log("Message " + QString::number(i), QtInfoMsg);
    server->postMessage(QString::number(i));
    i++;
    QThread::sleep(1);
  }
}

void DataReaderThread::setTerminate()
{
  terminate = true;
}
