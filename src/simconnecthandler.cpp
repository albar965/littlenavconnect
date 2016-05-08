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

enum EventIds
{
  EVENT_SIM_STATE,
  EVENT_SIM_PAUSE
};

enum DataDefinitionId
{
  DATA_DEFINITION,
};

enum DataRequestId
{
  DATA_REQUEST_ID,
  SYS_DATA_REQUEST_ID,
};

SimConnectHandler::SimConnectHandler(bool verboseLogging)
  : verbose(verboseLogging)
{

}

SimConnectHandler::~SimConnectHandler()
{
#if defined(Q_OS_WIN32)
  if(hSimConnect != NULL)
  {
    HRESULT hr = SimConnect_Close(hSimConnect);
    if(hr != S_OK)
      qWarning() << "Error closing SimConnect";
  }
#endif
}

bool SimConnectHandler::fetchData(atools::fs::SimConnectData& data)
{
#if defined(Q_OS_WIN32)
  HRESULT hr = SimConnect_RequestDataOnSimObjectType(hSimConnect, DATA_REQUEST_ID, DATA_DEFINITION, 0,
                                                     SIMCONNECT_SIMOBJECT_TYPE_USER);

  if(hr != S_OK)
  {
    qWarning() << "Error requesting data";
    return false;
  }

  SimConnect_CallDispatch(hSimConnect, DispatchCallback, this);

  if(!simRunning || simPaused)
  {
    if(verbose)
      qDebug() << "No data fetched. Running" << simRunning << "paused" << simPaused;
    return false;
  }

  data.setAirplaneName(simData.title);
  data.setAirplaneReg(simData.atcId);
  data.setAirplaneType(simData.atcModel);

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

  return true;
}

void SimConnectHandler::initialize()
{
#if defined(Q_OS_WIN32)
  HRESULT hr;

  if(verbose)
    qDebug() << "Before open";

  hr = SimConnect_Open(&hSimConnect, "Little Navconnect", NULL, 0, 0, 0);
  if(hr == S_OK)
  {
    if(verbose)
      qDebug() << "Connected to Flight Simulator";

    // Set up the data definition, but do not yet do anything with it
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Title", NULL,
                                        SIMCONNECT_DATATYPE_STRING256);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "ATC Type", NULL,
                                        SIMCONNECT_DATATYPE_STRING64);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "ATC Model", NULL,
                                        SIMCONNECT_DATATYPE_STRING64);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "ATC Id", NULL,
                                        SIMCONNECT_DATATYPE_STRING64);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "ATC Airline", NULL,
                                        SIMCONNECT_DATATYPE_STRING64);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "ATC Flight Number", NULL,
                                        SIMCONNECT_DATATYPE_STRING64);

    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Plane Altitude", "feet");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Plane Latitude", "degrees");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Plane Longitude", "degrees");

    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Ground Velocity", "knots");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Indicated Altitude", "feet");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Plane Alt Above Ground", "feet");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Plane Heading Degrees Magnetic",
                                        "degrees");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Plane Heading Degrees True", "degrees");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Ground Altitude", "feet");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Sim On Ground", "bool",
                                        SIMCONNECT_DATATYPE_INT64);

    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Airspeed True", "knots");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Airspeed Indicated", "knots");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Airspeed Mach", "mach");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Vertical Speed", "feet/second");

    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Ambient Temperature", "celsius");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Ambient Pressure", "inches of mercury");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Ambient Wind Velocity", "knots");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Ambient Wind Direction", "degrees");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Ambient Precip State", "bool",
                                        SIMCONNECT_DATATYPE_INT64);

    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Aircraft Wind X", "knots");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Aircraft Wind Y", "knots");
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Aircraft Wind Z", "knots");

    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Ambient In Cloud", "bool",
                                        SIMCONNECT_DATATYPE_INT64);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Sea Level Pressure", "millibars");

    // Request an event when the simulation starts
    hr = SimConnect_SubscribeToSystemEvent(hSimConnect, EVENT_SIM_STATE, "Sim");
    hr = SimConnect_SubscribeToSystemEvent(hSimConnect, EVENT_SIM_PAUSE, "Pause");
  }
#endif
}

#if defined(Q_OS_WIN32)
void SimConnectHandler::DispatchProcedure(SIMCONNECT_RECV *pData, DWORD cbData)
{
  Q_UNUSED(cbData);

  switch(pData->dwID)
  {
    case SIMCONNECT_RECV_ID_EVENT:
      {
        SIMCONNECT_RECV_EVENT *evt = (SIMCONNECT_RECV_EVENT *)pData;

        switch(evt->uEventID)
        {
          case EVENT_SIM_PAUSE:
            if(verbose)
              qDebug() << "EVENT_SIM_PAUSE" << evt->dwData;
            simPaused = evt->dwData == 1;
            break;

          case EVENT_SIM_STATE:
            if(verbose)
              qDebug() << "EVENT_SIM_STATE" << evt->dwData;
            simRunning = evt->dwData == 1;
            break;
        }
        break;
      }

    case SIMCONNECT_RECV_ID_SIMOBJECT_DATA_BYTYPE:
      {
        SIMCONNECT_RECV_SIMOBJECT_DATA_BYTYPE *pObjData = (SIMCONNECT_RECV_SIMOBJECT_DATA_BYTYPE *)pData;

        if(pObjData->dwRequestID == DATA_REQUEST_ID)
        {
          if(verbose)
            qDebug() << "DATA_REQUEST_ID";
          DWORD objectID = pObjData->dwObjectID;
          SimData *simDataPtr = reinterpret_cast<SimData *>(&pObjData->dwData);
          if(SUCCEEDED(StringCbLengthA(&simDataPtr->title[0], sizeof(simDataPtr->title), NULL))) // security check
          {
            if(verbose)
              qDebug() << "ObjectID" << objectID << "Title" << simDataPtr->title
                       << "atcType" << simDataPtr->atcType
                       << "atcModel" << simDataPtr->atcModel
                       << "atcId" << simDataPtr->atcId
                       << "atcAirline" << simDataPtr->atcAirline
                       << "atcFlightNumber" << simDataPtr->atcFlightNumber
                       << "Lat" << simDataPtr->latitude << "Lon" << simDataPtr->longitude
                       << "Alt" << simDataPtr->altitude
                       << "ias" << simDataPtr->airspeedIndicated << "gs" << simDataPtr->groundVelocity
                       << "vs" << simDataPtr->verticalSpeed
                       << "course " << simDataPtr->planeHeadingMagnetic
                       << "M" << simDataPtr->planeHeadingTrue << "T"
                       << "wind" << simDataPtr->ambientWindDirection
                       << "/" << simDataPtr->ambientWindVelocity;
            simData = *simDataPtr;
          }
        }
        break;
      }

    case SIMCONNECT_RECV_ID_QUIT:
      if(verbose)
        qDebug() << "SIMCONNECT_RECV_ID_QUIT";
      break;

    default:
      if(verbose)
        qDebug() << "Received" << pData->dwID;
      break;
  }
}

void CALLBACK SimConnectHandler::DispatchCallback(SIMCONNECT_RECV *pData, DWORD cbData, void *pContext)
{
  SimConnectHandler *handlerClass = static_cast<SimConnectHandler *>(pContext);
  handlerClass->DispatchProcedure(pData, cbData);
}

#endif
