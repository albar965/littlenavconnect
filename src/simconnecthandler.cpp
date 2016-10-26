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
#include "geo/calculations.h"
#endif

#include <QDate>
#include <QTime>
#include <QDateTime>
#include <QThread>

enum EventIds
{
  EVENT_SIM_STATE,
  EVENT_SIM_PAUSE
};

enum DataRequestId
{
  DATA_REQUEST_ID_USER,
  DATA_REQUEST_ID_AI
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

    fillDataDefinitionAicraft(DATA_DEFINITION_AI);

    fillDataDefinitionAicraft(DATA_DEFINITION_USER);

    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION_USER, "Plane Alt Above Ground", "feet",
                                        SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION_USER, "Ground Altitude", "feet",
                                        SIMCONNECT_DATATYPE_FLOAT32);

    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION_USER, "Ambient Temperature", "celsius",
                                        SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION_USER, "Total Air Temperature", "celsius",
                                        SIMCONNECT_DATATYPE_FLOAT32);

    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION_USER, "Ambient Wind Velocity", "knots",
                                        SIMCONNECT_DATATYPE_FLOAT32);
    hr =
      SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION_USER, "Ambient Wind Direction", "degrees",
                                     SIMCONNECT_DATATYPE_FLOAT32);

    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION_USER, "Ambient Precip State", "mask",
                                        SIMCONNECT_DATATYPE_INT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION_USER, "Ambient In Cloud", "bool",
                                        SIMCONNECT_DATATYPE_INT32);

    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION_USER, "Ambient Visibility", "meters",
                                        SIMCONNECT_DATATYPE_FLOAT32);

    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION_USER, "Sea Level Pressure", "millibars",
                                        SIMCONNECT_DATATYPE_FLOAT32);

    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION_USER, "Pitot Ice Pct", "percent",
                                        SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION_USER, "Structural Ice Pct", "percent",
                                        SIMCONNECT_DATATYPE_FLOAT32);

    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION_USER, "Total Weight", "pounds",
                                        SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION_USER, "Max Gross Weight", "pounds",
                                        SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION_USER, "Empty Weight", "pounds",
                                        SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION_USER, "Fuel Total Quantity", "gallons",
                                        SIMCONNECT_DATATYPE_FLOAT32);
    hr =
      SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION_USER, "Fuel Total Quantity Weight",
                                     "pounds", SIMCONNECT_DATATYPE_FLOAT32);

    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION_USER, "Eng Fuel Flow PPH:1",
                                        "Pounds per hour", SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION_USER, "Eng Fuel Flow PPH:2",
                                        "Pounds per hour", SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION_USER, "Eng Fuel Flow PPH:3",
                                        "Pounds per hour", SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION_USER, "Eng Fuel Flow PPH:4",
                                        "Pounds per hour", SIMCONNECT_DATATYPE_FLOAT32);

    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION_USER, "Eng Fuel Flow GPH:1",
                                        "Gallons per hour", SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION_USER, "Eng Fuel Flow GPH:2",
                                        "Gallons per hour", SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION_USER, "Eng Fuel Flow GPH:3",
                                        "Gallons per hour", SIMCONNECT_DATATYPE_FLOAT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION_USER, "Eng Fuel Flow GPH:4",
                                        "Gallons per hour", SIMCONNECT_DATATYPE_FLOAT32);

    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION_USER, "Local Time",
                                        "seconds", SIMCONNECT_DATATYPE_INT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION_USER, "Local Year",
                                        "number", SIMCONNECT_DATATYPE_INT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION_USER, "Local Month of Year",
                                        "number", SIMCONNECT_DATATYPE_INT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION_USER, "Local Day of Month",
                                        "number", SIMCONNECT_DATATYPE_INT32);

    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION_USER, "Zulu Time",
                                        "seconds", SIMCONNECT_DATATYPE_INT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION_USER, "Zulu Year",
                                        "number", SIMCONNECT_DATATYPE_INT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION_USER, "Zulu Month of Year",
                                        "number", SIMCONNECT_DATATYPE_INT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION_USER, "Zulu Day of Month",
                                        "number", SIMCONNECT_DATATYPE_INT32);
    hr = SimConnect_AddToDataDefinition(hSimConnect, DATA_DEFINITION_USER, "Time Zone Offset",
                                        "seconds", SIMCONNECT_DATATYPE_INT32);

    // Request an event when the simulation starts or pauses
    hr = SimConnect_SubscribeToSystemEvent(hSimConnect, EVENT_SIM_STATE, "Sim");
    hr = SimConnect_SubscribeToSystemEvent(hSimConnect, EVENT_SIM_PAUSE, "Pause");

    state = sc::OK;

    return true;
  }
  else
  {
    qWarning() << "SimConnect_Open: Error";
    state = sc::OPEN_ERROR;
    hSimConnect = NULL;
    return false;
  }
