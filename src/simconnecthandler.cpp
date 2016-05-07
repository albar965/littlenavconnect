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

#include "simconnecthandler.h"

#include <QDebug>

#include <fs/simconnectdata.h>

#if defined(Q_OS_WIN32)
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>
#include "SimConnect.h"
#endif

enum EventIds
{
  EVENT_SIM_START,
  EVENT_SIM_STOP,
  EVENT_SIM_PAUSE,
  EVENT_SIM_ONE_SEC,
};

enum DataDefinitionId
{
  DEFINITION_1,
};

enum DataRequestId
{
  REQUEST_1,
};

SimConnectHandler::SimConnectHandler()
{

}

SimConnectHandler::~SimConnectHandler()
{
#if defined(Q_OS_WIN32)

  if(hSimConnect != NULL)
  {
    HRESULT hr = SimConnect_Close(hSimConnect);
  }
#endif
}

void SimConnectHandler::fetchData(atools::fs::SimConnectData& data)
{
#if defined(Q_OS_WIN32)
  SimConnect_CallDispatch(hSimConnect, MyDispatchProcRD, this);
  data.setAirplaneName(simData.title);
  data.setAirplaneReg("Airplane Registration " + dataId);
  data.setAirplaneType("Airplane Type " + dataId);

  // 200 kts: 0.0555 nm per second / 0.0277777 nm per cycle
  data.getPosition().setLonX(static_cast<float>(simData.longitude));
  data.getPosition().setLatY(static_cast<float>(simData.latitude));
  data.getPosition().setAltitude(static_cast<float>(simData.altitude));
  data.setCourseMag(static_cast<float>(simData.planeHeadingMagnetic));
  data.setCourseTrue(static_cast<float>(simData.planeHeadingTrue));
  data.setGroundSpeed(static_cast<float>(simData.groundVelocity));
  data.setIndicatedSpeed(static_cast<float>(simData.airspeedIndicated));
  data.setWindDirection(static_cast<float>(simData.ambientWindDirection));
  data.setWindSpeed(static_cast<float>(simData.ambientWindVelocity));
  data.setVerticalSpeed(static_cast<float>(simData.verticalSpeed));
#else

  QString dataIdStr = QString::number(dataId);
  data.setAirplaneName("Airplane " + dataIdStr);
  data.setAirplaneReg("Airplane Registration " + dataIdStr);
  data.setAirplaneType("Airplane Type " + dataIdStr);

  // 200 kts: 0.0555 nm per second / 0.0277777 nm per cycle
  data.getPosition().setLonX(8.f + (dataId * 0.0277777f / 60.f));
  data.getPosition().setLatY(51.f + (dataId * 0.0277777f / 60.f));
  data.getPosition().setAltitude(qrand() * 100.f / RAND_MAX + 10000.f);
  data.setCourseMag(45.f);
  data.setCourseTrue(45.5f);
  data.setGroundSpeed(250.f);
  data.setIndicatedSpeed(200.f);
  data.setWindDirection(180.f);
  data.setWindSpeed(25.f);
  data.setVerticalSpeed(0.1f);

#endif
  dataId++;
}

void SimConnectHandler::initialize()
{
#if defined(Q_OS_WIN32)
  HRESULT hr;

  qDebug() << "Before open";

  hr = SimConnect_Open(&hSimConnect, "Little Navconnect", NULL, 0, 0, 0);
  if(hr == S_OK)
  {
    qDebug() << "Connected to Flight Simulator";

    // Set up the data definition, but do not yet do anything with it
    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_1, "Title", NULL,
                                        SIMCONNECT_DATATYPE_STRING256);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_1, "Plane Altitude", "feet");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_1, "Plane Latitude", "degrees");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_1, "Plane Longitude", "degrees");

    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_1, "Ground Velocity", "knots");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_1, "Indicated Altitude", "feet");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_1, "Plane Alt Above Ground", "feet");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_1, "Plane Heading Degrees Magnetic",
                                        "degrees");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_1, "Plane Heading Degrees True", "degrees");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_1, "Ground Altitude", "feet");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_1, "Sim On Ground", "bool",
                                        SIMCONNECT_DATATYPE_INT64);

    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_1, "Airspeed True", "knots");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_1, "Airspeed Indicated", "knots");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_1, "Airspeed Mach", "mach");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_1, "Vertical Speed", "feet/second");

    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_1, "Ambient Temperature", "celsius");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_1, "Ambient Pressure", "inches of mercury");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_1, "Ambient Wind Velocity", "knots");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_1, "Ambient Wind Direction", "degrees");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_1, "Ambient Precip State", "bool",
                                        SIMCONNECT_DATATYPE_INT64);

    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_1, "Aircraft Wind X", "knots");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_1, "Aircraft Wind Y", "knots");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_1, "Aircraft Wind Z", "knots");

    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_1, "Ambient In Cloud", "bool",
                                        SIMCONNECT_DATATYPE_INT64);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DEFINITION_1, "Sea Level Pressure", "millibars");

    // Struct latlonalt (SIMCONNECT_DATATYPE_LATLONALT)

    // Request an event when the simulation starts
    hr = SimConnect_SubscribeToSystemEvent(hSimConnect, EVENT_SIM_START, "SimStart");

    hr = SimConnect_SubscribeToSystemEvent(hSimConnect, EVENT_SIM_STOP, "SimStop");

    hr = SimConnect_SubscribeToSystemEvent(hSimConnect, EVENT_SIM_PAUSE, "Pause");

    hr = SimConnect_SubscribeToSystemEvent(hSimConnect, EVENT_SIM_ONE_SEC, "1sec");

    hr = SimConnect_SetSystemEventState(hSimConnect, EVENT_SIM_ONE_SEC, SIMCONNECT_STATE_OFF);
  }
