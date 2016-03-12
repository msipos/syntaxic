#ifndef SYNTAXIC_QTGUI_MY_TAB_BAR_HPP
#define SYNTAXIC_QTGUI_MY_TAB_BAR_HPP

#include <QTabBar>

class QAction;
class QMouseEvent;

class MyTabBar : public QTabBar {
  Q_OBJECT

  int context_tab_index;

  QAction* q_action_context_close_tab;
  QAction* q_action_context_switch_tab;

public:
  MyTabBar(QWidget* p);

  virtual void mousePressEvent(QMouseEvent* event);

private slots:
  void slot_context_close_tab();
  void slot_context_switch_tab();

signals:
  /** User has chosen the "switch to other side" option in the tab bar menu. */
  void signal_switch_tab(int tab_index);
};

#endif