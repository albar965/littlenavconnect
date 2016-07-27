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

#ifndef LITTLENAVCONNECT_COMMON_H
#define LITTLENAVCONNECT_COMMON_H

#include <QLoggingCategory>

/* key names for atools::settings */
const QString SETTINGS_OPTIONS_DEFAULT_PORT = "Options/DefaultPort";
const QString SETTINGS_OPTIONS_UPDATE_RATE = "Options/UpdateRate";
const QString SETTINGS_ACTIONS_SHOW_PORT_CHANGE = "Actions/ShowPortChange";
const QString SETTINGS_ACTIONS_SHOW_QUIT = "Actions/ShowQuit";
const QString SETTINGS_OPTIONS_VERBOSE = "Options/Verbose";
const QString SETTINGS_MAINWINDOW_WIDGET = "MainWindow/Widget";
const QString SETTINGS_OPTIONS_RECONNECT_RATE = "Options/ReconnectRate";
const QString SETTINGS_OPTIONS_LANGUAGE = "Options/Language";

/* Declare a own logging category to append in the text edit */
Q_DECLARE_LOGGING_CATEGORY(gui);

#endif // LITTLENAVCONNECT_COMMON_H
