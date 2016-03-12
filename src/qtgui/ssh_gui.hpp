#ifndef SYNTAXIC_QTGUI_SSH_GUI_HPP
#define SYNTAXIC_QTGUI_SSH_GUI_HPP

#include "settings.hpp"

#include <QDialog>
#include <string>
#include <vector>

class QLineEdit;
class QListWidget;
class QPushButton;
class QPlainTextEdit;
class QSpinBox;

class SSHConnectionsDialog;

class SSHWizardDialog : public QDialog {
private:
  QLineEdit* q_name;
  QLineEdit* q_host;
  QLineEdit* q_user;
  QLineEdit* q_password;
  QSpinBox* q_port;
  QLineEdit* q_post_command;
  QLineEdit* q_post_password;

  SSHConnectionsDialog* connections_dialog;
  SSHConnectionInfo* current_info;

public:
  SSHWizardDialog(QWidget* parent, SSHConnectionsDialog* d);

  int do_dialog(SSHConnectionInfo*);

public slots:
  void slot_dialog_accepted();
  void slot_dialog_rejected();
};

class SSHPropertiesDialog : public QDialog {
private:
  QLineEdit* q_name;
  QLineEdit* q_command_line;
  QPlainTextEdit* q_events;

  SSHConnectionsDialog* connections_dialog;
  SSHConnectionInfo* current_info;

public:
  SSHPropertiesDialog(QWidget* parent, SSHConnectionsDialog* d);

  int do_dialog(SSHConnectionInfo*);

public slots:
  void slot_dialog_accepted();
  void slot_dialog_rejected();
};

class SSHConnectionsDialog : public QDialog {
private:
  QListWidget* q_list;
  QPushButton* q_connect_button;
  QPushButton* q_edit_button;
  QPushButton* q_delete_button;
  QPushButton* q_new_button;
  QPushButton* q_cancel_button;

  SSHPropertiesDialog* ssh_properties_dialog;
  SSHWizardDialog* ssh_wizard_dialog;

  void refresh_list();

public:
  SSHConnectionsDialog(QWidget* parent);

  std::vector<SSHConnectionInfo> connections;
  SSHConnectionInfo* chosen_connection;

  /** Return 1 if should connect to chosen_connection. */
  int do_dialog();

public slots:
  void slot_connect_clicked();
  void slot_edit_clicked();
  void slot_delete_clicked();
  void slot_new_clicked();
  void slot_cancel_clicked();
  void slot_row_changed(int i);
};

#endif