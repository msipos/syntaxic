#include "lm.hpp"
#include "master.hpp"
#include "core/util.hpp"
#include "qtgui/complete_dialog.hpp"
#include "qtgui/editor.hpp"
#include "qtgui/feedback_box.hpp"
#include "qtgui/jf_dialog.hpp"
#include "qtgui/main_window.hpp"
#include "qtgui/preferences.hpp"
#include "qtgui/register_dialog.hpp"
#include "qtgui/sar_dialog.hpp"
#include "settings.hpp"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QContextMenuEvent>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QDropEvent>
#include <QFileDialog>
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPalette>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QSplitter>
#include <QStyle>
#include <QTabBar>
#include <QTimer>
#include <QVBoxLayout>

class SidebarList : public QListWidget {
private:
  MainWindow* main_window;
  QAction* new_file;
  QAction* new_dir;
  QAction* rename;
  QAction* remove;
  QAction* open;
  QAction* refresh;
  int triggered_item;

public slots:
  void slot_new_file() { main_window->sidebar_list_context_new_file(); }
  void slot_new_dir() { main_window->sidebar_list_context_new_dir(); }
  void slot_rename() { main_window->sidebar_list_context_rename(triggered_item); }
  void slot_remove() { main_window->sidebar_list_context_delete(triggered_item); }
  void slot_open() { main_window->sidebar_list_context_open(triggered_item); }
  void slot_refresh() { main_window->sidebar_list_context_refresh(); }

public:
  inline SidebarList(QWidget* parent, MainWindow* mw) : QListWidget(parent), main_window(mw),
      triggered_item(-1) {
    new_file = new QAction("New &file...", this);
    connect(new_file, &QAction::triggered, this, &SidebarList::slot_new_file);

    new_dir = new QAction("New &dir...", this);
    connect(new_dir, &QAction::triggered, this, &SidebarList::slot_new_dir);

    rename = new QAction("Rename...", this);
    connect(rename, &QAction::triggered, this, &SidebarList::slot_rename);

    remove = new QAction("Delete...", this);
    connect(remove, &QAction::triggered, this, &SidebarList::slot_remove);

    open = new QAction("Open", this);
    connect(open, &QAction::triggered, this, &SidebarList::slot_open);

    refresh = new QAction("Refresh", this);
    connect(refresh, &QAction::triggered, this, &SidebarList::slot_refresh);
  }

  virtual void contextMenuEvent(QContextMenuEvent* event);
};

void SidebarList::contextMenuEvent(QContextMenuEvent* event) {
  QPoint pt = event->pos();
  QListWidgetItem* item = itemAt(pt);

  QMenu menu;
  menu.addAction(new_file);
  menu.addAction(new_dir);

  if (item != nullptr) {
    triggered_item = item->type();
    menu.addSeparator();
    menu.addAction(rename);
    menu.addAction(remove);
    menu.addAction(open);
  }

  menu.addSeparator();
  menu.addAction(refresh);

  menu.exec(event->globalPos());

  triggered_item = -1;
}

