#include "qtgui/jf_dialog.hpp"
#include "qtgui/main_window.hpp"
#include "qtgui/my_line_edit.hpp"
#include "core/util.hpp"

#include <QFormLayout>
#include <QHideEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QShowEvent>

JfDialog::JfDialog(QWidget* parent, MainWindow* mw) : QDialog(parent, Qt::Popup), main_window(mw) {
  QFormLayout* flayout = new QFormLayout;

  {
    QLabel* header = new QLabel("Jump to file...");
    QFont font = header->font();
    font.setBold(true);
    font.setPointSize(12);
    header->setFont(font);
    header->setAlignment(Qt::AlignCenter);
    flayout->addRow(header);
  }

  q_file_name = new MyLineEdit(this);
  q_file_name->setMinimumSize(300, 0);
  connect(q_file_name, &QLineEdit::textEdited, this, &JfDialog::slot_text_changed);
  connect(q_file_name, &QLineEdit::returnPressed, this, &JfDialog::slot_text_return);
  connect(q_file_name, &MyLineEdit::key_up, this, &JfDialog::slot_list_up);
  connect(q_file_name, &MyLineEdit::key_down, this, &JfDialog::slot_list_down);
  flayout->addRow("File name:", q_file_name);

  q_file_list = new QListWidget(this);
  q_file_list->setFocusPolicy(Qt::NoFocus);
  connect(q_file_list, &QListWidget::itemDoubleClicked, this, &JfDialog::slot_list_double_clicked);
  flayout->addRow(q_file_list);

  setLayout(flayout);
}

void JfDialog::repopulate_file_list() {
  q_file_list->clear();
  int i = 0;
  for (Choice* choice: choice_list.chosen) {
    KnownDocument& kd = known_documents[choice->num];
    std::string text = kd.file_name + "   (" + elide_right(kd.abs_path, 50) + ")";
    QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(text), nullptr, i);
    q_file_list->insertItem(i, item);
    i++;
  }
  q_file_list->setCurrentRow(0);
}

void JfDialog::show() {
  known_documents.clear();
  master.get_known_documents(known_documents);
  if (known_documents.empty()) {
    master.feedback("No documents", "No documents to jump to.");
    return;
  }
  QDialog::show();
}

void JfDialog::showEvent(QShowEvent* event) {
  QDialog::showEvent(event);
  q_file_name->setText("");
  q_file_name->setFocus();

  int i = 0;
  choice_list.choices.clear();
  for (KnownDocument& kd : known_documents) {
    choice_list.add_choice(kd.abs_path, i);
    i++;
  }
  choice_list.order_choices();
  repopulate_file_list();
}

void JfDialog::hideEvent(QHideEvent*  event ) {
  QDialog::hideEvent(event);
}

void JfDialog::slot_text_changed(const QString& new_text) {
  choice_list.refilter_choices(new_text.toStdString());
  repopulate_file_list();
}

void JfDialog::slot_text_return() {
  int current = q_file_list->currentRow();
  if (current < 0) current = 0;

  Choice* choice = choice_list.chosen[current];
  KnownDocument& kd = known_documents[choice->num];
  master.open_document(kd.abs_path.c_str(), main_window);
  hide();
}

void JfDialog::slot_list_double_clicked(QListWidgetItem* item) {
  Choice* choice = choice_list.chosen[item->type()];
  KnownDocument& kd = known_documents[choice->num];
  master.open_document(kd.abs_path.c_str(), main_window);
  hide();
}

void JfDialog::slot_list_up() {
  int current = q_file_list->currentRow();
  if (current < 1) q_file_list->setCurrentRow(0);
  else q_file_list->setCurrentRow(current-1);
}

void JfDialog::slot_list_down() {
  int current = q_file_list->currentRow();
  int num_rows = q_file_list->count();
  if (current < 0) q_file_list->setCurrentRow(0);
  else if (current >= num_rows - 1) q_file_list->setCurrentRow(num_rows - 1);
  else q_file_list->setCurrentRow(current+1);
}