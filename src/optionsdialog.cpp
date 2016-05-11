#include "optionsdialog.h"
#include "ui_optionsdialog.h"

OptionsDialog::OptionsDialog(QWidget *parent) :
  QDialog(parent), ui(new Ui::OptionsDialog)
{
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

void OptionsDialog::setPort(int port)
{
  ui->spinBoxOptionsPort->setValue(port);
}

void OptionsDialog::setUpdateRate(unsigned int ms)
{
  ui->spinBoxOptionsUpdateRate->setValue(static_cast<int>(ms));
}