////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<UIWindow> make_window() {
  MainWindow* mw = new MainWindow(nullptr);

  int width = mw->frameGeometry().width();
  int height = mw->frameGeometry().height();

  //QDesktopWidget* wid = QApplication::desktop();
  //int screenWidth = wid->screen()->width();
  //int screenHeight = wid->screen()->height();
  //mw->setGeometry((screenWidth/2)-(width/2), (screenHeight/2)-(height/2), width, height);
  mw->show();
  return std::unique_ptr<UIWindow>(mw);
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), active_document(-1),
    window_cursor_x(0), window_cursor_y(0), lm_valid(false), lm_check_counter(10) {

  // Don't have QT delete our Window on close.
  setAttribute(Qt::WA_DeleteOnClose, false);
  //setAttribute(Qt::WA_QuitOnClose, false);

  // Splitter
  {
    q_splitter = new QSplitter;

    // Sidebar
    {
      QVBoxLayout* vlayout = new QVBoxLayout;
      QWidget* widget = new QWidget;
      vlayout->setContentsMargins(1, 1, 1, 1);

      // Path at the top
      {
        QHBoxLayout* hlayout = new QHBoxLayout;
        hlayout->setContentsMargins(8, 4, 8, 0);

        QLabel* image = new QLabel(widget);
        QStyle* app_style = QApplication::style();
        QIcon dir_icon = app_style->standardIcon(QStyle::SP_DirIcon);
        image->setPixmap(dir_icon.pixmap(16, 16));
        hlayout->addWidget(image, 0);

        q_sidebar_path_label = new QLabel("build", widget);
        q_sidebar_path_label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        q_sidebar_path_label->setTextFormat(Qt::PlainText);
        hlayout->addWidget(q_sidebar_path_label, 1);

        vlayout->addLayout(hlayout);
      }

      q_sidebar_list = new SidebarList(widget, this);
      q_sidebar_list->setFocusPolicy(Qt::NoFocus);
      q_sidebar_list->setIconSize(QSize(16, 16));
      connect(q_sidebar_list, &QListWidget::itemDoubleClicked, this, &MainWindow::slot_sidebar_dblclick);
      vlayout->addWidget(q_sidebar_list);

      q_sidebar_position_label = new QLabel("", widget);
      q_sidebar_position_label->setContentsMargins(4, 0, 4, 4);
      vlayout->addWidget(q_sidebar_position_label);

      widget->setLayout(vlayout);
      q_splitter->addWidget(widget);
      q_splitter->setStretchFactor(0, 0);
    }

    // Main vlayout
    {
      QVBoxLayout* vlayout = new QVBoxLayout;
      QWidget* widget = new QWidget;
      vlayout->setContentsMargins(0, 0, 0, 0);
      vlayout->setSpacing(0);

      // Tabbar
      {
        q_main_tabbar = new QTabBar(widget);
        q_main_tabbar->setDocumentMode(true);
        q_main_tabbar->setExpanding(false);
        q_main_tabbar->setFocusPolicy(Qt::NoFocus);
        q_main_tabbar->setTabsClosable(true);
        q_main_tabbar->setContentsMargins(0, 0, 0, 0);
        vlayout->addWidget(q_main_tabbar);
        connect(q_main_tabbar, &QTabBar::currentChanged, this, &MainWindow::slot_tab_changed);
        connect(q_main_tabbar, &QTabBar::tabCloseRequested, this, &MainWindow::slot_tab_closed);
      }

      // Editor
      {
        q_scroll_area = new QScrollArea(widget);
        q_scroll_area->setFocusPolicy(Qt::NoFocus);
        q_scroll_area->setWidgetResizable(true);
        q_scroll_area->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
        editor = nullptr;
        editor = new Editor(widget, this);
        q_scroll_area->setWidget(editor);
        vlayout->addWidget(q_scroll_area);
      }

      widget->setLayout(vlayout);
      q_splitter->addWidget(widget);
      q_splitter->setStretchFactor(1, 5);

      {
        QList<int> sizes = {150, 400};
        q_splitter->setSizes(sizes);
      }
    }
  }

  setCentralWidget(q_splitter);
  setMinimumSize(400, 400);

  // Menus
  {
    static QMenuBar* q_menu_bar = nullptr;
    if (q_menu_bar == nullptr) {
      q_menu_bar = new QMenuBar(nullptr);
    }
    setMenuBar(q_menu_bar);

    // File
    q_menu_file = q_menu_bar->addMenu("&File");

    q_action_new = new QAction("&New", this);
    q_action_new->setShortcuts(QKeySequence::New);
    q_action_new->setStatusTip("Create a new file");
    connect(q_action_new, &QAction::triggered, this, &MainWindow::slot_new);
    q_menu_file->addAction(q_action_new);

    q_action_open = new QAction("&Open...", this);
    q_action_open->setShortcuts(QKeySequence::Open);
    q_action_open->setStatusTip("Open a file");
    connect(q_action_open, &QAction::triggered, this, &MainWindow::slot_open);
    q_menu_file->addAction(q_action_open);

    q_menu_file->addSeparator();

    q_action_save = new QAction("&Save", this);
    q_action_save->setShortcuts(QKeySequence::Save);
    q_action_save->setStatusTip("Save file");
    connect(q_action_save, &QAction::triggered, this, &MainWindow::slot_save);
    q_menu_file->addAction(q_action_save);

    q_action_save_as = new QAction("Save &as...", this);
    q_action_save_as->setShortcuts(QKeySequence::SaveAs);
    q_action_save_as->setStatusTip("Save file with new name");
    connect(q_action_save_as, &QAction::triggered, this, &MainWindow::slot_save_as);
    q_menu_file->addAction(q_action_save_as);

    #ifndef CMAKE_MACOSX
    q_menu_file->addSeparator();
    #endif

    q_action_quit = new QAction("&Quit", this);
    q_action_quit->setShortcuts(QKeySequence::Quit);
    q_action_quit->setStatusTip("Quit");
    connect(q_action_quit, &QAction::triggered, this, &MainWindow::close);
    q_menu_file->addAction(q_action_quit);

    // Edit

    q_menu_edit = menuBar()->addMenu("&Edit");

    q_action_edit_undo = new QAction("&Undo", this);
    q_action_edit_undo->setShortcuts(QKeySequence::Undo);
    q_action_edit_undo->setStatusTip("Undo");
    connect(q_action_edit_undo, &QAction::triggered, this, &MainWindow::slot_undo);
    q_menu_edit->addAction(q_action_edit_undo);

    q_menu_edit->addSeparator();

    q_action_edit_cut = new QAction("C&ut", this);
    q_action_edit_cut->setShortcuts(QKeySequence::Cut);
    q_action_edit_cut->setStatusTip("Cut");
    connect(q_action_edit_cut, &QAction::triggered, this, &MainWindow::slot_cut);
    q_menu_edit->addAction(q_action_edit_cut);

    q_action_edit_copy = new QAction("&Copy", this);
    q_action_edit_copy->setShortcuts(QKeySequence::Copy);
    q_action_edit_copy->setStatusTip("Copy");
    connect(q_action_edit_copy, &QAction::triggered, this, &MainWindow::slot_copy);
    q_menu_edit->addAction(q_action_edit_copy);

    q_action_edit_paste = new QAction("&Paste", this);
    q_action_edit_paste->setShortcuts(QKeySequence::Paste);
    q_action_edit_paste->setStatusTip("Paste");
    connect(q_action_edit_paste, &QAction::triggered, this, &MainWindow::slot_paste);
    q_menu_edit->addAction(q_action_edit_paste);

    q_menu_edit->addSeparator();

    q_action_edit_comment = new QAction("Co&mment out", this);
    q_action_edit_comment->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_M));
    q_action_edit_comment->setStatusTip("Comment out lines");
    connect(q_action_edit_comment, &QAction::triggered, this, &MainWindow::slot_comment);
    q_menu_edit->addAction(q_action_edit_comment);

    q_action_edit_uncomment = new QAction("Uncomme&nt", this);
    q_action_edit_uncomment->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_M));
    q_action_edit_uncomment->setStatusTip("Uncomment lines");
    connect(q_action_edit_uncomment, &QAction::triggered, this, &MainWindow::slot_uncomment);
    q_menu_edit->addAction(q_action_edit_uncomment);

    q_menu_edit->addSeparator();

    q_action_edit_complete = new QAction("Comp&lete", this);
    #ifdef CMAKE_MACOSX
    q_action_edit_complete->setShortcut(QKeySequence(Qt::META + Qt::Key_Space));
    #else
    q_action_edit_complete->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Space));
    #endif
    q_action_edit_complete->setStatusTip("Provide completion candidates");
    connect(q_action_edit_complete, &QAction::triggered, this, &MainWindow::slot_complete);
    q_menu_edit->addAction(q_action_edit_complete);

    q_menu_edit->addSeparator();

    q_action_edit_find_replace = new QAction("&Find/Replace...", this);
    q_action_edit_find_replace->setShortcuts(QKeySequence::Find);
    q_action_edit_find_replace->setStatusTip("Find and Replace");
    connect(q_action_edit_find_replace, &QAction::triggered, this, &MainWindow::slot_find_replace);
    q_menu_edit->addAction(q_action_edit_find_replace);

    #ifndef CMAKE_MACOSX
    q_menu_edit->addSeparator();
    #endif

    q_action_edit_preferences = new QAction("&Preferences...", this);
    q_action_edit_preferences->setShortcuts(QKeySequence::Preferences);
    q_action_edit_preferences->setStatusTip("Change preferences");
    connect(q_action_edit_preferences, &QAction::triggered, this, &MainWindow::slot_preferences);
    q_menu_edit->addAction(q_action_edit_preferences);

    // Documents

    q_menu_document = menuBar()->addMenu("&Document");

    q_action_document_close = new QAction("&Close document", this);
    q_action_document_close->setShortcuts(QKeySequence::Close);
    q_action_document_close->setStatusTip("Close document");
    connect(q_action_document_close, &QAction::triggered, this, &MainWindow::slot_document_close);
    q_menu_document->addAction(q_action_document_close);

    q_menu_document->addSeparator();

    q_action_document_next = new QAction("&Next document", this);
    #ifndef CMAKE_MACOSX
    q_action_document_next->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_PageDown));
    #else
    q_action_document_next->setShortcut(QKeySequence(Qt::META + Qt::Key_Tab));
    #endif
    q_action_document_next->setStatusTip("Next document");
    connect(q_action_document_next, &QAction::triggered, this, &MainWindow::slot_document_next);
    q_menu_document->addAction(q_action_document_next);

    q_action_document_prev = new QAction("&Previous document", this);
    #ifndef CMAKE_MACOSX
    q_action_document_prev->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_PageUp));
    #else
    q_action_document_prev->setShortcut(QKeySequence(Qt::META + Qt::SHIFT + Qt::Key_Tab));
    #endif
    q_action_document_prev->setStatusTip("Previous document");
    connect(q_action_document_prev, &QAction::triggered, this, &MainWindow::slot_document_prev);
    q_menu_document->addAction(q_action_document_prev);

    q_menu_document->addSeparator();

    // Document type

    q_menu_document_type = q_menu_document->addMenu("Document type");
    q_action_document_type = new QActionGroup(this);
    {
      QAction* action = new QAction("Text", q_action_document_type);
      action->setCheckable(true);
      action->setChecked(true);
      q_menu_document_type->addAction(action);
    }
    for (const std::string& file_type: master.stat_lang.available_file_types) {
      QAction* action = new QAction(QString::fromStdString(file_type), q_action_document_type);
      action->setCheckable(true);
      q_menu_document_type->addAction(action);
    }
    connect(q_action_document_type, &QActionGroup::triggered, this, &MainWindow::slot_document_type);

    // back to Document menu

    q_menu_document->addSeparator();

    q_action_document_set_bookmark = new QAction("Set &bookmark", this);
    q_action_document_set_bookmark->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_B));
    q_action_document_set_bookmark->setStatusTip("Set a bookmark");
    connect(q_action_document_set_bookmark, &QAction::triggered,
        this, &MainWindow::slot_document_set_bookmark);
    q_menu_document->addAction(q_action_document_set_bookmark);

    q_action_document_jump_to_bookmark = new QAction("&Jump to bookmark", this);
    q_action_document_jump_to_bookmark->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_B));
    q_action_document_jump_to_bookmark->setStatusTip("Jump to bookmark");
    connect(q_action_document_jump_to_bookmark, &QAction::triggered,
        this, &MainWindow::slot_document_jump_to_bookmark);
    q_menu_document->addAction(q_action_document_jump_to_bookmark);

    q_menu_document->addSeparator();

    q_action_document_jump_to_file = new QAction("Jump to &file...", this);
    q_action_document_jump_to_file->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_J));
    q_action_document_jump_to_file->setStatusTip("Jump to file");
    connect(q_action_document_jump_to_file, &QAction::triggered,
        this, &MainWindow::slot_document_jump_to_file);
    q_menu_document->addAction(q_action_document_jump_to_file);

    // Help

    q_menu_help = menuBar()->addMenu("&Help");

    q_action_help_about = new QAction("&About", this);
    q_action_help_about->setStatusTip("Help about");
    connect(q_action_help_about, &QAction::triggered, this, &MainWindow::slot_help_about);
    q_menu_help->addAction(q_action_help_about);

    q_action_help_enter_license = new QAction("&License...", this);
    q_action_help_enter_license->setStatusTip("Enter license");
    connect(q_action_help_enter_license, &QAction::triggered, this,
        &MainWindow::slot_help_enter_license);
    q_menu_help->addAction(q_action_help_enter_license);

    q_action_help_online = new QAction("&Online help...", this);
    q_action_help_online->setStatusTip("Online help");
    connect(q_action_help_online, &QAction::triggered, this, &MainWindow::slot_help_online);
    q_menu_help->addAction(q_action_help_online);
  }

  // Pop-uppy things:
  complete_dialog = new CompleteDialog(this, editor);
  sar_dialog = new SarDialog(this, this);
  feedback_box = new FeedbackBox(this);
  jf_dialog = new JfDialog(this, this);
  preferences_dialog = new PreferencesDialog(this);
  register_dialog = new RegisterDialog(this);

  // Document hook
  document_hook = all_documents_hook.add(
      std::bind(&MainWindow::document_callback, this, std::placeholders::_1,
      std::placeholders::_2));

  // Timer
  q_timer25 = new QTimer(this);
  connect(q_timer25, &QTimer::timeout, this, &MainWindow::slot_timer);
  q_timer25->start(25);

  // Icons
  app_icon = QIcon(":resources/icon.png");
  setWindowIcon(app_icon);
  QStyle* app_style = QApplication::style();
  #ifdef CMAKE_WINDOWS
  file_icon = QIcon(":resources/file.png");
  #else
  file_icon = app_style->standardIcon(QStyle::SP_FileIcon);
  #endif
  dir_icon = app_style->standardIcon(QStyle::SP_DirIcon);

  // Accept drag and drop.
  setAcceptDrops(true);

  populate_sidebar_list();
  refresh_window_title();
  refresh_cursor_label();

  read_settings();
  lm_check(false);
}

