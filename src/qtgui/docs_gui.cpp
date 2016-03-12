#include "master.hpp"
#include "qtgui/docs_gui.hpp"
#include "qtgui/editor.hpp"
#include "qtgui/main_window.hpp"
#include "qtgui/my_tab_bar.hpp"

#include <QVBoxLayout>
#include <QScrollBar>
#include <QWidget>

DocsGui::DocsGui(MainWindow* mw) : main_window(mw) {
  {
    QVBoxLayout* vlayout = new QVBoxLayout;
    root_widget = new QWidget;
    vlayout->setContentsMargins(0, 0, 0, 0);
    vlayout->setSpacing(0);

    // Tabbar
    {
      q_main_tabbar = new MyTabBar(root_widget);
      vlayout->addWidget(q_main_tabbar);
      connect(q_main_tabbar, &QTabBar::currentChanged, this, &DocsGui::slot_tab_changed);
      connect(q_main_tabbar, &QTabBar::tabCloseRequested, this, &DocsGui::slot_tab_closed);
      connect(q_main_tabbar, &QTabBar::tabMoved, this, &DocsGui::slot_tab_moved);
      connect(q_main_tabbar, &MyTabBar::signal_switch_tab, this, &DocsGui::slot_tab_switch_requested);
    }

    // Editor
    {
      editor = new Editor(root_widget, mw);
      q_scroll_area = editor->get_scroll_area();
      vlayout->addWidget(q_scroll_area);
    }

    root_widget->setLayout(vlayout);
  }

  doc_hook = all_docs_hook.add(std::bind(&DocsGui::doc_callback, this, std::placeholders::_1, std::placeholders::_2));
}

void DocsGui::doc_callback(Doc* doc, int flags) {
  for (unsigned int i = 0; i < docs.size(); i++) {
    const Doc* d = docs[i];
    if (doc == d) {
      if (flags & DocEvent::CHANGED_STATE) {
        q_main_tabbar->setTabText(i, QString::fromStdString(d->get_short_title()));
        main_window->refresh_window_title();
      }

      if (flags & DocEvent::CURSOR_MOVED) {
        main_window->refresh_cursor_label();
      }

      return;
    }
  }
}

void DocsGui::save_scroll_info() {
  Doc* doc = get_current_doc();
  if (doc != nullptr) {
    doc->get_appendage().scroll_x = q_scroll_area->horizontalScrollBar()->value();
    doc->get_appendage().scroll_y = doc->get_appendage().cursor_y - q_scroll_area->verticalScrollBar()->value();
  }
}

void DocsGui::restore_scroll_info() {
  Doc* doc = get_current_doc();
  if (doc != nullptr) {
    q_scroll_area->horizontalScrollBar()->setValue(doc->get_appendage().scroll_x);
    q_scroll_area->verticalScrollBar()->setValue(doc->get_appendage().cursor_y - doc->get_appendage().scroll_y);
  }
}

void DocsGui::set_current_index(int index) {
  save_scroll_info();
  current_index = index;
  Doc* doc = get_current_doc();
  main_window->set_current_doc(doc);
  editor->set_document(doc);
  restore_scroll_info();
  if (index >= 0) {
    q_main_tabbar->blockSignals(true);
    q_main_tabbar->setCurrentIndex(index);
    q_main_tabbar->blockSignals(false);
    editor->setFocus(Qt::OtherFocusReason);
  }
}

void DocsGui::next_doc() {
  if (docs.size() == 0) {
    set_current_index(-1);
    return;
  }
  set_current_index((current_index + 1) % docs.size());
}

void DocsGui::prev_doc() {
  if (docs.size() == 0) {
    set_current_index(-1);
    return;
  }
  set_current_index((current_index - 1 + docs.size()) % docs.size());
}

void DocsGui::add_doc(Doc* doc, bool switch_to_doc) {
  docs.push_back(doc);
  doc->set_window(main_window);
  doc->set_collection(this);

  q_main_tabbar->blockSignals(true);
  q_main_tabbar->addTab(QString::fromStdString(doc->get_short_title()));
  q_main_tabbar->setFocusPolicy(Qt::NoFocus);
  q_main_tabbar->blockSignals(false);

  if (docs.size() == 1 || switch_to_doc) set_current_index(docs.size()-1);
}

void DocsGui::remove_doc(Doc* doc) {
  int doc_index = -1;
  for (unsigned int i = 0; i < docs.size(); i++) {
    if (docs[i] == doc) {
      doc_index = i;
      break;
    }
  }
  // This document isn't in this collection:
  if (doc_index < 0) return;

  // The document was found.
  if (doc_index < current_index) {
    // Document being removed is BEFORE the current document.
    q_main_tabbar->blockSignals(true);
    q_main_tabbar->removeTab(doc_index);
    q_main_tabbar->blockSignals(false);
    docs.erase(docs.begin() + doc_index);
    current_index--;
    set_current_index(current_index);
  } else if (doc_index > current_index) {
    // Document being removed is AFTER the current document.
    q_main_tabbar->blockSignals(true);
    q_main_tabbar->removeTab(doc_index);
    q_main_tabbar->blockSignals(false);
    docs.erase(docs.begin() + doc_index);
  } else {
    // Here doc_index = current_index

    if (doc_index == int(docs.size()) - 1) {
      // Document being removed is the last document

      set_current_index(doc_index - 1);
      q_main_tabbar->blockSignals(true);
      q_main_tabbar->removeTab(doc_index);
      q_main_tabbar->blockSignals(false);
      docs.erase(docs.begin() + doc_index);
      // One more time, because main window must be given a chance to delete the pane.
      set_current_index(doc_index - 1);
    } else {
      set_current_index(doc_index+1);
      q_main_tabbar->blockSignals(true);
      q_main_tabbar->removeTab(doc_index);
      q_main_tabbar->blockSignals(false);
      docs.erase(docs.begin() + doc_index);
      current_index--;
      set_current_index(current_index);
    }
  }
}

void DocsGui::go_to_doc(Doc* doc) {
  for (unsigned int i = 0; i < docs.size(); i++) {
    if (docs[i] == doc) {
      set_current_index(i);
      return;
    }
  }
}

void DocsGui::slot_tab_closed(int index) {
  master.set_markovian(MARKOVIAN_NONE);
  Doc* doc = docs.at(index);
  master.close_document(doc);
}

void DocsGui::slot_tab_changed(int index) {
  master.set_markovian(MARKOVIAN_NONE);
  if (index >= 0) set_current_index(index);
}

void DocsGui::slot_tab_moved(int from, int to) {
  master.set_markovian(MARKOVIAN_NONE);
  std::swap(docs[from], docs[to]);
}

void DocsGui::slot_tab_switch_requested(int index) {
  if (index >= 0) set_current_index(index);
  main_window->slot_document_move_to_other_side();
}