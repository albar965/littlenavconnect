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

#ifndef SIMCONNECTHANDLER_H
#define SIMCONNECTHANDLER_H

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
enum State
{
  OK,
  FETCH_ERROR,
  OPEN_ERROR,
  DISCONNECTED
};

}

class SimConnectHandler
{
public:
  SimConnectHandler(bool verboseLogging = false);
  virtual ~SimConnectHandler();
  bool connect();

  bool fetchData(atools::fs::sc::SimConnectData& data);

  bool isSimRunning() const
  {
    return simRunning;
  }

  bool isSimPaused() const
  {
    return simPaused;
  }

  sc::State getState() const
  {
    return state;
  }

  struct SimData
  {
    char title[256];
    char atcType[32];
    char atcModel[32];
    char atcId[32];
    char atcAirline[64];
    char atcFlightNumber[32];
    float altitude;
    float latitude;
    float longitude;

    float groundVelocity;
    float indicatedAltitude;

    float planeAboveGround;
    float planeHeadingMagnetic;
    float planeHeadingTrue;
    float groundAltitude;
    qint32 simOnGround;

    float airspeedTrue;
    float airspeedIndicated;
    float airspeedMach;
    float verticalSpeed;

    // float ambientTemperture;
    // float ambientPressure;
    float ambientWindVelocity;
    float ambientWindDirection;

    // qint64 ambientPrecipState;
    // float aircraftWindX;
    // float aircraftWindY;
    // float aircraftWindZ;
    // qint64 ambientInCloud;
    // float seaLevelPressure;
  };

private:
#if defined(Q_OS_WIN32)
  void DispatchProcedure(SIMCONNECT_RECV *pData, DWORD cbData);

  static void CALLBACK DispatchCallback(SIMCONNECT_RECV *pData, DWORD cbData, void *pContext);

  HANDLE hSimConnect = NULL;
#endif

  SimData simData;
  bool simRunning = true, simPaused = false, verbose = false, dataFetched = false;
  sc::State state = sc::OK;
};

#endif // SIMCONNECTHANDLER_H
