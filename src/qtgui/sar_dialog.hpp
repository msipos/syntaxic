#ifndef SYNTAXIC_SAR_DIALOG_HPP
#define SYNTAXIC_SAR_DIALOG_HPP

#include "document.hpp"

#include <QDialog>

class MainWindow;
class QCheckBox;
class QHideEvent;
class QKeyEvent;
class QLabel;
class QLineEdit;
class QShowEvent;

class SarDialog : public QDialog {
  Q_OBJECT

private:
  MainWindow* main_window;
  QCheckBox* q_whole_word;
  QCheckBox* q_case_sensitive;
  QLineEdit* q_search_text;
  QLineEdit* q_replace_text;
  QLabel* q_feedback_label;

  void feedback(DocSearchResults dsr);
  SearchSettings get_search_settings();

public:
  SarDialog(QWidget* parent, MainWindow* mw);

  void sar_search(bool search_back, bool move);
  void sar_replace(bool all);

  virtual void showEvent(QShowEvent* event);
  virtual void hideEvent(QHideEvent* event);

  /** Start off the dialog with search_text already populated. */
  void execute(const std::string& search_text);

public slots:
  void slot_next();
  void slot_prev();
  void slot_replace();
  void slot_replace_all();
  void slot_redo_search();
};

#endif