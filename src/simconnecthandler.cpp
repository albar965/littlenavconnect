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
#include "fs/sc/simconnectdata.h"

#if !defined(Q_OS_WIN32)
#include "settings/settings.h"
#include "geo/calculations.h"
#endif

#include <QDate>
#include <QTime>
#include <QDateTime>

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

bool SimConnectHandler::fetchData(atools::fs::sc::SimConnectData& data)
{
#if defined(Q_OS_WIN32)
  dataFetched = false;

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

  if(!simRunning || simPaused || !dataFetched)
  {
    if(verbose)
      qDebug() << "No data fetched. Running" << simRunning << "paused" << simPaused
               << "dataFetched" << dataFetched;
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

  data.setGroundSpeed(simData.groundVelocity);
  data.setIndicatedAltitude(simData.indicatedAltitude);
  data.setAltitudeAboveGround(simData.planeAboveGround);
  data.setCourseMag(simData.planeHeadingMagnetic);
  data.setCourseTrue(simData.planeHeadingTrue);
  data.setTrackMag(simData.planeTrackMagnetic);
  data.setTrackTrue(simData.planeTrackTrue);
  data.setGroundAltitude(simData.groundAltitude);

  data.setFlags(atools::fs::sc::NONE);
  if(simData.simOnGround > 0)
    data.getFlags() |= atools::fs::sc::ON_GROUND;

  if(simData.ambientPrecipState & 4)
    data.getFlags() |= atools::fs::sc::IN_RAIN;
  if(simData.ambientPrecipState & 8)
    data.getFlags() |= atools::fs::sc::IN_SNOW;

  if(simData.ambientInCloud > 0)
    data.getFlags() |= atools::fs::sc::IN_CLOUD;

  data.setTrueSpeed(simData.airspeedTrue);
  data.setIndicatedSpeed(simData.airspeedIndicated);
  data.setMachSpeed(simData.airspeedMach);
  data.setVerticalSpeed(simData.verticalSpeed * 60.f);

  data.setAmbientTemperature(simData.ambientTemperature);
  data.setTotalAirTemperature(simData.totalAirTemperature);
  data.setAmbientVisibility(simData.ambientVisibility);

  data.setSeaLevelPressure(simData.seaLevelPressure);
  data.setPitotIce(simData.pitotIce);
  data.setStructuralIce(simData.structuralIce);
  data.setAirplaneTotalWeight(simData.airplaneTotalWeight);
  data.setAirplaneMaxGrossWeight(simData.airplaneMaxGrossWeight);
  data.setAirplaneEmptyWeight(simData.airplaneEmptyWeight);
  data.setFuelTotalQuantity(simData.fuelTotalQuantity);
  data.setFuelTotalWeight(simData.fuelTotalWeight);

  data.setFuelFlowPPH(
    simData.fuelFlowPph1 + simData.fuelFlowPph2 + simData.fuelFlowPph3 + simData.fuelFlowPph4);

  data.setFuelFlowGPH(
    simData.fuelFlowGph1 + simData.fuelFlowGph2 + simData.fuelFlowGph3 + simData.fuelFlowGph4);

  data.setWindDirection(simData.ambientWindDirection);
  data.setWindSpeed(simData.ambientWindVelocity);
  data.setMagVar(simData.magVar);

  QDate localDate(simData.localYear, simData.localMonth, simData.localDay);
  QTime localTime = QTime::fromMSecsSinceStartOfDay(simData.localTime * 1000);
  QDateTime localDateTime(localDate, localTime, Qt::OffsetFromUTC, simData.timeZoneOffset);
  data.setLocalTime(localDateTime);

  QDate zuluDate(simData.zuluYear, simData.zuluMonth, simData.zuluDay);
  QTime zuluTime = QTime::fromMSecsSinceStartOfDay(simData.zuluTime * 1000);
  QDateTime zuluDateTime(zuluDate, zuluTime, Qt::UTC);
  data.setZuluTime(zuluDateTime);

#else

  int updateRate =
    atools::settings::Settings::instance().getAndStoreValue("Options/UpdateRate", 500).toInt();
  static int dataId = 0;
  static int updatesMs = 0;
  static atools::geo::Pos curPos(8.34239197, 54.9116364);
  // 200 kts: 0.0555 nm per second / 0.0277777 nm per cycle - only for 500 ms updates
  float speed = 200.f;
  float nmPerSec = speed / 3600.f;
  static float course = 45.f;
  static float courseChange = 0.f;
  static float fuelFlow = 100.f;
  static float visibility = 0.1f;

  static float alt = 0.f, altChange = 0.f;

  updatesMs += updateRate;

  if((updatesMs % 40000) == 0)
    courseChange = 0.f;
  else if((updatesMs % 30000) == 0)
  {
    courseChange = updateRate / 1000.f * 2.f; // 2 deg per second
    if(course > 180.f)
      courseChange = -courseChange;
  }
  course += courseChange;
  course = atools::geo::normalizeCourse(course);

  // Simulate takeoff run
  if(updatesMs <= 10000)
  {
    data.setFlags(data.getFlags() | atools::fs::sc::ON_GROUND);
    fuelFlow = 200.f;
  }

  // Simulate takeoff
  if(updatesMs == 10000)
  {
    altChange = updateRate / 1000.f * 16.6f; // 1000 ft per min
    data.setFlags(data.getFlags() & ~atools::fs::sc::ON_GROUND);
    fuelFlow = 150.f;
  }

  if((updatesMs % 120000) == 0)
  {
    altChange = 0.f;
    fuelFlow = 100.f;
  }
  else if((updatesMs % 60000) == 0)
  {
    altChange = updateRate / 1000.f * 16.6f; // 1000 ft per min
    fuelFlow = 150.f;
    if(alt > 8000.f)
    {
      altChange = -altChange / 2.f;
      fuelFlow = 50.f;
    }
  }
  alt += altChange;

  if(updatesMs == 20000)
    data.setFlags(
      data.getFlags() | atools::fs::sc::IN_SNOW | atools::fs::sc::IN_CLOUD | atools::fs::sc::IN_RAIN);
  else if(updatesMs == 10000)
    data.setFlags(data.getFlags() &
                  ~(atools::fs::sc::IN_SNOW | atools::fs::sc::IN_CLOUD | atools::fs::sc::IN_RAIN));

  atools::geo::Pos next =
    curPos.endpoint(atools::geo::nmToMeter(updateRate / 1000.f * nmPerSec), course).normalize();

  QString dataIdStr = QString::number(dataId);
  data.setAirplaneTitle("Airplane Title " + dataIdStr);
  data.setAirplaneModel("Duke");
  data.setAirplaneReg("D-REGI");
  data.setAirplaneType("Beech");
  data.setAirplaneAirline("Airline");
  data.setAirplaneFlightnumber("965");
  data.setFuelFlowPPH(fuelFlow);
  data.setFuelFlowGPH(fuelFlow / 6.f);
  data.setAmbientVisibility(visibility);
  visibility += 1.f;

  data.setPosition(next);
  data.getPosition().setAltitude(alt);
  data.setVerticalSpeed(altChange * 60.f);

  data.setCourseMag(course);
  data.setCourseTrue(course + 1.f);

  data.setGroundSpeed(200.f);
  data.setIndicatedSpeed(150.f);
  data.setTrueSpeed(170.f);
  data.setWindDirection(180.f);
  data.setWindSpeed(25.f);
  data.setSeaLevelPressure(1013.f);

  data.setAmbientTemperature(10.f);
  data.setTotalAirTemperature(20.f);
  data.setFuelTotalQuantity(1000.f / 6.f);
  data.setFuelTotalWeight(1000.f);

  data.setLocalTime(QDateTime::currentDateTime());

  QDate zuluDate(QDate::currentDate().year(), QDate::currentDate().month(), QDate::currentDate().day());
  QTime zuluTime = QTime::fromMSecsSinceStartOfDay(QTime::currentTime().msecsSinceStartOfDay());
  QDateTime zuluDateTime(zuluDate, zuluTime, Qt::UTC);
  data.setZuluTime(zuluDateTime);

  dataId++;

  curPos = next;
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
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "GPS Ground Magnetic Track",
                                        "degrees", SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "GPS Ground True Track", "degrees",
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
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Vertical Speed", "feet per second",
                                        SIMCONNECT_DATATYPE_FLOAT32);

    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Ambient Temperature", "celsius",
                                        SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Total Air Temperature", "celsius",
                                        SIMCONNECT_DATATYPE_FLOAT32);

    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Ambient Wind Velocity", "knots",
                                        SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Ambient Wind Direction", "degrees",
                                        SIMCONNECT_DATATYPE_FLOAT32);

    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Ambient Precip State", "mask",
                                        SIMCONNECT_DATATYPE_INT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Ambient In Cloud", "bool",
                                        SIMCONNECT_DATATYPE_INT32);

    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Ambient Visibility", "meters",
                                        SIMCONNECT_DATATYPE_FLOAT32);

    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Sea Level Pressure", "millibars",
                                        SIMCONNECT_DATATYPE_FLOAT32);

    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Pitot Ice Pct", "percent",
                                        SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Structural Ice Pct", "percent",
                                        SIMCONNECT_DATATYPE_FLOAT32);

    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Total Weight", "pounds",
                                        SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Max Gross Weight", "pounds",
                                        SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Empty Weight", "pounds",
                                        SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Fuel Total Quantity", "gallons",
                                        SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Fuel Total Quantity Weight", "pounds",
                                        SIMCONNECT_DATATYPE_FLOAT32);

    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Eng Fuel Flow PPH:1",
                                        "Pounds per hour", SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Eng Fuel Flow PPH:2",
                                        "Pounds per hour", SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Eng Fuel Flow PPH:3",
                                        "Pounds per hour", SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Eng Fuel Flow PPH:4",
                                        "Pounds per hour", SIMCONNECT_DATATYPE_FLOAT32);

    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Eng Fuel Flow GPH:1",
                                        "Gallons per hour", SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Eng Fuel Flow GPH:2",
                                        "Gallons per hour", SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Eng Fuel Flow GPH:3",
                                        "Gallons per hour", SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Eng Fuel Flow GPH:4",
                                        "Gallons per hour", SIMCONNECT_DATATYPE_FLOAT32);

    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Magvar",
                                        "degrees", SIMCONNECT_DATATYPE_FLOAT32);

    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Local Time",
                                        "seconds", SIMCONNECT_DATATYPE_INT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Local Year",
                                        "number", SIMCONNECT_DATATYPE_INT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Local Month of Year",
                                        "number", SIMCONNECT_DATATYPE_INT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Local Day of Month",
                                        "number", SIMCONNECT_DATATYPE_INT32);

    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Zulu Time",
                                        "seconds", SIMCONNECT_DATATYPE_INT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Zulu Year",
                                        "number", SIMCONNECT_DATATYPE_INT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Zulu Month of Year",
                                        "number", SIMCONNECT_DATATYPE_INT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Zulu Day of Month",
                                        "number", SIMCONNECT_DATATYPE_INT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION, "Time Zone Offset",
                                        "seconds", SIMCONNECT_DATATYPE_INT32);

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
      {
        // enter code to handle SimConnect version information received in a SIMCONNECT_RECV_OPEN structure.
        SIMCONNECT_RECV_OPEN *openData = (SIMCONNECT_RECV_OPEN *)pData;

        qInfo() << "ApplicationName" << openData->szApplicationName;
        qInfo().nospace() << "ApplicationVersion " << openData->dwApplicationVersionMajor
                          << "." << openData->dwApplicationVersionMinor;
        qInfo().nospace() << "ApplicationBuild " << openData->dwApplicationBuildMajor
                          << "." << openData->dwApplicationBuildMinor;
        qInfo().nospace() << "SimConnectVersion " << openData->dwSimConnectVersionMajor
                          << "." << openData->dwSimConnectVersionMinor;
        qInfo().nospace() << "SimConnectBuild " << openData->dwSimConnectBuildMajor
                          << "." << openData->dwSimConnectBuildMinor;
        break;
      }

    case SIMCONNECT_RECV_ID_EXCEPTION:
      {
        // enter code to handle errors received in a SIMCONNECT_RECV_EXCEPTION structure.
        SIMCONNECT_RECV_EXCEPTION *except = (SIMCONNECT_RECV_EXCEPTION *)pData;
        qWarning() << "SimConnect exception" << except->dwException
                   << "send ID" << except->dwSendID << "index" << except->dwIndex;
        break;
      }

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
                       << "/" << simDataPtr->ambientWindVelocity
                       << "/" << simDataPtr->ambientWindVelocity
                       << "magvar" << simDataPtr->magVar
                       << "local time" << simDataPtr->localTime
                       << "local year" << simDataPtr->localYear
                       << "local month" << simDataPtr->localMonth
                       << "local day" << simDataPtr->localDay
                       << "zulu time" << simDataPtr->zuluTime
                       << "zulu year" << simDataPtr->zuluYear
                       << "zulu month" << simDataPtr->zuluMonth
                       << "zulu day" << simDataPtr->zuluDay
              ;
            simData = *simDataPtr;
            dataFetched = true;
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
