#include "core/util.hpp"
#include "core/utf8_util.hpp"
#include "master.hpp"
#include "master_io_provider.hpp"
#include "qtgui/ssh_gui.hpp"

#include <stdexcept>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpacerItem>
#include <QSpinBox>
#include <QTextEdit>
#include <QVBoxLayout>

//////////// SSHConnectionsDialog

SSHConnectionsDialog::SSHConnectionsDialog(QWidget* parent) : QDialog(parent) {
  QHBoxLayout* layout = new QHBoxLayout;

  q_list = new QListWidget(this);
  connect(q_list, &QListWidget::currentRowChanged, this, &SSHConnectionsDialog::slot_row_changed);
  layout->addWidget(q_list);

  {
    QVBoxLayout* vlayout = new QVBoxLayout;

    q_connect_button = new QPushButton("Connect", this);
    connect(q_connect_button, &QPushButton::clicked, this, &SSHConnectionsDialog::slot_connect_clicked);
    vlayout->addWidget(q_connect_button);

    q_edit_button = new QPushButton("Edit...", this);
    connect(q_edit_button, &QPushButton::clicked, this, &SSHConnectionsDialog::slot_edit_clicked);
    vlayout->addWidget(q_edit_button);

    q_delete_button = new QPushButton("Delete", this);
    connect(q_delete_button, &QPushButton::clicked, this, &SSHConnectionsDialog::slot_delete_clicked);
    vlayout->addWidget(q_delete_button);

    vlayout->addItem(new QSpacerItem(20, 20));

    q_new_button = new QPushButton("New...", this);
    connect(q_new_button, &QPushButton::clicked, this, &SSHConnectionsDialog::slot_new_clicked);
    vlayout->addWidget(q_new_button);

    vlayout->addItem(new QSpacerItem(20, 20));

    q_cancel_button = new QPushButton("Cancel", this);
    connect(q_cancel_button, &QPushButton::clicked, this, &SSHConnectionsDialog::slot_cancel_clicked);
    vlayout->addWidget(q_cancel_button);

    layout->addLayout(vlayout);
  }

  setWindowTitle("SSH Connections");
  setLayout(layout);

  ssh_properties_dialog = new SSHPropertiesDialog(this, this);
  ssh_wizard_dialog = new SSHWizardDialog(this, this);
}

int SSHConnectionsDialog::do_dialog() {
  connections = master.settings.get_ssh_connections();
  refresh_list();
  return exec();
}

void SSHConnectionsDialog::refresh_list() {
  q_list->clear();
  for (SSHConnectionInfo& sci : connections) {
    q_list->addItem(QString::fromStdString(sci.name));
  }
  if (connections.size() > 0) {
    q_list->setCurrentRow(0);
  } else {
    slot_row_changed(-1);
  }
}

void SSHConnectionsDialog::slot_connect_clicked() {
  int row = q_list->currentRow();
  if (row < 0) return;
  chosen_connection = &connections[row];
  this->accept();
}

void SSHConnectionsDialog::slot_delete_clicked() {
  int row = q_list->currentRow();
  if (row < 0) return;
  connections.erase(connections.begin() + row);
  refresh_list();
}

void SSHConnectionsDialog::slot_edit_clicked() {
  int row = q_list->currentRow();
  if (row < 0) return;
  SSHConnectionInfo* sci = &connections[row];
  if (ssh_properties_dialog->do_dialog(sci)) {
    master.settings.save_ssh_connections(connections);
    refresh_list();
  }
}

void SSHConnectionsDialog::slot_new_clicked() {
  SSHConnectionInfo sci;
  if (ssh_wizard_dialog->do_dialog(&sci)) {
    connections.push_back(sci);
    master.settings.save_ssh_connections(connections);
    if (ssh_properties_dialog->do_dialog(&connections.back())) {
      master.settings.save_ssh_connections(connections);
    }
    refresh_list();
  }
}

void SSHConnectionsDialog::slot_cancel_clicked() {
  reject();
}

