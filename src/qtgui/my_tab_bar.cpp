#include "qtgui/my_tab_bar.hpp"

#include <QAction>
#include <QMenu>
#include <QMouseEvent>

MyTabBar::MyTabBar(QWidget* p) : QTabBar(p), context_tab_index(-1) {
  setDocumentMode(true);
  setExpanding(false);
  setFocusPolicy(Qt::NoFocus);
  setTabsClosable(true);
  setMovable(true);
  setContentsMargins(0, 0, 0, 0);

  q_action_context_close_tab = new QAction("&Close tab", this);
  connect(q_action_context_close_tab, &QAction::triggered, this, &MyTabBar::slot_context_close_tab);
  q_action_context_switch_tab = new QAction("&Move to other side", this);
  connect(q_action_context_switch_tab, &QAction::triggered, this, &MyTabBar::slot_context_switch_tab);
}

void MyTabBar::mousePressEvent(QMouseEvent* event) {
  int tab = tabAt(event->pos());

  if (event->button() == Qt::RightButton) {
    event->accept();
    if (tab >= 0) {
      context_tab_index = tab;
      QMenu menu;
      menu.addAction(q_action_context_switch_tab);
      menu.addSeparator();
      menu.addAction(q_action_context_close_tab);
      menu.exec(event->globalPos());
    }
  } else if (event->button() == Qt::MiddleButton) {
    event->accept();
    if (tab >= 0) {
      emit tabCloseRequested(tab);
    }
  } else QTabBar::mousePressEvent(event);
}

void MyTabBar::slot_context_close_tab() {
  if (context_tab_index >= 0) emit tabCloseRequested(context_tab_index);
  context_tab_index = -1;
}

void MyTabBar::slot_context_switch_tab() {
  if (context_tab_index >= 0) emit signal_switch_tab(context_tab_index);
  context_tab_index = -1;
}