/*****************************************************************************
* Copyright 2015-2024 Alexander Barthel alex@littlenavmap.org
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

#include "gui/dataexchange.h"

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
const QLatin1String SETTINGS_ACTIONS_SHOW_TRAY_HINT("Actions/ShowTrayHint");
const QLatin1String SETTINGS_OPTIONS_VERBOSE("Options/Verbose");
const QLatin1String SETTINGS_MAINWINDOW_WIDGET("MainWindow/Widget");
const QLatin1String SETTINGS_OPTIONS_RECONNECT_RATE("Options/ReconnectRate2");
const QLatin1String SETTINGS_OPTIONS_LANGUAGE("Options/Language");
const QLatin1String SETTINGS_OPTIONS_SIMULATOR_FSX("Options/Simulator");

const QLatin1String OPTIONS_APPLICATION("Little Navconnect");
const QLatin1String OPTIONS_APPLICATION_ORGANIZATION("ABarthel");
const QLatin1String OPTIONS_APPLICATION_DOMAIN("littlenavmap.org");

const QLatin1String STACKTRACE_SUFFIX("_stacktrace.txt");
const QLatin1String CRASHREPORTS_DIR("crashreports");
const QLatin1String CRASHREPORT_SUFFIX("_crashreport.zip");

const QLatin1String PROGRAM_GUID("919a3676-a86c-46cb-bafc-e78439fd0906");

const QLatin1String STARTUP_COMMAND_ACTIVATE(atools::gui::DataExchange::STARTUP_COMMAND_ACTIVATE); /* Bring window to front */
const QLatin1String STARTUP_COMMAND_QUIT(atools::gui::DataExchange::STARTUP_COMMAND_QUIT); /* Exit application */

const QLatin1String STARTUP_COMMAND_SAVE_REPLAY("save-replay");
const QLatin1String STARTUP_COMMAND_LOAD_REPLAY("load-replay");
const QLatin1String STARTUP_COMMAND_REPLAY_SPEED("replay-speed");
const QLatin1String STARTUP_COMMAND_WRITE_WHAZZUP("write-whazzup");
const QLatin1String STARTUP_COMMAND_WRITE_WHAZZUP_SPEED("write-whazzup-speed");
const QLatin1String STARTUP_COMMAND_REPLAY_GUI("replay-gui");

// "master" or "release/1.4" VERSION_NUMBER_TODO
const static QString HELP_BRANCH = "release/3.0";

#if defined(WINARCH64)
const QLatin1String SIMCONNECT_DLL_NAME("SimConnect_msfs_2020.dll");
#else
const QLatin1String SIMCONNECT_DLL_NAME("SimConnect.dll");
#endif

} // namespace lnc

#endif // LITTLENAVCONNECT_CONSTANTS_H
