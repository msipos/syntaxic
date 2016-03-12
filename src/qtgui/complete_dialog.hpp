#ifndef SYNTAXIC_QTGUI_COMPLETE_DIALOG_HPP
#define SYNTAXIC_QTGUI_COMPLETE_DIALOG_HPP

#include <QDialog>

class Editor;
class MainWindow;
class QListWidget;

class CompleteDialog : public QDialog {
  Q_OBJECT

  MainWindow* main_window;
  Editor* editor;
  QListWidget* q_list;

  bool repopulate_list(bool complete_prefix);
  void forwardKey(QKeyEvent*);
  void list_up();
  void list_down();

public:
  CompleteDialog(QWidget* parent);

  void show(Editor* editor);
  virtual void showEvent(QShowEvent* event);
  virtual void keyPressEvent(QKeyEvent*);
};

#endif