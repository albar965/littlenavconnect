/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel alex@littlenavmap.org
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

#include "optionsdialog.h"
#include "ui_optionsdialog.h"

OptionsDialog::OptionsDialog(QWidget *parent) :
  QDialog(parent), ui(new Ui::OptionsDialog)
{
  setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
  setWindowModality(Qt::ApplicationModal);

  ui->setupUi(this);
  connect(ui->buttonBoxOptions, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(ui->buttonBoxOptions, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

OptionsDialog::~OptionsDialog()
{
  delete ui;
}

int OptionsDialog::getPort() const
{
  return ui->spinBoxOptionsPort->value();
}

unsigned int OptionsDialog::getUpdateRate() const
{
  return static_cast<unsigned int>(ui->spinBoxOptionsUpdateRate->value());
}

bool OptionsDialog::isHideHostname() const
{
  return ui->checkBoxHideHostname->isChecked();
}

bool OptionsDialog::isFetchAiAircraft() const
{
  return ui->checkBoxFetchAiAircraft->isChecked();
}

bool OptionsDialog::isFetchAiShip() const
{
  return ui->checkBoxFetchAiShip->isChecked();
}

void OptionsDialog::setHideHostname(bool hide)
{
  ui->checkBoxHideHostname->setChecked(hide);
}

void OptionsDialog::setFetchAiAircraft(bool value)
{
  ui->checkBoxFetchAiAircraft->setChecked(value);
}

void OptionsDialog::setFetchAiShip(bool value)
{
  ui->checkBoxFetchAiShip->setChecked(value);
}

void OptionsDialog::setPort(int port)
{
  ui->spinBoxOptionsPort->setValue(port);
}

void OptionsDialog::setUpdateRate(unsigned int ms)
{
  ui->spinBoxOptionsUpdateRate->setValue(static_cast<int>(ms));
}
