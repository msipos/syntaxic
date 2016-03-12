#include "qtgui/sar_dialog.hpp"
#include "qtgui/main_window.hpp"

#include <QCheckBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QShowEvent>
#include <QVBoxLayout>

SarDialog::SarDialog(QWidget* parent, MainWindow* mw) : QDialog(parent, Qt::Popup), main_window(mw){
  QFormLayout* formlayout = new QFormLayout;
  formlayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

  {
    {
      QLabel* header = new QLabel("Find and replace...");
      QFont font = header->font();
      font.setBold(true);
      font.setPointSize(12);
      header->setFont(font);
      header->setAlignment(Qt::AlignCenter);
      formlayout->addRow(header);
    }

    q_search_text = new QLineEdit(this);
    connect(q_search_text, &QLineEdit::textEdited, this, &SarDialog::slot_redo_search);
    formlayout->addRow("Find:", q_search_text);

   {
      QHBoxLayout* row_layout = new QHBoxLayout;

      q_case_sensitive = new QCheckBox("Case sensitive", this);
      connect(q_case_sensitive, &QCheckBox::stateChanged, this, &SarDialog::slot_redo_search);
      row_layout->addWidget(q_case_sensitive);
    
      q_whole_word = new QCheckBox("Whole word only", this);
      connect(q_whole_word, &QCheckBox::stateChanged, this, &SarDialog::slot_redo_search);
      row_layout->addWidget(q_whole_word);

      formlayout->addRow(row_layout);
    }

    q_replace_text = new QLineEdit(this);
    formlayout->addRow("Replace:", q_replace_text);

    q_feedback_label = new QLabel(this);
    formlayout->addRow(q_feedback_label);

    {
      QHBoxLayout* button_layout = new QHBoxLayout;

      QPushButton* button_find_next = new QPushButton("Find &next", this);
      connect(button_find_next, &QPushButton::clicked, this, &SarDialog::slot_next);
      button_find_next->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_F));
      button_layout->addWidget(button_find_next);

      QPushButton* button_find_prev = new QPushButton("Find &previous", this);
      connect(button_find_prev, &QPushButton::clicked, this, &SarDialog::slot_prev);
      button_find_prev->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_G));
      button_layout->addWidget(button_find_prev);

      QPushButton* button_replace = new QPushButton("&Replace", this);
      connect(button_replace, &QPushButton::clicked, this, &SarDialog::slot_replace);
      button_replace->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_R));
      button_layout->addWidget(button_replace);

      QPushButton* button_replace_all = new QPushButton("Replace &all", this);
      connect(button_replace_all, &QPushButton::clicked, this, &SarDialog::slot_replace_all);
      button_replace_all->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_R));
      button_layout->addWidget(button_replace_all);

      formlayout->addRow(button_layout);
    }
  }

  setLayout(formlayout);
}

const char* START_TYPING_TEXT = "Type to start searching.";

void SarDialog::execute(const std::string& search_text) {
  q_search_text->setText(QString::fromStdString(search_text));
  show();
}

void SarDialog::showEvent(QShowEvent* event) {
  QDialog::showEvent(event);
  main_window->move_away(this, PL_HCENTER);
  q_feedback_label->setText(START_TYPING_TEXT);
  q_search_text->setFocus();
  q_replace_text->setText("");
  if (q_search_text->text().size() > 0) {
    sar_search(false, false);
  }
  q_whole_word->setChecked(false);
  q_case_sensitive->setChecked(false);

  Doc* doc = main_window->get_active_document();
  if (doc == nullptr) return;
  doc->get_appendage().rich_text.suppress_temporary_highlights = true;
}

void SarDialog::hideEvent(QHideEvent* event) {
  QDialog::hideEvent(event);
  Doc* doc = main_window->get_active_document();
  if (doc == nullptr) return;
  doc->handle_search_update("", get_search_settings(), false, false);
  doc->get_appendage().rich_text.suppress_temporary_highlights = false;
  main_window->menu_actions_enablement();
}

void SarDialog::feedback(DocSearchResults dsr) {
  std::string search_term = q_search_text->text().toStdString();
  if (search_term.empty()) {
    q_feedback_label->setText(START_TYPING_TEXT);
    return;
  }

  if (dsr.num_results == 0) {
    q_feedback_label->setText("No results found.");
    return;
  } else if (dsr.cur_result == dsr.num_results || dsr.cur_result == -1) {
    std::string result = "result";
    if (dsr.num_results > 1) result = "results";
    std::string text = "Found " + std::to_string(dsr.num_results);
    text += " " + result + ".";
    q_feedback_label->setText(QString::fromStdString(text));
  } else {
    std::string text = "Result " + std::to_string(dsr.cur_result + 1);
    text += " of " + std::to_string(dsr.num_results);
    text += ".";
    q_feedback_label->setText(QString::fromStdString(text));
  }
}

SearchSettings SarDialog::get_search_settings() {
  SearchSettings search_settings;
  search_settings.word = q_whole_word->isChecked();
  search_settings.case_insensitive = !q_case_sensitive->isChecked();
  return search_settings;
}

void SarDialog::slot_redo_search() {
  sar_search(false, false);
}

void SarDialog::sar_search(bool search_back, bool move) {
  Doc* doc = main_window->get_active_document();
  if (doc == nullptr) return;

  std::string search_term = q_search_text->text().toStdString();
  DocSearchResults dsr = doc->handle_search_update(search_term, get_search_settings(), search_back, move);
  feedback(dsr);
}

void SarDialog::sar_replace(bool all) {
  Doc* doc = main_window->get_active_document();
  if (doc == nullptr) return;

  std::string search_term = q_search_text->text().toStdString();
  std::string replacement_term = q_replace_text->text().toStdString();

  if (all) {
    int replaced = doc->handle_search_replace_all(search_term, get_search_settings(), replacement_term);
    if (all) {
      std::string feedback = "Replaced ";
      if (replaced == 1) {
        feedback += std::to_string(replaced) + " result.";
      } else {
        feedback += std::to_string(replaced) + " results.";
      }
      main_window->feedback("Replace All", feedback);
      if (replaced > 0) hide();
    }
  } else {
    if (search_term.empty()) {
      q_feedback_label->setText(START_TYPING_TEXT);
      return;
    }
    DocSearchResults dsr = doc->handle_search_replace(search_term, get_search_settings(), replacement_term);
    feedback(dsr);
  }
}

void SarDialog::slot_next() {
  sar_search(false, true);
}

void SarDialog::slot_prev() {
  sar_search(true, true);
}

void SarDialog::slot_replace() {
  sar_replace(false);
}

void SarDialog::slot_replace_all() {
  sar_replace(true);
  // Quit dialog
}