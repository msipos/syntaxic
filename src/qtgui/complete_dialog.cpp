#include "core/util.hpp"
#include "document.hpp"
#include "qtgui/complete_dialog.hpp"

#include <qtgui/main_window.hpp>
#include <qtgui/editor.hpp>
#include <QKeyEvent>
#include <QListWidget>
#include <QVBoxLayout>

CompleteDialog::CompleteDialog(QWidget* parent) : QDialog(parent, Qt::Popup), main_window(dynamic_cast<MainWindow*> (parent)), editor(nullptr) {
  QVBoxLayout* layout = new QVBoxLayout;
  layout->setContentsMargins(0, 0, 0, 0);

  q_list = new QListWidget(this);
  layout->addWidget(q_list);

  setLayout(layout);
}

bool CompleteDialog::repopulate_list(bool complete_prefix) {
  q_list->clear();
  Doc* doc = main_window->get_active_document();
  if (doc == nullptr) return false;
  std::vector<std::string> matches;
  doc->handle_complete(matches, complete_prefix);
  for (const std::string& s: matches) {
    q_list->addItem(QString::fromStdString(s));
  }

  if (!matches.empty()) {
    if (q_list->currentRow() < 0) q_list->setCurrentRow(0);
  }

  return !matches.empty();
}

void CompleteDialog::show(Editor* e) {
  editor = e;
  // Only show if there is a completion.
  if (repopulate_list(true)) {
    QDialog::show();
  }
}

void CompleteDialog::showEvent(QShowEvent* event) {
  QDialog::showEvent(event);
}

void CompleteDialog::forwardKey(QKeyEvent* event) {
  if (editor) editor->keyPressEvent(event);
  if (!repopulate_list(false)) hide();
}

void CompleteDialog::list_up() {
  int current = q_list->currentRow();
  if (current < 1) q_list->setCurrentRow(0);
  else q_list->setCurrentRow(current-1);
}

void CompleteDialog::list_down() {
  int current = q_list->currentRow();
  int num_rows = q_list->count();
  if (current < 0) q_list->setCurrentRow(0);
  else if (current >= num_rows - 1) q_list->setCurrentRow(num_rows - 1);
  else q_list->setCurrentRow(current+1);
}

void CompleteDialog::keyPressEvent(QKeyEvent* event) {
  event->accept();

  int key = event->key();
  Qt::KeyboardModifiers km = event->modifiers();
  bool alt = km & Qt::AltModifier;
  bool ctrl = km & Qt::ControlModifier;

  if (key == Qt::Key_Escape) { hide(); return; }
  if (key == Qt::Key_Backspace) { forwardKey(event); return; }
  if (key == Qt::Key_Up) { list_up(); return; }
  if (key == Qt::Key_Down) { list_down(); return; }
  if (alt || ctrl) { hide(); return; }

  std::string stext = event->text().toStdString();
  if (stext.size() > 1) { hide(); return; }
  uint32_t char_code = stext[0];
  if (char_code == '\n' || char_code == '\r') {
    QListWidgetItem* item = q_list->currentItem();
    if (item != nullptr) {
      Doc* doc = main_window->get_active_document();
      if (doc == nullptr) return;
      doc->handle_complete(item->text().toStdString());
    }
    hide();
    return;
  }
  // Backspace is sometimes this.
  if (char_code == '\b') { forwardKey(event); return; }

  if (char_code >= 32 && char_code <= 122) {
    forwardKey(event); return;
  }

  // Otherwise ignore.
}