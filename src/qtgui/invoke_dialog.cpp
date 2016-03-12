#include "master.hpp"
#include "qtgui/invoke_dialog.hpp"
#include "qtgui/main_window.hpp"
#include "qtgui/my_line_edit.hpp"

#include <QFormLayout>
#include <QHideEvent>
#include <QLabel>
#include <QListWidget>
#include <QShowEvent>

InvokeDialog::InvokeDialog(QWidget* parent, MainWindow* mw) : QDialog(parent, Qt::Popup),
    main_window(mw) {
  QFormLayout* flayout = new QFormLayout;

  {
    QLabel* header = new QLabel("Invoke a command...");
    QFont font = header->font();
    font.setBold(true);
    font.setPointSize(12);
    header->setFont(font);
    header->setAlignment(Qt::AlignCenter);
    flayout->addRow(header);
  }

  command_edit = new MyLineEdit(this);
  command_edit->setMinimumSize(300, 0);
  connect(command_edit, &QLineEdit::returnPressed, this, &InvokeDialog::slot_text_return);
  connect(command_edit, &MyLineEdit::key_up, this, &InvokeDialog::slot_list_up);
  connect(command_edit, &MyLineEdit::key_down, this, &InvokeDialog::slot_list_down);
  flayout->addRow("Command:", command_edit);

  command_list = new QListWidget(this);
  flayout->addRow(command_list);

  setLayout(flayout);
}

void InvokeDialog::show() {
  QDialog::show();
}

void InvokeDialog::showEvent(QShowEvent* event) {
  // Populate command_list
  std::vector<std::string> invokes;
  master.get_past_invokes(invokes);
  command_list->clear();
  int i = 0;
  for (std::string& s : invokes) {
    QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(s), nullptr, i);
    command_list->insertItem(i, item);
    i++;
  }
  command_list->setCurrentRow(0);

  // Populate command_edit
  command_edit->setFocus(Qt::ActiveWindowFocusReason);
  if (invokes.size() > 0) {
    command_edit->setText(QString::fromStdString(invokes[0]));
  }
  command_edit->selectAll();

  QDialog::showEvent(event);
}

void InvokeDialog::hideEvent(QHideEvent*  event) {
  QDialog::hideEvent(event);
}

void InvokeDialog::slot_text_return() {
  std::string cmd = command_edit->text().toStdString();
  master.add_invoke(cmd);
  master.run_tool(cmd, true, ".");
  hide();
}

void InvokeDialog::slot_list_up() {
  int current = command_list->currentRow();
  if (current < 1) command_list->setCurrentRow(0);
  else command_list->setCurrentRow(current-1);

  QString current_text = command_list->currentItem()->text();
  command_edit->setText(current_text);
  command_edit->selectAll();
}

void InvokeDialog::slot_list_down() {
  int current = command_list->currentRow();
  int num_rows = command_list->count();
  if (current < 0) command_list->setCurrentRow(0);
  else if (current >= num_rows - 1) command_list->setCurrentRow(num_rows - 1);
  else command_list->setCurrentRow(current+1);

  QString current_text = command_list->currentItem()->text();
  command_edit->setText(current_text);
  command_edit->selectAll();
}