// Sidebar context menu feedback ///////////////////////////////////////////////

void MainWindow::sidebar_list_context_new_file() {
  bool ok;
  QString text = QInputDialog::getText(this, "Create new file",
      "Enter the name of the new file:", QLineEdit::Normal, "", &ok);
  if (!ok) return;
  file_browser.new_file(text.toStdString(), this);
  populate_sidebar_list();
}
void MainWindow::sidebar_list_context_new_dir() {
  bool ok;
  QString text = QInputDialog::getText(this, "Create new directory",
      "Enter the name of the new directory:", QLineEdit::Normal, "", &ok);
  if (!ok) return;
  file_browser.new_dir(text.toStdString(), this);
  populate_sidebar_list();
}
void MainWindow::sidebar_list_context_open(int i) {
  file_browser.activate(i, this);
}
void MainWindow::sidebar_list_context_rename(int i) {
  bool ok;
  QString text = QInputDialog::getText(this, "Rename",
      "Enter new name:", QLineEdit::Normal, "", &ok);
  if (!ok) return;
  file_browser.rename(i, text.toStdString(), this);
  populate_sidebar_list();
}
void MainWindow::sidebar_list_context_delete(int i) {
  file_browser.remove(i, this);
  populate_sidebar_list();
}

void MainWindow::sidebar_list_context_refresh() {
  file_browser.refresh();
  populate_sidebar_list();
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::editor_cursor_moved(int x, int y, int cursor_margin) {
  // Editor calls back before everything is instantiated.
  if (editor == nullptr) return;

  q_scroll_area->ensureVisible(x, y, 1, cursor_margin);

  // Map to window coordinates.
  QPoint pt = editor->mapToGlobal(QPoint(x, y));
  pt = mapFromGlobal(pt);
  window_cursor_x = pt.x();
  window_cursor_y = pt.y();

  if (sar_dialog->isVisible()) move_away(sar_dialog, PL_HCENTER);
  if (feedback_box->isVisible()) move_away(feedback_box, PL_BOTTOM_RIGHT);
  if (complete_dialog->isVisible()) move_away(complete_dialog, PL_CURSOR);
}

void MainWindow::move_away(QWidget* widget, PreferredLocation pl) {
  const int window_width = width(), window_height = height();
  const int widget_width = widget->size().width(), widget_height = widget->size().height();

  QPoint target;
  switch(pl) {
    case PL_HCENTER:
      target.setX((window_width - widget_width) / 2);
      if (window_cursor_y < window_height / 2) {
        target.setY(window_height - widget_height - 50);
      } else {
        target.setY(125);
      }
      target = mapToGlobal(target);
      break;
    case PL_BOTTOM_RIGHT:
      // In window coordinates:
      target.setX(window_width - 50 - widget_width);
      target.setY(window_height - 50 - widget_height);
      // Hence convert to global:
      target = mapToGlobal(target);
      break;
    case PL_CURSOR:
      target.setX(window_cursor_x);
      target.setY(window_cursor_y);
      target = mapToGlobal(target);
      break;
  }

  // For windows, the position is with respect to the screen.
  if (widget->isWindow()) {
    widget->move(target);
  } else {
    QWidget* parent = dynamic_cast<QWidget*> (widget->parent());
    QPoint pt = parent->mapFromGlobal(target);
    widget->move(pt);
  }
}

void MainWindow::feedback(const std::string& title, const std::string& text) {
  feedback_box->display(title, text, 3000);
  move_away(feedback_box, PL_BOTTOM_RIGHT);
}

void MainWindow::editor_page(bool up) {
  QScrollBar* bar = q_scroll_area->verticalScrollBar();
  if (up) {
    bar->setValue(bar->value() - bar->pageStep());
  } else {
    bar->setValue(bar->value() + bar->pageStep());
  }
}

void MainWindow::editor_redrawn() {
  if (feedback_box->isVisible()) {
    feedback_box->update();
  }
}

void MainWindow::closeEvent(QCloseEvent* event) {
  if (!master.quit()) {
    event->ignore();
  } else {
    write_settings();
    event->accept();
  }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
  if (event->mimeData()->hasUrls()) event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event) {
  if (event->mimeData()->hasUrls()) {
    for (QUrl url: event->mimeData()->urls()) {
      if (url.isLocalFile()) {
        QString path = url.toLocalFile();
        master.open_document(path.toStdString().c_str(), this);
      }
    }
  }
}

void MainWindow::refresh_cursor_label() {
  Document* doc = get_active_document();
  if (doc == nullptr) {
    q_sidebar_position_label->hide();
  } else {
    q_sidebar_position_label->show();
    std::string text("Row: ");
    CursorLocation cl = doc->get_text_view()->get_cursor();
    text += std::to_string(cl.row + 1);
    text += ", Col: ";
    text += std::to_string(cl.col);
    q_sidebar_position_label->setText(QString::fromStdString(text));
  }
}

void MainWindow::refresh_tab(int tab_index) {
  if (tab_index < 0 || tab_index >= q_main_tabbar->count() || tab_index >= (int) documents.size()) {
    return;
  }

  Document* d = documents[tab_index];
  q_main_tabbar->setTabText(tab_index, QString::fromStdString(d->get_title_text()));
}

void MainWindow::refresh_window_title() {
  std::string title;
  Document* document = get_active_document();
  if (document) {
    title += document->get_long_title_text();
    title += " - Syntaxic";
  } else {
    title = "Syntaxic";
  }
  if (!lm_valid) {
    title += " (Evaluation copy)";
  }
  setWindowTitle(QString::fromStdString(title));
}

void MainWindow::populate_sidebar_list() {
  q_sidebar_list->clear();
  for (unsigned int i = 0; i < file_browser.size(); i++) {
    // Set item->type = i.
    QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(file_browser.get_text(i)),
        nullptr, i);
    int type = file_browser.get_type(i);
    switch(type) {
      case FB_FILE:
        item->setIcon(file_icon);
        break;
      case FB_DIR:
      case FB_PARENT:
        item->setIcon(dir_icon);
        break;
    }
    q_sidebar_list->insertItem(i, item);
  }

  q_sidebar_path_label->setText(QString::fromStdString(file_browser.get_current_dir_name()));
}

