#ifndef SYNTAXIC_REGISTER_DIALOG_HPP
#define SYNTAXIC_REGISTER_DIALOG_HPP

#include <QDialog>

class QLabel;
class QLineEdit;

class RegisterDialog : public QDialog {
  Q_OBJECT

private:
  QLabel* feedback_label;
  QLineEdit* email_edit;
  QLineEdit* key_edit;

public:
  RegisterDialog(QWidget* parent);

  virtual void showEvent(QShowEvent* event);

public slots:

  void slot_buy_clicked();
  void slot_accepted();
  void slot_rejected();
};

#endif