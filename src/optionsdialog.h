#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QDialog>

namespace Ui {
class OptionsDialog;
}

class OptionsDialog : public QDialog
{
  Q_OBJECT

public:
  explicit OptionsDialog(QWidget *parent = 0);
  ~OptionsDialog();

  int getPort() const;
  unsigned int getUpdateRate() const;

  void setPort(int port);
  void setUpdateRate(unsigned int ms);

private:
  Ui::OptionsDialog *ui;
};

#endif // OPTIONSDIALOG_H
