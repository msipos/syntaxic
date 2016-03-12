#ifndef SYNTAXIC_QTGUI_DOCS_GUI_HPP
#define SYNTAXIC_QTGUI_DOCS_GUI_HPP

#include "doc.hpp"
#include <QObject>

class Editor;
class MyTabBar;
class MainWindow;
class QScrollArea;
class QWidget;

class DocsGui : public QObject, public DocCollection {
Q_OBJECT

  friend class MainWindow;

  MainWindow* main_window;

  QWidget* root_widget;
    MyTabBar* q_main_tabbar;
    QScrollArea* q_scroll_area;
      Editor* editor;

  void set_current_index(int index);
  void save_scroll_info();
  void restore_scroll_info();

  Hook<Doc*, int> doc_hook;
  void doc_callback(Doc*, int);

public:
  DocsGui(MainWindow* parent);

  void next_doc();
  void prev_doc();

  // Implement DocCollection interface:

  virtual void add_doc(Doc* doc, bool switch_to_doc) override;
  virtual void remove_doc(Doc* doc) override;
  virtual void go_to_doc(Doc* doc) override;

//  void refresh_tab(int tab_index);

private slots:
  void slot_tab_closed(int index);
  void slot_tab_changed(int index);
  void slot_tab_moved(int from, int to);
  void slot_tab_switch_requested(int index);
};

#endif