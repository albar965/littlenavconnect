/*****************************************************************************
* Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
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

#ifndef LITTLENAVCONNECT_CONSTANTS_H
#define LITTLENAVCONNECT_CONSTANTS_H

#include <QLatin1String>

namespace lnc {
/* key names for atools::settings */
const QLatin1String SETTINGS_OPTIONS_HIDE_HOSTNAME("Options/HideHostname");
const QLatin1String SETTINGS_OPTIONS_DEFAULT_PORT("Options/DefaultPort");
const QLatin1String SETTINGS_OPTIONS_UPDATE_RATE("Options/UpdateRate");
const QLatin1String SETTINGS_OPTIONS_FETCH_AI_AIRCRAFT("Options/FetchAiAircraft");
const QLatin1String SETTINGS_OPTIONS_FETCH_AI_SHIP("Options/FetchAiShip");
const QLatin1String SETTINGS_OPTIONS_FETCH_AI_RADIUS("Options/FetchAiRadius");
const QLatin1String SETTINGS_ACTIONS_SHOW_PORT_CHANGE("Actions/ShowPortChange");
const QLatin1String SETTINGS_ACTIONS_SHOW_QUIT("Actions/ShowQuit");
const QLatin1String SETTINGS_OPTIONS_VERBOSE("Options/Verbose");
const QLatin1String SETTINGS_MAINWINDOW_WIDGET("MainWindow/Widget");
const QLatin1String SETTINGS_OPTIONS_RECONNECT_RATE("Options/ReconnectRate2");
const QLatin1String SETTINGS_OPTIONS_LANGUAGE("Options/Language");
const QLatin1String SETTINGS_OPTIONS_SIMULATOR_FSX("Options/Simulator");
} // namespace lnc

#endif // LITTLENAVCONNECT_CONSTANTS_H
