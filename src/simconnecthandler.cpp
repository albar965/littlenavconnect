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
    qWarning() << "SimConnect_RequestDataOnSimObjectType: Error";
    state = sc::FETCH_ERROR;
    return false;
  }

  hr = SimConnect_CallDispatch(hSimConnect, DispatchCallback, this);
  if(hr != S_OK)
  {
    qWarning() << "SimConnect_CallDispatch: Error";
    state = sc::FETCH_ERROR;
    return false;
  }

  state = sc::OK;

  if(!simRunning || simPaused)
  {
    if(verbose)
      qDebug() << "No data fetched. Running" << simRunning << "paused" << simPaused;
    return false;
  }

  data.setAirplaneTitle(simData.title);
  data.setAirplaneModel(simData.atcModel);
  data.setAirplaneReg(simData.atcId);
  data.setAirplaneType(simData.atcType);
  data.setAirplaneAirline(simData.atcAirline);
  data.setAirplaneFlightnumber(simData.atcFlightNumber);

  data.getPosition().setLonX(simData.longitude);
  data.getPosition().setLatY(simData.latitude);
  data.getPosition().setAltitude(simData.altitude);
  data.setCourseMag(simData.planeHeadingMagnetic);
  data.setCourseTrue(simData.planeHeadingTrue);
  data.setGroundSpeed(simData.groundVelocity);
  data.setIndicatedSpeed(simData.airspeedIndicated);
  data.setWindDirection(simData.ambientWindDirection);
  data.setWindSpeed(simData.ambientWindVelocity);
  data.setVerticalSpeed(simData.verticalSpeed);
#else
  static int dataId = 0;
  QString dataIdStr = QString::number(dataId);
  data.setAirplaneTitle("Airplane Title " + dataIdStr);
  data.setAirplaneModel("Airplane Model " + dataIdStr);
  data.setAirplaneReg("Airplane Registration " + dataIdStr);
  data.setAirplaneType("Airplane Type " + dataIdStr);
  data.setAirplaneAirline("Airplane Airline " + dataIdStr);
  data.setAirplaneFlightnumber("Airplane Flight Number " + dataIdStr);

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
  dataId++;
#endif

  return true;
}

bool SimConnectHandler::connect()
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
                                        SIMCONNECT_DATATYPE_STRING32);

    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "ATC Model", NULL,
                                        SIMCONNECT_DATATYPE_STRING32);

    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "ATC Id", NULL,
                                        SIMCONNECT_DATATYPE_STRING32);

    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "ATC Airline", NULL,
                                        SIMCONNECT_DATATYPE_STRING64);

    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "ATC Flight Number", NULL,
                                        SIMCONNECT_DATATYPE_STRING32);

    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Plane Altitude", "feet",
                                        SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Plane Latitude", "degrees",
                                        SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Plane Longitude", "degrees",
                                        SIMCONNECT_DATATYPE_FLOAT32);

    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Ground Velocity", "knots",
                                        SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Indicated Altitude", "feet",
                                        SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Plane Alt Above Ground", "feet",
                                        SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Plane Heading Degrees Magnetic",
                                        "degrees", SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Plane Heading Degrees True", "degrees",
                                        SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Ground Altitude", "feet",
                                        SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Sim On Ground", "bool",
                                        SIMCONNECT_DATATYPE_INT32);

    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Airspeed True", "knots",
                                        SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Airspeed Indicated", "knots",
                                        SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Airspeed Mach", "mach",
                                        SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Vertical Speed", "feet/second",
                                        SIMCONNECT_DATATYPE_FLOAT32);

    // hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Ambient Temperature", "celsius",
    // SIMCONNECT_DATATYPE_FLOAT32);
    // hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Ambient Pressure", "inches of mercury",
    // SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Ambient Wind Velocity", "knots",
                                        SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Ambient Wind Direction", "degrees",
                                        SIMCONNECT_DATATYPE_FLOAT32);

    // hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Ambient Precip State", "bool",
    // SIMCONNECT_DATATYPE_INT32);
    // hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Aircraft Wind X", "knots",
    // SIMCONNECT_DATATYPE_FLOAT32);
    // hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Aircraft Wind Y", "knots",
    // SIMCONNECT_DATATYPE_FLOAT32);
    // hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Aircraft Wind Z", "knots",
    // SIMCONNECT_DATATYPE_FLOAT32);
    // hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Ambient In Cloud", "bool",
    // SIMCONNECT_DATATYPE_INT32);
    // hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Sea Level Pressure", "millibars",
    // SIMCONNECT_DATATYPE_FLOAT32);

    // Request an event when the simulation starts
    hr = SimConnect_SubscribeToSystemEvent(hSimConnect, EVENT_SIM_STATE, "Sim");
    hr = SimConnect_SubscribeToSystemEvent(hSimConnect, EVENT_SIM_PAUSE, "Pause");

    state = sc::OK;

    return true;
  }
  else
  {
    qWarning() << "SimConnect_Open: Error";
    state = sc::OPEN_ERROR;
    return false;
  }
#endif
  return true;
}

#if defined(Q_OS_WIN32)
void SimConnectHandler::DispatchProcedure(SIMCONNECT_RECV *pData, DWORD cbData)
{
  Q_UNUSED(cbData);

  switch(pData->dwID)
  {
    case SIMCONNECT_RECV_ID_OPEN:
      // enter code to handle SimConnect version information received in a SIMCONNECT_RECV_OPEN structure.
      SIMCONNECT_RECV_OPEN * openData = (SIMCONNECT_RECV_OPEN *)pData;

      qInfo() << "ApplicationName" << szApplicationName;
      qInfo() << "ApplicationVersionMajor" << dwApplicationVersionMajor
              << "ApplicationVersionMinor" << dwApplicationVersionMinor;
      qInfo() << "ApplicationBuildMajor" << dwApplicationBuildMajor
              << "ApplicationBuildMinor" << dwApplicationBuildMinor;
      qInfo() << "SimConnectVersionMajor" << dwSimConnectVersionMajor
              << "SimConnectVersionMinor" << dwSimConnectVersionMinor;
      qInfo() << "SimConnectBuildMajor" << dwSimConnectBuildMajor
              << "SimConnectBuildMinor" << dwSimConnectBuildMinor;
      break;

    case SIMCONNECT_RECV_ID_EXCEPTION:
      // enter code to handle errors received in a SIMCONNECT_RECV_EXCEPTION structure.
      SIMCONNECT_RECV_EXCEPTION * except = (SIMCONNECT_RECV_EXCEPTION *)pData;
      qWarning() << "SimConnect exception" << except->dwException
                 << "send ID" << except->dwSendId << "index" << except->dwIndex;
      break;

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
      simRunning = false;
      state = sc::DISCONNECTED;
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
