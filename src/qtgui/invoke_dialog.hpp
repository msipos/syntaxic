#ifndef SYNTAXIC_QTGUI_INVOKE_DIALOG_HPP
#define SYNTAXIC_QTGUI_INVOKE_DIALOG_HPP

#include <QDialog>

class MainWindow;
class MyLineEdit;
class QHideEvent;
class QListWidget;
class QShowEvent;

class InvokeDialog : public QDialog {
  Q_OBJECT

private:
  MainWindow* main_window;
  MyLineEdit* command_edit;
  QListWidget* command_list;

public:
  InvokeDialog(QWidget* parent, MainWindow* mw);

  void show();
  virtual void showEvent(QShowEvent* event);
  virtual void hideEvent(QHideEvent* event);

public slots:
  void slot_text_return();
  void slot_list_up();
  void slot_list_down();
};

#endif