void MainWindow::write_settings() {
  QSettings settings;

  settings.beginGroup("MainWindow");
  settings.setValue("size", size());
  settings.setValue("pos", pos());
  settings.endGroup();
}

void MainWindow::read_settings() {
  QSettings settings;

  settings.beginGroup("MainWindow");
  resize(settings.value("size", QSize(400, 400)).toSize());
  move(settings.value("pos", QPoint(200, 200)).toPoint());
  settings.endGroup();
}

void MainWindow::lm_check(bool show_message) {
  lm_check_counter++;

  if (lm_check_counter < 10) return;
  lm_check_counter = 0;

  Settings settings;
  std::string email = settings.get_lm_email();
  std::string skey = settings.get_lm_key();
  if (email.empty()) {
    lm_valid = false;
    if (!show_message) return;
    QMessageBox::information(this, "Thank you for evaluating Syntaxic",
        "Thank you for evaluating Syntaxic!  If you like it, please buy a license to support "
        "development.");
    return;
  }

  bool pass = false;
  if (well_formed(skey)) {
    LMKey lmkey = string_to_key(skey);
    pass = check(lmkey, email);
  }

  if (!pass) {
    lm_valid = false;
    QMessageBox::critical(this, "Thank you for evaluating Syntaxic",
        "Thank you for evaluating Syntaxic!  Unfortunately, your license doesn't pass validation. "
        "Please enter a correct license or contact support@kpartite.com for help.");
    slot_help_enter_license();
    return;
  }
  lm_valid = true;
}