void SSHConnectionsDialog::slot_row_changed(int row) {
  if (row < 0) {
    q_connect_button->setEnabled(false);
    q_edit_button->setEnabled(false);
    q_delete_button->setEnabled(false);
  } else {
    q_connect_button->setEnabled(true);
    q_edit_button->setEnabled(true);
    q_delete_button->setEnabled(true);
  }
}

//////////// SSHPropertiesDialog

SSHPropertiesDialog::SSHPropertiesDialog(QWidget* parent, SSHConnectionsDialog* d) : QDialog(parent), connections_dialog(d) {
  QFormLayout* flayout = new QFormLayout;
  flayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

  q_name = new QLineEdit(this);
  flayout->addRow("Name:", q_name);

  q_command_line = new QLineEdit(this);
  flayout->addRow("Command line:", q_command_line);

  q_events = new QPlainTextEdit(this);
  q_events->setAutoFillBackground(true);
  q_events->setBackgroundVisible(true);
  // Somehow the textEdit doesn't get properly styled, so hack it.
  q_events->setObjectName("textEdit");

  flayout->addRow("Login events:", q_events);

  {
    QLabel* lbl = new QLabel("NOTE: Use these settings to control the raw interface of the underlying SSH implementation.  Use the New Connection Wizard to initialize settings for common SSH configurations.", this);
    lbl->setWordWrap(true);
    flayout->addRow(lbl);
  }

  QDialogButtonBox* q_bbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  connect(q_bbox, &QDialogButtonBox::accepted, this, &SSHPropertiesDialog::slot_dialog_accepted);
  connect(q_bbox, &QDialogButtonBox::rejected, this, &SSHPropertiesDialog::slot_dialog_rejected);
  flayout->addRow(q_bbox);

  setWindowTitle("SSH Connection Properties");
  setLayout(flayout);
}

int SSHPropertiesDialog::do_dialog(SSHConnectionInfo* sci) {
  current_info = sci;

  q_name->setText(QString::fromStdString(sci->name));
  q_command_line->setText(QString::fromStdString(sci->cmd_line));
  q_events->setPlainText(QString::fromStdString(sci->login_events));

  return exec();
}

void SSHPropertiesDialog::slot_dialog_accepted() {
  std::string connection_name = utf8_strip(q_name->text().toStdString());
  if (connection_name.empty()) {
    QMessageBox::critical(this, "Invalid SSH connection", "Invalid SSH connection title");
    return;
  }

  std::string cmd_line = utf8_strip(q_command_line->text().toStdString());
  if (cmd_line.empty()) {
    QMessageBox::critical(this, "Invalid SSH connection", "Invalid SSH connection command line");
    return;
  }

  std::string login_events = q_events->toPlainText().toStdString();
  current_info->name = connection_name;
  current_info->cmd_line = cmd_line;
  current_info->login_events = login_events;
  accept();
}

void SSHPropertiesDialog::slot_dialog_rejected() {
  reject();
}

//////////// SSHWizardDialog

