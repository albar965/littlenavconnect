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

namespace atools {
namespace fs {
class SimConnectData;
}
}

class SimConnectHandler
{
public:
  SimConnectHandler();
  virtual ~SimConnectHandler();
  void initialize();

  void fetchData(atools::fs::SimConnectData& data);

  struct SimData
  {
    char title[256];
    double altitude;
    double latitude;
    double longitude;

    double groundVelocity;
    double indicatedAltitude;

    double planeAboveGround;
    double planeHeadingMagnetic;
    double planeHeadingTrue;
    double groundAltitude;
    qint64 simOnGround;

    double airspeedTrue;
    double airspeedIndicated;
    double airspeedMach;
    double verticalSpeed;

    double ambientTemperture;
    double ambientPressure;
    double ambientWindVelocity;
    double ambientWindDirection;
    qint64 ambientPrecipState;

    double aircraftWindX;
    double aircraftWindY;
    double aircraftWindZ;

    qint64 ambientInCloud;
    double seaLevelPressure;
  };

private:
#if defined(Q_OS_WIN32)
  void DispatchProcedure(SIMCONNECT_RECV *pData, DWORD cbData);

  static void CALLBACK MyDispatchProcRD(SIMCONNECT_RECV *pData, DWORD cbData, void *pContext);

  HANDLE hSimConnect = NULL;
#endif

  SimData simData;
  int dataId = 0;
};

#endif // SIMCONNECTHANDLER_H