void MainWindow::menu_actions_enablement() {
  bool d = (get_active_document() != nullptr);
  q_action_save->setEnabled(d);
  q_action_save_as->setEnabled(d);
  q_action_edit_undo->setEnabled(d);
  q_action_edit_cut->setEnabled(d);
  q_action_edit_copy->setEnabled(d);
  q_action_edit_paste->setEnabled(d);
  q_action_edit_comment->setEnabled(d);
  q_action_edit_uncomment->setEnabled(d);
  q_action_edit_complete->setEnabled(d);
  q_action_edit_find_replace->setEnabled(d);
  q_action_document_close->setEnabled(d);
  q_menu_document_type->setEnabled(d);
  q_action_document_set_bookmark->setEnabled(d);
}

MainWindow::~MainWindow() {
}

/////////////////////////////////////////////////////////////////////// QT Slots

void MainWindow::slot_tab_closed(int index) {
  master.set_markovian(MARKOVIAN_NONE);
  Document* document = documents.at(index);
  master.close_document(document);
}

void MainWindow::slot_tab_changed(int index) {
  master.set_markovian(MARKOVIAN_NONE);
  if (index >= 0) set_active_document(index);
}

void MainWindow::slot_new() {
  master.set_markovian(MARKOVIAN_NONE);
  master.new_document(this);
}

