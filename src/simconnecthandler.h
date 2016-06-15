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

#ifndef LITTLENAVCONNECT_SIMCONNECTHANDLER_H
#define LITTLENAVCONNECT_SIMCONNECTHANDLER_H

#include <QtGlobal>

#if defined(Q_OS_WIN32)
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>
#include "SimConnect.h"
#endif

namespace atools {
namespace fs {
namespace sc {
class SimConnectData;
}
}
}

namespace sc {
/* Status of the last operation when fetching data. */
enum State
{
  OK,
  FETCH_ERROR,
  OPEN_ERROR,
  DISCONNECTED,
  SIMCONNECT_EXCEPTION
};

}

/* Reads data synchronously from Fs simconnect interfaces.
 *  For non windows platforms contains also a simple aircraft simulation. */
class SimConnectHandler
{
public:
  SimConnectHandler(bool verboseLogging = false);
  virtual ~SimConnectHandler();

  /* Connect to fs.. Returns true it successful. */
  bool connect();

  /* Fetch data from simulator. Returns false if no data was retrieved due to paused or not running fs. */
  bool fetchData(atools::fs::sc::SimConnectData& data);

  /* true if simulator is running and not stuck in open dialogs. */
  bool isSimRunning() const
  {
    return simRunning;
  }

  bool isSimPaused() const
  {
    return simPaused;
  }

  /* Get state of last call. */
  sc::State getState() const
  {
    return state;
  }

  /* Struct that will be filled with raw data from the simconnect interface. */
  struct SimData
  {
    char aircraftTitle[256];
    char aircraftAtcType[32];
    char aircraftAtcModel[32];
    char aircraftAtcId[32];
    char aircraftAtcAirline[64];
    char aircraftAtcFlightNumber[32];
    float altitudeFt;
    float latitudeDeg;
    float longitudeDeg;

    float groundVelocityKts;
    float indicatedAltitudeFt;

    float planeAboveGroundFt;
    float planeHeadingMagneticDeg;
    float planeHeadingTrueDeg;
    float planeTrackMagneticDeg;
    float planeTrackTrueDeg;
    float groundAltitudeFt;
    qint32 isSimOnGround;

    float airspeedTrueKts;
    float airspeedIndicatedKts;
    float airspeedMach;
    float verticalSpeedFps;

    float ambientTemperatureC;
    float totalAirTemperatureC;
    float ambientWindVelocityKts;
    float ambientWindDirectionDegT;

    qint32 ambientPrecipStateFlags;
    qint32 ambientIsInCloud;
    float ambientVisibilityMeter;
    float seaLevelPressureMbar;
    float pitotIcePercent;
    float structuralIcePercent;

    float airplaneTotalWeightLbs;
    float airplaneMaxGrossWeightLbs;
    float airplaneEmptyWeightLbs;
    float fuelTotalQuantityGallons;
    float fuelTotalWeightLbs;

    float fuelFlowPph1;
    float fuelFlowPph2;
    float fuelFlowPph3;
    float fuelFlowPph4;

    float fuelFlowGph1;
    float fuelFlowGph2;
    float fuelFlowGph3;
    float fuelFlowGph4;
    float magVarDeg;
    qint32 localTime;
    qint32 localYear;
    qint32 localMonth;
    qint32 localDay;
    qint32 zuluTimeSeconds;
    qint32 zuluYear;
    qint32 zuluMonth;
    qint32 zuluDay;
    qint32 timeZoneOffsetSeconds;
  };

private:
#if defined(Q_OS_WIN32)
  /* Callback receiving the data. */
  void DispatchProcedure(SIMCONNECT_RECV *pData, DWORD cbData);

  /* Static method will pass call to object which is passed in pContext. */
  static void CALLBACK DispatchCallback(SIMCONNECT_RECV *pData, DWORD cbData, void *pContext);

  HANDLE hSimConnect = NULL;
#endif

  SimData simData;
  bool simRunning = true, simPaused = false, verbose = false, dataFetched = false;
  sc::State state = sc::OK;
};

#endif // LITTLENAVCONNECT_SIMCONNECTHANDLER_H
