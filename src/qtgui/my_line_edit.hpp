#ifndef SYNTAXIC_QTGUI_MY_LINE_EDIT_HPP
#define SYNTAXIC_QTGUI_MY_LINE_EDIT_HPP

#include <QLineEdit>

class MyLineEdit : public QLineEdit {
  Q_OBJECT

public:
  MyLineEdit(QWidget* parent) : QLineEdit(parent) {}

  virtual void keyPressEvent(QKeyEvent *event);

signals:
  void key_up();
  void key_down();
};

#endif