void MainWindow::slot_open() {
  master.set_markovian(MARKOVIAN_NONE);
  QString fileName = QFileDialog::getOpenFileName(this, "Open File", ".", "All Files (*)");
  std::string str = fileName.toStdString();
  if (str.empty()) return;
  master.open_document(str.c_str(), this);
  lm_check();
}

void MainWindow::slot_save() {
  master.set_markovian(MARKOVIAN_NONE);
  Document* document = get_active_document();
  if (document != nullptr) master.save_document(document);
  lm_check();
}

void MainWindow::slot_save_as() {
  master.set_markovian(MARKOVIAN_NONE);
  Document* document = get_active_document();
  if (document != nullptr) master.save_document_as(document);
  lm_check();
}

void MainWindow::slot_timer() {
  double ts = get_timestamp();
  editor->timer(ts);
}

void MainWindow::slot_undo(){
  master.set_markovian(MARKOVIAN_NONE);
  Document* document = get_active_document();
  if (document != nullptr) document->handle_undo();
}

void MainWindow::slot_copy(){
  master.set_markovian(MARKOVIAN_NONE);
  Document* document = get_active_document();
  if (document != nullptr) {
    QClipboard* clipboard = QGuiApplication::clipboard();
    std::string txt = document->handle_copy();
    if (!txt.empty()) clipboard->setText(QString::fromStdString(txt));
  }
}

