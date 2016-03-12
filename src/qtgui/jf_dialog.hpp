#ifndef SYNTAXIC_QTGUI_JF_DIALOG_HPP
#define SYNTAXIC_QTGUI_JF_DIALOG_HPP

#include "master.hpp"
#include "choices.hpp"
#include <QDialog>
#include <QLineEdit>

class MainWindow;
class MyLineEdit;
class QHideEvent;
class QListWidget;
class QListWidgetItem;
class QShowEvent;

class JfDialog : public QDialog {
  Q_OBJECT

private:
  MainWindow* main_window;
  MyLineEdit* q_file_name;
  QListWidget* q_file_list;

  std::vector<KnownDocument> known_documents;
  ChoiceList choice_list;

  void repopulate_file_list();

public:
  JfDialog(QWidget* parent, MainWindow* mw);

  void show();
  virtual void showEvent(QShowEvent* event);
  virtual void hideEvent(QHideEvent* event);

public slots:
  void slot_text_changed(const QString& new_text);
  void slot_text_return();
  void slot_list_double_clicked(QListWidgetItem*);
  void slot_list_up();
  void slot_list_down();
};

#endif