#endif
  return true;
}

bool SimConnectHandler::fetchData(atools::fs::sc::SimConnectData& data, int radiusKm)
{
#if defined(Q_OS_WIN32)
  if(verbose)
    qDebug() << "fetchData entered ================================================================";

  // ==========================================================
  if(verbose)
    qDebug() << "fetchData AI aircraft details";

  HRESULT hr = SimConnect_RequestDataOnSimObjectType(hSimConnect, DATA_REQUEST_ID_AI, DATA_DEFINITION_AI,
                                                     static_cast<DWORD>(radiusKm) * 1000,
                                                     SIMCONNECT_SIMOBJECT_TYPE_AIRCRAFT);

  if(hr != S_OK)
  {
    qWarning() << "SimConnect_RequestDataOnSimObjectType SIMCONNECT_SIMOBJECT_TYPE_AIRCRAFT: Error";
    state = sc::FETCH_ERROR;
    return false;
  }

  hr = SimConnect_RequestDataOnSimObjectType(hSimConnect, DATA_REQUEST_ID_USER, DATA_DEFINITION_USER, 0,
                                             SIMCONNECT_SIMOBJECT_TYPE_USER);

  if(hr != S_OK)
  {
    qWarning() << "SimConnect_RequestDataOnSimObjectType SIMCONNECT_SIMOBJECT_TYPE_USER: Error";
    state = sc::FETCH_ERROR;
    return false;
  }

  userDataFetched = false;
  dataFetched = true;
  simDataAircraft.clear();
  int dispatchCycles = 0;
  while(dataFetched)
  {
    dataFetched = false;
    hr = SimConnect_CallDispatch(hSimConnect, dispatchCallback, this);
    if(hr != S_OK)
    {
      qWarning() << "SimConnect_CallDispatch: Error";
      state = sc::FETCH_ERROR;
      return false;
    }

    QThread::msleep(10);
    dispatchCycles++;
  }

  state = sc::OK;

  if(verbose)
  {
    if(dispatchCycles > 1)
      qDebug() << "dispatchCycles > 1" << dispatchCycles;
    qDebug() << "numDataFetchedAi" << simDataAircraft.size();
  }

  for(const SimDataAircraft& simDataAi : simDataAircraft)
  {
    atools::fs::sc::SimConnectData data;
    copyToSimData(simDataAi, data);
  }

  if(userDataFetched)
  {
    copyToSimData(simData.aircraft, data);
    data.setGroundAltitudeFt(simData.groundAltitudeFt);

    if(simData.ambientPrecipStateFlags & 4)
      data.getFlags() |= atools::fs::sc::IN_RAIN;
    if(simData.ambientPrecipStateFlags & 8)
      data.getFlags() |= atools::fs::sc::IN_SNOW;

    if(simData.ambientIsInCloud > 0)
      data.getFlags() |= atools::fs::sc::IN_CLOUD;

    data.setAmbientTemperatureCelsius(simData.ambientTemperatureC);
    data.setTotalAirTemperatureCelsius(simData.totalAirTemperatureC);
    data.setAmbientVisibilityMeter(simData.ambientVisibilityMeter);

    data.setSeaLevelPressureMbar(simData.seaLevelPressureMbar);
    data.setPitotIcePercent(simData.pitotIcePercent);
    data.setStructuralIcePercent(simData.structuralIcePercent);
    data.setAirplaneTotalWeightLbs(simData.airplaneTotalWeightLbs);
    data.setAirplaneMaxGrossWeightLbs(simData.airplaneMaxGrossWeightLbs);
    data.setAirplaneEmptyWeightLbs(simData.airplaneEmptyWeightLbs);
    data.setFuelTotalQuantityGallons(simData.fuelTotalQuantityGallons);
    data.setFuelTotalWeightLbs(simData.fuelTotalWeightLbs);

    // Summarize fuel flow for all engines
    data.setFuelFlowPPH(
      simData.fuelFlowPph1 + simData.fuelFlowPph2 + simData.fuelFlowPph3 + simData.fuelFlowPph4);

    data.setFuelFlowGPH(
      simData.fuelFlowGph1 + simData.fuelFlowGph2 + simData.fuelFlowGph3 + simData.fuelFlowGph4);

    data.setWindDirectionDegT(simData.ambientWindDirectionDegT);
    data.setWindSpeedKts(simData.ambientWindVelocityKts);

    // Build local time and use timezone offset from simulator
    QDate localDate(simData.localYear, simData.localMonth, simData.localDay);
    QTime localTime = QTime::fromMSecsSinceStartOfDay(simData.localTime * 1000);
    QDateTime localDateTime(localDate, localTime, Qt::OffsetFromUTC, simData.timeZoneOffsetSeconds);
    data.setLocalTime(localDateTime);

    QDate zuluDate(simData.zuluYear, simData.zuluMonth, simData.zuluDay);
    QTime zuluTime = QTime::fromMSecsSinceStartOfDay(simData.zuluTimeSeconds * 1000);
    QDateTime zuluDateTime(zuluDate, zuluTime, Qt::UTC);
    data.setZuluTime(zuluDateTime);
  }

  if(!simRunning || simPaused || !userDataFetched)
  {
    if(verbose)
      qDebug() << "Running" << simRunning << "paused" << simPaused
               << "dataFetched" << dataFetched;
    return false;
  }
  return true;

#else

  // Simple aircraft simulation ------------------------------------------------
  static qint64 lastUpdate = QDateTime::currentMSecsSinceEpoch();
  int updateRate = static_cast<int>(QDateTime::currentMSecsSinceEpoch() - lastUpdate);
  lastUpdate = QDateTime::currentMSecsSinceEpoch();
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
  data.setAirplaneRegistration("D-REGI");
  data.setAirplaneType("Beech");
  data.setAirplaneAirline("Airline");
  data.setAirplaneFlightnumber("965");
  data.setFuelFlowPPH(fuelFlow);
  data.setFuelFlowGPH(fuelFlow / 6.f);
  data.setAmbientVisibilityMeter(visibility);
  visibility += 1.f;

  data.setPosition(next);
  data.getPosition().setAltitude(alt);
  data.setVerticalSpeedFeetPerMin(altChange * 60.f);

  data.setHeadingDegMag(course);
  data.setHeadingDegTrue(course + 1.f);

  data.setGroundSpeedKts(200.f);
  data.setIndicatedSpeedKts(150.f);
  data.setTrueSpeedKts(170.f);
  data.setWindDirectionDegT(180.f);
  data.setWindSpeedKts(25.f);
  data.setSeaLevelPressureMbar(1013.f);

  data.setAmbientTemperatureCelsius(10.f);
  data.setTotalAirTemperatureCelsius(20.f);
  data.setFuelTotalQuantityGallons(1000.f / 6.f);
  data.setFuelTotalWeightLbs(1000.f);

  data.setLocalTime(QDateTime::currentDateTime());

  QDate zuluDate(QDate::currentDate().year(), QDate::currentDate().month(), QDate::currentDate().day());
  QTime zuluTime = QTime::fromMSecsSinceStartOfDay(QTime::currentTime().msecsSinceStartOfDay());
  QDateTime zuluDateTime(zuluDate, zuluTime, Qt::UTC);
  data.setZuluTime(zuluDateTime);

  dataId++;

  curPos = next;
  return true;

#endif
}