void MainWindow::slot_cut(){
  master.set_markovian(MARKOVIAN_NONE);
  Document* document = get_active_document();
  if (document != nullptr) {
    QClipboard* clipboard = QGuiApplication::clipboard();
    std::string txt = document->handle_cut();
    if (!txt.empty()) clipboard->setText(QString::fromStdString(txt));
  }
}

void MainWindow::slot_paste(){
  master.set_markovian(MARKOVIAN_NONE);
  Document* document = get_active_document();
  if (document != nullptr) {
    QClipboard* clipboard = QGuiApplication::clipboard();
    document->handle_paste(clipboard->text().toStdString());
  }
  lm_check();
}

void MainWindow::slot_comment() {
  Document* document = get_active_document();
  if (document != nullptr) document->handle_comment();
}

void MainWindow::slot_uncomment() {
  Document* document = get_active_document();
  if (document != nullptr) document->handle_uncomment();
}

void MainWindow::slot_complete() {
  Document* document = get_active_document();
  if (document == nullptr) return;
  complete_dialog->show();
  move_away(complete_dialog, PL_CURSOR);
}

void MainWindow::slot_find_replace(){
  master.set_markovian(MARKOVIAN_NONE);
  Document* document = get_active_document();
  if (document != nullptr) {
    // Disable on MacOSX because it conflicts with Command+F in the dialog.
    q_action_edit_find_replace->setEnabled(false);
    sar_dialog->show();
  }
}

void MainWindow::slot_preferences() {
  preferences_dialog->exec();
  editor->reload_settings();
  master.reload_settings();
}

void MainWindow::slot_document_close(){
  master.set_markovian(MARKOVIAN_NONE);
  Document* document = get_active_document();
  if (document != nullptr) master.close_document(document);
}

void MainWindow::slot_document_next(){
  master.set_markovian(MARKOVIAN_NONE);
  next_document();
}

void MainWindow::slot_document_prev(){
  master.set_markovian(MARKOVIAN_NONE);
  prev_document();
}

void MainWindow::slot_document_type(QAction* action) {
  Document* document = get_active_document();
  if (document != nullptr) {
    std::string new_type = action->text().toStdString();
    document->handle_change_type(new_type);
  }
  editor->update();
}

void MainWindow::slot_document_set_bookmark(){
  master.set_markovian(MARKOVIAN_NONE);
  Document* document = get_active_document();
  if (document != nullptr) {
    std::string path = document->get_text_file()->get_absolute_path();
    if (path.empty()) {
      // TODO: Not ideal.
      feedback("Can't set bookmark", "Cannot set bookmark in a file that is unsaved!");
      return;
    }
    CursorLocation cl = document->get_text_view()->get_cursor();
    master.add_bookmark(path, cl.row, cl.col);
  }
}

void MainWindow::slot_document_jump_to_bookmark(){
  master.set_markovian(MARKOVIAN_BOOKMARK);
  master.jump_to_bookmark();
}

void MainWindow::slot_document_jump_to_file(){
  jf_dialog->show();
}

void MainWindow::slot_help_about(){ master.open_about(); }

void MainWindow::slot_help_enter_license() {
  register_dialog->exec();
}
void MainWindow::slot_help_online() {
  QDesktopServices::openUrl(QUrl("http://www.syntaxiceditor.com/help"));
}

void MainWindow::slot_sidebar_dblclick(QListWidgetItem* item) {
  const int rv = file_browser.activate(item->type(), this);

  if (rv == FB_DIR || rv == FB_PARENT) {
    populate_sidebar_list();
  }
}

/////////////////////////////////////////////////////////////////////// Implement UIWindow interface

Document* MainWindow::get_active_document() {
  if (active_document < 0) return nullptr;
  return documents.at(active_document);
}

void MainWindow::save_scroll_info() {
  Document* doc = get_active_document();
  if (doc != nullptr) {
    doc->appendage().scroll_x = q_scroll_area->horizontalScrollBar()->value();
    doc->appendage().scroll_y = q_scroll_area->verticalScrollBar()->value();
  }
}

void MainWindow::restore_scroll_info() {
  Document* doc = get_active_document();
  if (doc != nullptr) {
    q_scroll_area->horizontalScrollBar()->setValue(doc->appendage().scroll_x);
    q_scroll_area->verticalScrollBar()->setValue(doc->appendage().scroll_y);
  }
}

void MainWindow::restore_document_type() {
  Document* doc = get_active_document();
  if (doc != nullptr) {
    QList<QAction*> actions = q_action_document_type->actions();
    for (QAction* action: actions) {
      if (action->text().toStdString() == doc->appendage().file_type) {
        action->setChecked(true);
        return;
      }
    }
  }
}

