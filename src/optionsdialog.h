/*****************************************************************************
* Copyright 2015-2025 Alexander Barthel alex@littlenavmap.org
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

#ifndef LITTLENAVCONNECT_OPTIONSDIALOG_H
#define LITTLENAVCONNECT_OPTIONSDIALOG_H

#include <QDialog>

namespace Ui {
class OptionsDialog;
}

/*
 * Options dialog. Passive and allows only to set and get option values
 */
class OptionsDialog :
  public QDialog
{
  Q_OBJECT

public:
  explicit OptionsDialog(QWidget *parent);
  virtual ~OptionsDialog() override;

  OptionsDialog(const OptionsDialog& other) = delete;
  OptionsDialog& operator=(const OptionsDialog& other) = delete;

  int getPort() const;
  unsigned int getUpdateRate() const;
  bool isHideHostname() const;
  bool isFetchAiAircraft() const;
  bool isFetchAiShip() const;
  int getAiFetchRadiusNm() const;

  void setPort(int port);
  void setUpdateRate(unsigned int ms);
  void setHideHostname(bool hide);
  void setFetchAiAircraft(bool value);
  void setFetchAiShip(bool value);
  void setFetchAiRadius(int radiusNm);

private:
  Ui::OptionsDialog *ui;
};

#endif // LITTLENAVCONNECT_OPTIONSDIALOG_H