SSHWizardDialog::SSHWizardDialog(QWidget* parent, SSHConnectionsDialog* d) : QDialog(parent), connections_dialog(d) {
  QFormLayout* flayout = new QFormLayout;
  flayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

  q_name = new QLineEdit(this);
  flayout->addRow("Name:", q_name);

  {
    flayout->addItem(new QSpacerItem(10, 10));
    QLabel* lbl = new QLabel("Connection", this);
    QFont font = lbl->font(); font.setBold(true); lbl->setFont(font);
    flayout->addRow(lbl);
  }

  q_host = new QLineEdit(this);
  flayout->addRow("SSH hostname or IP:", q_host);

  q_port = new QSpinBox(this);
  q_port->setValue(22);
  q_port->setMinimum(0);
  q_port->setMaximum(65535);
  flayout->addRow("Port:", q_port);

  {
    flayout->addItem(new QSpacerItem(10, 10));
    QLabel* lbl = new QLabel("Credentials", this);
    QFont font = lbl->font(); font.setBold(true); lbl->setFont(font);
    flayout->addRow(lbl);
  }

  q_user = new QLineEdit(this);
  flayout->addRow("Username:", q_user);

  q_password = new QLineEdit(this);
  flayout->addRow("Password:", q_password);
  {
    QLabel* lbl = new QLabel("NOTE: Only enter a password if you need to enter it every time you log in via SSH.  If you are using a public key (encrypted or unencrypted), then leave this blank.", this);
    lbl->setMinimumHeight(70);
    lbl->setWordWrap(true);
    lbl->setMinimumWidth(350);
    flayout->addRow(lbl);
  }

  {
    flayout->addItem(new QSpacerItem(10, 10));
    QLabel* lbl = new QLabel("After the connection", this);
    QFont font = lbl->font(); font.setBold(true); lbl->setFont(font);
    flayout->addRow(lbl);
  }

  {
    QLabel* lbl = new QLabel("You may enter a command to execute after logging in to the remote server.  For example, 'sudo' , 'su' or tunnel to another server.", this);
    lbl->setMinimumWidth(350);
    lbl->setMinimumHeight(70);
    lbl->setWordWrap(true);
    flayout->addRow(lbl);
  }
  q_post_command = new QLineEdit(this);
  flayout->addRow("Command:", q_post_command);
  q_post_password = new QLineEdit(this);
  flayout->addRow("Password (optional):", q_post_password);

  QDialogButtonBox* q_bbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  connect(q_bbox, &QDialogButtonBox::accepted, this, &SSHWizardDialog::slot_dialog_accepted);
  connect(q_bbox, &QDialogButtonBox::rejected, this, &SSHWizardDialog::slot_dialog_rejected);
  flayout->addRow(q_bbox);

  setWindowTitle("SSH Connection Wizard");
  setLayout(flayout);

  // N.B.: Without this the preferences are not properly centered at beginning. Not sure why.
  updateGeometry();
  adjustSize();
}

int SSHWizardDialog::do_dialog(SSHConnectionInfo* cur) {
  current_info = cur;

  q_name->setText("");
  q_name->setFocus();
  q_host->setText("");
  q_user->setText("");
  q_password->setText("");
  q_post_command->setText("");
  q_post_password->setText("");
  q_port->setValue(22);

  return exec();
}

void SSHWizardDialog::slot_dialog_accepted() {
  std::string name = utf8_strip(q_name->text().toStdString());
  std::string host = utf8_strip(q_host->text().toStdString());
  std::string user = utf8_strip(q_user->text().toStdString());
  std::string password = utf8_strip(q_password->text().toStdString());
  int port = q_port->value();
  std::string post_command = q_post_command->text().toStdString();
  std::string post_password = q_post_password->text().toStdString();

  if (name.empty() || user.empty() || host.empty()) {
    QMessageBox::critical(this, "Invalid SSH connection", "Invalid SSH connection");
    return;
  }

  std::string cmd_line;
  std::string login_events;

  #ifdef CMAKE_WINDOWS
  cmd_line = std::string("\"") + under_root("plink.exe") + "\" -C -P " + std::to_string(port) + " " + user + "@" + host + " bash --norc --noprofile";
  #else
  cmd_line = "ssh -p " + std::to_string(port) + " " + user + "@" + host + " bash --norc --noprofile";
  #endif
  if (!password.empty()) {
    login_events += "MATCH password\nWRITE_NO_ECHO " + password + "\n";
  }
  login_events += "WAIT 1000\nWRITE unset HISTFILE\nWRITE unset PS1\nWRITE echo SYNTAXIC_LOGGED_IN\nMATCH_CASE SYNTAXIC_LOGGED_IN\n";
  bool unset_again = false;
  if (!post_command.empty()) {
    login_events += "WRITE " + post_command + "\n";
    login_events += "WAIT 200\n";
    unset_again = true;
  }
  if (!post_password.empty()) {
    login_events += "WRITE " + post_password + "\n";
    login_events += "WAIT 200\n";
    unset_again = true;
  }
  if (unset_again) login_events += "WRITE unset HISTFILE\nWRITE unset PS1\n";
  login_events += "WRITE echo SYNTAXIC_READY\nMATCH_CASE SYNTAXIC_READY\n";

  current_info->name = name;
  current_info->cmd_line = cmd_line;
  current_info->login_events = login_events;
  accept();
}

void SSHWizardDialog::slot_dialog_rejected() {
  reject();
}