void MainWindow::set_active_document(int index) {
  save_scroll_info();

  active_document = index;
  if (active_document >= 0) {
    Document* doc = get_active_document();
    editor->set_document(documents[index]);
    q_main_tabbar->blockSignals(true);
    q_main_tabbar->setCurrentIndex(index);
    q_main_tabbar->blockSignals(false);
    restore_scroll_info();
    restore_document_type();
  } else {
    editor->set_document(nullptr);
  }
  refresh_window_title();
  refresh_cursor_label();
  menu_actions_enablement();
}

void MainWindow::remove_document(Document* doc) {
  int doc_index = -1;
  for (unsigned int i = 0; i < documents.size(); i++) {
    if (documents[i] == doc) {
      //documents.erase(documents.begin() + i);
      doc_index = i;
      break;
    }
  }
  // This document doesn't belong:
  if (doc_index < 0) return;

  // The document was found.
  int new_index = doc_index;
  if (new_index >= (int(documents.size()) - 1)) {
    new_index = documents.size() - 2;
  }
  save_scroll_info();
  documents.erase(documents.begin() + doc_index);
  q_main_tabbar->blockSignals(true);
  q_main_tabbar->removeTab(doc_index);
  if (new_index >= 0) q_main_tabbar->setCurrentIndex(new_index);
  q_main_tabbar->blockSignals(false);
  active_document = -1;
  set_active_document(new_index);
}

void MainWindow::document_callback(Document* doc, int flag) {
  for (unsigned int i = 0; i < documents.size(); i++) {
    if (documents[i] == doc) {
      if (flag & DOC_CHANGE_STATE) {
        refresh_tab(i);
        refresh_window_title();
      }
      if (active_document == int(i) && (flag & DOC_MOVE)) {
        refresh_cursor_label();
      }
      return;
    }
  }
}

void MainWindow::add_document(Document* doc) {
  doc->set_window(this);
  documents.push_back(doc);
  q_main_tabbar->blockSignals(true);
  q_main_tabbar->addTab(QString::fromStdString(doc->get_title_text()));
  q_main_tabbar->setFocusPolicy(Qt::NoFocus);
  q_main_tabbar->blockSignals(false);
  set_active_document(documents.size()-1);
}

void MainWindow::next_document() {
  if (documents.size() == 0) {
    set_active_document(-1);
    return;
  }
  set_active_document((active_document + 1) % documents.size());
}

void MainWindow::prev_document() {
  if (documents.size() == 0) {
    set_active_document(-1);
    return;
  }
  set_active_document((active_document - 1 + documents.size()) % documents.size());
}

void MainWindow::go_to_document(Document* doc) {
  for (unsigned int i = 0; i < documents.size(); i++) {
    if (documents[i] == doc) {
      set_active_document(i);
      return;
    }
  }
}

int MainWindow::get_page_size() {
  QScrollBar* bar = q_scroll_area->verticalScrollBar();
  return bar->pageStep() / editor->line_height();
}

int MainWindow::get_user_input(const std::string& title, const std::string& text, int type) {
  QMessageBox msgBox(this);
  #ifdef CMAKE_MACOSX
  msgBox.setWindowFlags(Qt::Sheet);
  msgBox.setWindowModality(Qt::WindowModal);
  #endif
  msgBox.setWindowTitle(QString::fromStdString(title));
  msgBox.setText(QString::fromStdString(text));
  QMessageBox::StandardButtons flags = 0;
  if (type & UI_YES) flags |= QMessageBox::Yes;
  if (type & UI_NO) flags |= QMessageBox::No;
  if (type & UI_CANCEL) flags |= QMessageBox::Cancel;
  if (type & UI_OK) flags |= QMessageBox::Ok;
  msgBox.setStandardButtons(flags);
  if (type & UI_WARNING) msgBox.setIcon(QMessageBox::Warning);
  int ret = msgBox.exec();
  switch(ret) {
    case QMessageBox::Yes: return UI_YES;
    case QMessageBox::No: return UI_NO;
    case QMessageBox::Cancel: return UI_CANCEL;
    case QMessageBox::Ok: return UI_OK;
  }
  return UI_CANCEL;
}

int MainWindow::get_user_file(std::string& output_file, const std::string& text,
    const std::string& wildcard, int type) {
  if (type & UF_OPEN) {
    QString result = QFileDialog::getOpenFileName(this, QString::fromStdString(text),
        ".", QString::fromStdString(wildcard), 0, QFileDialog::DontUseCustomDirectoryIcons
        | QFileDialog::HideNameFilterDetails);
    output_file = result.toStdString();
    if (output_file.empty()) return UI_CANCEL;
    return UI_YES;
  } else if (type & UF_SAVE) {
    QString result = QFileDialog::getSaveFileName(this, QString::fromStdString(text),
        ".", QString::fromStdString(wildcard), 0, QFileDialog::DontUseCustomDirectoryIcons
        | QFileDialog::HideNameFilterDetails);
    output_file = result.toStdString();
    if (output_file.empty()) return UI_CANCEL;
    return UI_YES;
  } else return UI_CANCEL;
}