#endif
}

#if defined(Q_OS_WIN32)
void SimConnectHandler::DispatchProcedure(SIMCONNECT_RECV *pData, DWORD cbData)
{
  HRESULT hr;

  switch(pData->dwID)
  {
    case SIMCONNECT_RECV_ID_EVENT:
      {
        SIMCONNECT_RECV_EVENT *evt = (SIMCONNECT_RECV_EVENT *)pData;

        switch(evt->uEventID)
        {
          case EVENT_SIM_ONE_SEC:
            qDebug() << "EVENT_SIM_ONE_SEC";
            // Now the sim is running, request information on the user aircraft
            hr = SimConnect_RequestDataOnSimObjectType(hSimConnect, REQUEST_1, DEFINITION_1, 0,
                                                       SIMCONNECT_SIMOBJECT_TYPE_USER);

            break;
          case EVENT_SIM_PAUSE:
            qDebug() << "EVENT_SIM_PAUSE" << evt->dwData;
            if(evt->dwData == 1)
              hr = SimConnect_SetSystemEventState(hSimConnect, EVENT_SIM_ONE_SEC, SIMCONNECT_STATE_OFF);
            else if(evt->dwData == 0)
              hr = SimConnect_SetSystemEventState(hSimConnect, EVENT_SIM_ONE_SEC, SIMCONNECT_STATE_ON);
            break;

          case EVENT_SIM_START:
            qDebug() << "EVENT_SIM_START";
            hr = SimConnect_SetSystemEventState(hSimConnect, EVENT_SIM_ONE_SEC, SIMCONNECT_STATE_ON);

            break;

          case EVENT_SIM_STOP:
            qDebug() << "EVENT_SIM_STOP";
            hr = SimConnect_SetSystemEventState(hSimConnect, EVENT_SIM_ONE_SEC, SIMCONNECT_STATE_OFF);

            break;

          default:
            break;
        }
        break;
      }

    case SIMCONNECT_RECV_ID_SIMOBJECT_DATA_BYTYPE:
      {
        SIMCONNECT_RECV_SIMOBJECT_DATA_BYTYPE *pObjData = (SIMCONNECT_RECV_SIMOBJECT_DATA_BYTYPE *)pData;

        switch(pObjData->dwRequestID)
        {
          case REQUEST_1:
            {
              qDebug() << "REQUEST_1";
              DWORD ObjectID = pObjData->dwObjectID;
              SimData *pS = (SimData *)&pObjData->dwData;
              if(SUCCEEDED(StringCbLengthA(&pS->title[0], sizeof(pS->title), NULL))) // security check
              {
                qDebug() << "ObjectID" << ObjectID << "Title" << pS->title
                         << "Lat" << pS->latitude << "Lon" << pS->longitude
                         << "Alt" << pS->altitude << "gs" << pS->groundVelocity;
                qDebug() << "wind xyz" << pS->aircraftWindX << "," << pS->aircraftWindY << ","
                         << pS->aircraftWindZ;
              }
              break;
            }

          default:
            break;
        }
        break;
      }

    case SIMCONNECT_RECV_ID_QUIT:
      qDebug() << "SIMCONNECT_RECV_ID_QUIT";
      quit = 1;
      break;

    default:
      qDebug() << "Received" << pData->dwID;
      break;
  }
}

void CALLBACK SimConnectHandler::MyDispatchProcRD(SIMCONNECT_RECV *pData, DWORD cbData, void *pContext)
{
  SimConnectHandler *handlerClass = static_cast<SimConnectHandler *>(pContext);
  handlerClass->DispatchProcedure(pData, cbData);
}

#endif