#if defined(Q_OS_WIN32)
void SimConnectHandler::dispatchProcedure(SIMCONNECT_RECV *pData, DWORD cbData)
{
  Q_UNUSED(cbData);

  if(verbose)
    qDebug() << "DispatchProcedure entered";

  switch(pData->dwID)
  {
    case SIMCONNECT_RECV_ID_OPEN:
      {
        // enter code to handle SimConnect version information received in a SIMCONNECT_RECV_OPEN structure.
        SIMCONNECT_RECV_OPEN *openData = (SIMCONNECT_RECV_OPEN *)pData;

        // Print some useful simconnect interface data to log
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
        state = sc::SIMCONNECT_EXCEPTION;
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

        if(pObjData->dwRequestID == DATA_REQUEST_ID_USER)
        {
          if(verbose)
            qDebug() << "DATA_REQUEST_ID";
          DWORD objectID = pObjData->dwObjectID;
          SimData *simDataPtr = reinterpret_cast<SimData *>(&pObjData->dwData);

          if(verbose)
            qDebug() << "ObjectID" << objectID << "Title" << simDataPtr->aircraft.aircraftTitle
                     << "atcType" << simDataPtr->aircraft.aircraftAtcType
                     << "atcModel" << simDataPtr->aircraft.aircraftAtcModel
                     << "atcId" << simDataPtr->aircraft.aircraftAtcId
                     << "atcAirline" << simDataPtr->aircraft.aircraftAtcAirline
                     << "atcFlightNumber" << simDataPtr->aircraft.aircraftAtcFlightNumber
                     << "category" << simDataPtr->aircraft.category
                     << "numEngines" << simDataPtr->aircraft.numEngines
                     << "engineType" << simDataPtr->aircraft.engineType
                     << "Lat" << simDataPtr->aircraft.latitudeDeg
                     << "Lon" << simDataPtr->aircraft.longitudeDeg
                     << "Alt" << simDataPtr->aircraft.altitudeFt
                     << "ias" << simDataPtr->aircraft.airspeedIndicatedKts
                     << "gs" << simDataPtr->aircraft.groundVelocityKts
                     << "vs" << simDataPtr->aircraft.verticalSpeedFps
                     << "course " << simDataPtr->aircraft.planeHeadingMagneticDeg
                     << "M" << simDataPtr->aircraft.planeHeadingTrueDeg << "T"
                     << "wind" << simDataPtr->ambientWindDirectionDegT
                     << "/" << simDataPtr->ambientWindVelocityKts
                     << "magvar" << simDataPtr->aircraft.magVarDeg
                     << "local time" << simDataPtr->localTime
                     << "local year" << simDataPtr->localYear
                     << "local month" << simDataPtr->localMonth
                     << "local day" << simDataPtr->localDay
                     << "zulu time" << simDataPtr->zuluTimeSeconds
                     << "zulu year" << simDataPtr->zuluYear
                     << "zulu month" << simDataPtr->zuluMonth
                     << "zulu day" << simDataPtr->zuluDay
            ;
          simData = *simDataPtr;
          dataFetched = true;
          userDataFetched = true;
        }
        else if(pObjData->dwRequestID == DATA_REQUEST_ID_AI)
        {
          if(verbose)
            qDebug() << "DATA_REQUEST_AC_ID";
          DWORD objectID = pObjData->dwObjectID;
          SimDataAircraft *simDataAircraftPtr = reinterpret_cast<SimDataAircraft *>(&pObjData->dwData);
          if(SUCCEEDED(StringCbLengthA(&simDataAircraftPtr->aircraftTitle[0],
                                       sizeof(simDataAircraftPtr->aircraftTitle),
                                       NULL))) // security check
          {
            if(verbose)
              qDebug() << "ObjectID" << objectID << "Title" << simDataAircraftPtr->aircraftTitle
                       << "atcType" << simDataAircraftPtr->aircraftAtcType
                       << "atcModel" << simDataAircraftPtr->aircraftAtcModel
                       << "atcId" << simDataAircraftPtr->aircraftAtcId
                       << "atcAirline" << simDataAircraftPtr->aircraftAtcAirline
                       << "atcFlightNumber" << simDataAircraftPtr->aircraftAtcFlightNumber
                       << "category" << simDataAircraftPtr->category
                       << "numEngines" << simDataAircraftPtr->numEngines
                       << "engineType" << simDataAircraftPtr->engineType
                       << "Lat" << simDataAircraftPtr->latitudeDeg
                       << "Lon" << simDataAircraftPtr->longitudeDeg
                       << "Alt" << simDataAircraftPtr->altitudeFt
                       << "ias" << simDataAircraftPtr->airspeedIndicatedKts
                       << "gs" << simDataAircraftPtr->groundVelocityKts
                       << "vs" << simDataAircraftPtr->verticalSpeedFps
                       << "course " << simDataAircraftPtr->planeHeadingMagneticDeg
                       << "M" << simDataAircraftPtr->planeHeadingTrueDeg << "T"
                       << "magvar" << simDataAircraftPtr->magVarDeg
              ;

            simDataAircraft.append(*simDataAircraftPtr);
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
  if(verbose)
    qDebug() << "DispatchProcedure finished";
}

void CALLBACK SimConnectHandler::dispatchCallback(SIMCONNECT_RECV *pData, DWORD cbData, void *pContext)
{
  SimConnectHandler *handlerClass = static_cast<SimConnectHandler *>(pContext);
  handlerClass->dispatchProcedure(pData, cbData);
}

void SimConnectHandler::copyToSimData(const SimDataAircraft& simDataUserAircraft,
                                      atools::fs::sc::SimConnectData& data)
{
  data.setAirplaneTitle(simDataUserAircraft.aircraftTitle);
  data.setAirplaneModel(simDataUserAircraft.aircraftAtcModel);
  data.setAirplaneRegistration(simDataUserAircraft.aircraftAtcId);
  data.setAirplaneType(simDataUserAircraft.aircraftAtcType);
  data.setAirplaneAirline(simDataUserAircraft.aircraftAtcAirline);
  data.setAirplaneFlightnumber(simDataUserAircraft.aircraftAtcFlightNumber);

  data.getPosition().setLonX(simDataUserAircraft.longitudeDeg);
  data.getPosition().setLatY(simDataUserAircraft.latitudeDeg);
  data.getPosition().setAltitude(simDataUserAircraft.altitudeFt);

  data.setGroundSpeedKts(simDataUserAircraft.groundVelocityKts);
  data.setIndicatedAltitudeFt(simDataUserAircraft.indicatedAltitudeFt);
  data.setAltitudeAboveGroundFt(simData.planeAboveGroundFt);
  data.setHeadingDegMag(simDataUserAircraft.planeHeadingMagneticDeg);
  data.setHeadingDegTrue(simDataUserAircraft.planeHeadingTrueDeg);
  data.setTrackDegMag(simDataUserAircraft.planeTrackMagneticDeg);
  data.setTrackDegTrue(simDataUserAircraft.planeTrackTrueDeg);

  data.setTrueSpeedKts(simDataUserAircraft.airspeedTrueKts);
  data.setIndicatedSpeedKts(simDataUserAircraft.airspeedIndicatedKts);
  data.setMachSpeed(simDataUserAircraft.airspeedMach);
  data.setVerticalSpeedFeetPerMin(simDataUserAircraft.verticalSpeedFps * 60.f);

  if(simDataUserAircraft.isSimOnGround > 0)
    data.getFlags() |= atools::fs::sc::ON_GROUND;

  data.setMagVarDeg(simDataUserAircraft.magVarDeg);
}

void SimConnectHandler::fillDataDefinitionAicraft(DataDefinitionId definitionId)
{
  // Set up the data definition, but do not yet do anything with it
  HRESULT hr = SimConnect_AddToDataDefinition(hSimConnect, definitionId, "Title", NULL,
                                              SIMCONNECT_DATATYPE_STRING256);

  hr = SimConnect_AddToDataDefinition(hSimConnect, definitionId, "ATC Type", NULL,
                                      SIMCONNECT_DATATYPE_STRING32);

  hr = SimConnect_AddToDataDefinition(hSimConnect, definitionId, "ATC Model", NULL,
                                      SIMCONNECT_DATATYPE_STRING32);

  hr = SimConnect_AddToDataDefinition(hSimConnect, definitionId, "ATC Id", NULL,
                                      SIMCONNECT_DATATYPE_STRING32);

  hr = SimConnect_AddToDataDefinition(hSimConnect, definitionId, "ATC Airline", NULL,
                                      SIMCONNECT_DATATYPE_STRING64);

  hr = SimConnect_AddToDataDefinition(hSimConnect, definitionId, "ATC Flight Number", NULL,
                                      SIMCONNECT_DATATYPE_STRING32);

  hr = SimConnect_AddToDataDefinition(hSimConnect, definitionId, "Category", NULL,
                                      SIMCONNECT_DATATYPE_STRING32);

  hr = SimConnect_AddToDataDefinition(hSimConnect, definitionId, "Plane Altitude", "feet",
                                      SIMCONNECT_DATATYPE_FLOAT32);
  hr = SimConnect_AddToDataDefinition(hSimConnect, definitionId, "Plane Latitude", "degrees",
                                      SIMCONNECT_DATATYPE_FLOAT32);
  hr = SimConnect_AddToDataDefinition(hSimConnect, definitionId, "Plane Longitude", "degrees",
                                      SIMCONNECT_DATATYPE_FLOAT32);

  hr = SimConnect_AddToDataDefinition(hSimConnect, definitionId, "Ground Velocity", "knots",
                                      SIMCONNECT_DATATYPE_FLOAT32);
  hr = SimConnect_AddToDataDefinition(hSimConnect, definitionId, "Indicated Altitude", "feet",
                                      SIMCONNECT_DATATYPE_FLOAT32);

  hr = SimConnect_AddToDataDefinition(hSimConnect, definitionId, "Plane Heading Degrees Magnetic",
                                      "degrees", SIMCONNECT_DATATYPE_FLOAT32);
  hr =
    SimConnect_AddToDataDefinition(hSimConnect, definitionId, "Plane Heading Degrees True", "degrees",
                                   SIMCONNECT_DATATYPE_FLOAT32);
  hr = SimConnect_AddToDataDefinition(hSimConnect, definitionId, "GPS Ground Magnetic Track",
                                      "degrees", SIMCONNECT_DATATYPE_FLOAT32);
  hr = SimConnect_AddToDataDefinition(hSimConnect, definitionId, "GPS Ground True Track", "degrees",
                                      SIMCONNECT_DATATYPE_FLOAT32);

  hr = SimConnect_AddToDataDefinition(hSimConnect, definitionId, "Sim On Ground", "bool",
                                      SIMCONNECT_DATATYPE_INT32);

  hr = SimConnect_AddToDataDefinition(hSimConnect, definitionId, "Airspeed True", "knots",
                                      SIMCONNECT_DATATYPE_FLOAT32);
  hr = SimConnect_AddToDataDefinition(hSimConnect, definitionId, "Airspeed Indicated", "knots",
                                      SIMCONNECT_DATATYPE_FLOAT32);
  hr = SimConnect_AddToDataDefinition(hSimConnect, definitionId, "Airspeed Mach", "mach",
                                      SIMCONNECT_DATATYPE_FLOAT32);
  hr = SimConnect_AddToDataDefinition(hSimConnect, definitionId, "Vertical Speed", "feet per second",
                                      SIMCONNECT_DATATYPE_FLOAT32);

  hr = SimConnect_AddToDataDefinition(hSimConnect, definitionId, "Magvar",
                                      "degrees", SIMCONNECT_DATATYPE_FLOAT32);

  hr = SimConnect_AddToDataDefinition(hSimConnect, definitionId, "Number of Engines", "number",
                                      SIMCONNECT_DATATYPE_INT32);

  hr = SimConnect_AddToDataDefinition(hSimConnect, definitionId, "Engine Type", "number",
                                      SIMCONNECT_DATATYPE_INT32);
}

#endif
