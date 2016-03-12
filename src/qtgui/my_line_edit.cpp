#include "qtgui/my_line_edit.hpp"

#include <QKeyEvent>

void MyLineEdit::keyPressEvent(QKeyEvent *event) {
  if(event->key() == Qt::Key_Up){
    emit key_up();
  } else if(event->key() == Qt::Key_Down){
    emit key_down();
  } else {
    QLineEdit::keyPressEvent(event);
  }
}