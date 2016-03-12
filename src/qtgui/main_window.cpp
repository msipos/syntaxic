#include "keymapper.hpp"
#include "lm.hpp"
#include "master.hpp"
#include "master_io_provider.hpp"
#include "master_js.hpp"
#include "core/util.hpp"
#include "core/util_path.hpp"
#include "core/utf8_util.hpp"
#include "qtgui/complete_dialog.hpp"
#include "qtgui/docs_gui.hpp"
#include "qtgui/dock.hpp"
#include "qtgui/editor.hpp"
#include "qtgui/feedback_box.hpp"
#include "qtgui/invoke_dialog.hpp"
#include "qtgui/jf_dialog.hpp"
#include "qtgui/main_window.hpp"
#include "qtgui/menu_manager.hpp"
#include "qtgui/my_tab_bar.hpp"
#include "qtgui/pref_dialog.hpp"
#include "qtgui/project_dialog.hpp"
#include "qtgui/register_dialog.hpp"
#include "qtgui/sar_dialog.hpp"
#include "qtgui/sidebar_tree.hpp"
#include "qtgui/ssh_gui.hpp"
#include "recents.hpp"
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
#include <QTimer>
#include <QVBoxLayout>
#include <utility>

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

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), menu_manager(new MenuManager(this)), current_doc(nullptr), window_cursor_x(0), window_cursor_y(0), lm_valid(false), lm_check_counter(10) {

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

      {
        q_sidebar_model = new SidebarModel(this);
        q_sidebar_tree = new SidebarTree(widget);
        q_sidebar_tree->setModel(q_sidebar_model);
        vlayout->addWidget(q_sidebar_tree);
      }

      q_dock_site = new QWidget(this);
      q_dock_site->setLayout(new QVBoxLayout);
      q_dock_site->layout()->setContentsMargins(0, 0, 0, 0);
      vlayout->addWidget(q_dock_site);

      q_sidebar_position_label = new QLabel("", widget);
      q_sidebar_position_label->setContentsMargins(4, 0, 4, 4);
      vlayout->addWidget(q_sidebar_position_label);

      widget->setLayout(vlayout);
      q_splitter->addWidget(widget);
      q_splitter->setStretchFactor(0, 0);
    }

    {
      docs_guis[0] = new DocsGui(this);
      q_splitter->addWidget(docs_guis[0]->root_widget);
      q_splitter->setStretchFactor(1, 5);
    }
    {
      docs_guis[1] = new DocsGui(this);
      q_splitter->addWidget(docs_guis[1]->root_widget);
      q_splitter->setStretchFactor(2, 5);
    }

    {
      QList<int> sizes = {250, 300, 300};
      q_splitter->setSizes(sizes);
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

    q_action_new = new QAction("&New File", this);
    q_action_new->setShortcuts(QKeySequence::New);
    connect(q_action_new, &QAction::triggered, this, &MainWindow::slot_new);
    q_menu_file->addAction(q_action_new);

    QMenu* q_menu_file_new = q_menu_file->addMenu("&New...");

    q_action_new_project = new QAction("&Project...", this);
    connect(q_action_new_project, &QAction::triggered, this, &MainWindow::slot_new_project);
    q_menu_file_new->addAction(q_action_new_project);

    q_action_new_file_browser = new QAction("&File Browser", this);
    connect(q_action_new_file_browser, &QAction::triggered, this, &MainWindow::slot_new_file_browser);
    q_menu_file_new->addAction(q_action_new_file_browser);

    q_action_new_shell = new QAction("S&hell...", this);
    connect(q_action_new_shell, &QAction::triggered, this, &MainWindow::slot_new_shell);
    q_menu_file_new->addAction(q_action_new_shell);

    q_action_new_ssh_connection = new QAction("SSH &Connection...", this);
    connect(q_action_new_ssh_connection, &QAction::triggered, this, &MainWindow::slot_new_ssh_connection);
    q_menu_file_new->addAction(q_action_new_ssh_connection);

    q_action_new_js_console = new QAction("&JS Console...", this);
    connect(q_action_new_js_console, &QAction::triggered, this, &MainWindow::slot_new_js_console);
    q_menu_file_new->addAction(q_action_new_js_console);

    q_action_open = new QAction("&Open...", this);
    q_action_open->setShortcuts(QKeySequence::Open);
    connect(q_action_open, &QAction::triggered, this, &MainWindow::slot_open);
    q_menu_file->addAction(q_action_open);

    q_action_open_project = new QAction("&Open Project...", this);
    connect(q_action_open_project, &QAction::triggered, this, &MainWindow::slot_open_project);
    q_menu_file->addAction(q_action_open_project);

    q_menu_file->addSeparator();

    q_action_save = new QAction("&Save", this);
    q_action_save->setShortcuts(QKeySequence::Save);
    connect(q_action_save, &QAction::triggered, this, &MainWindow::slot_save);
    q_menu_file->addAction(q_action_save);

    q_action_save_as = new QAction("Save &as...", this);
    q_action_save_as->setShortcuts(QKeySequence::SaveAs);
    connect(q_action_save_as, &QAction::triggered, this, &MainWindow::slot_save_as);
    q_menu_file->addAction(q_action_save_as);

    q_menu_file_recent_projects = q_menu_file->addMenu("Recent projects");
    for (int i = 0; i < 10; i++) {
      q_action_file_recent_projects[i] = new QAction("", this);
      q_menu_file_recent_projects->addAction(q_action_file_recent_projects[i]);
    }
    connect(q_menu_file_recent_projects, &QMenu::triggered, this, &MainWindow::slot_recent_project);
    recent_projects_callback();

    q_menu_file_recent_files = q_menu_file->addMenu("Recent files");
    for (int i = 0; i < 10; i++) {
      q_action_file_recent_files[i] = new QAction("", this);
      q_menu_file_recent_files->addAction(q_action_file_recent_files[i]);
    }
    connect(q_menu_file_recent_files, &QMenu::triggered, this, &MainWindow::slot_recent_file);
    recent_files_callback();

    #ifndef CMAKE_MACOSX
    q_menu_file->addSeparator();
    #endif

    q_action_quit = new QAction("&Quit", this);
    q_action_quit->setShortcuts(QKeySequence::Quit);
    q_action_quit->setMenuRole(QAction::QuitRole);
    connect(q_action_quit, &QAction::triggered, this, &MainWindow::close);
    q_menu_file->addAction(q_action_quit);

    // Edit

    q_menu_edit = menuBar()->addMenu("&Edit");

    q_action_edit_undo = new QAction("&Undo", this);
    q_action_edit_undo->setMenuRole(QAction::NoRole);
    connect(q_action_edit_undo, &QAction::triggered, this, &MainWindow::slot_undo);
    q_menu_edit->addAction(q_action_edit_undo);

    q_menu_edit->addSeparator();

    q_action_edit_cut = new QAction("C&ut", this);
    q_action_edit_cut->setShortcuts(QKeySequence::Cut);
    q_action_edit_cut->setMenuRole(QAction::NoRole);
    connect(q_action_edit_cut, &QAction::triggered, this, &MainWindow::slot_cut);
    q_menu_edit->addAction(q_action_edit_cut);

    q_action_edit_copy = new QAction("&Copy", this);
    q_action_edit_copy->setShortcuts(QKeySequence::Copy);
    q_action_edit_copy->setMenuRole(QAction::NoRole);
    connect(q_action_edit_copy, &QAction::triggered, this, &MainWindow::slot_copy);
    q_menu_edit->addAction(q_action_edit_copy);

    q_action_edit_paste = new QAction("&Paste", this);
    q_action_edit_paste->setShortcuts(QKeySequence::Paste);
    q_action_edit_paste->setMenuRole(QAction::NoRole);
    connect(q_action_edit_paste, &QAction::triggered, this, &MainWindow::slot_paste);
    q_menu_edit->addAction(q_action_edit_paste);

    q_action_edit_kill = new QAction("&Kill line", this);
    q_action_edit_kill->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_K));
    connect(q_action_edit_kill, &QAction::triggered, this, &MainWindow::slot_kill);
    q_menu_edit->addAction(q_action_edit_kill);

    q_action_edit_select_all = new QAction("Select &all", this);
    q_action_edit_select_all->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_A));
    q_action_edit_select_all->setMenuRole(QAction::NoRole);
    connect(q_action_edit_select_all, &QAction::triggered, this, &MainWindow::slot_select_all);
    q_menu_edit->addAction(q_action_edit_select_all);

    q_action_edit_select_word = new QAction("Select wo&rd", this);
    q_action_edit_select_word->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_R));
    q_action_edit_select_word->setMenuRole(QAction::NoRole);
    connect(q_action_edit_select_word, &QAction::triggered, this, &MainWindow::slot_select_word);
    q_menu_edit->addAction(q_action_edit_select_word);

    q_menu_edit->addSeparator();

    q_action_edit_comment = new QAction("Co&mment out", this);
    q_action_edit_comment->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_M));
    connect(q_action_edit_comment, &QAction::triggered, this, &MainWindow::slot_comment);
    q_menu_edit->addAction(q_action_edit_comment);

    q_action_edit_uncomment = new QAction("Uncomme&nt", this);
    q_action_edit_uncomment->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_M));
    connect(q_action_edit_uncomment, &QAction::triggered, this, &MainWindow::slot_uncomment);
    q_menu_edit->addAction(q_action_edit_uncomment);

    q_menu_edit->addSeparator();

    q_action_edit_complete = new QAction("Comp&lete", this);
    #ifdef CMAKE_MACOSX
    q_action_edit_complete->setShortcut(QKeySequence(Qt::META + Qt::Key_Space));
    #else
    q_action_edit_complete->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Space));
    #endif
    connect(q_action_edit_complete, &QAction::triggered, this, &MainWindow::slot_complete);
    q_menu_edit->addAction(q_action_edit_complete);

    q_menu_edit->addSeparator();

    q_action_edit_find_replace = new QAction("&Find/Replace...", this);
    q_action_edit_find_replace->setShortcuts(QKeySequence::Find);
    connect(q_action_edit_find_replace, &QAction::triggered, this, &MainWindow::slot_find_replace);
    q_menu_edit->addAction(q_action_edit_find_replace);

    q_action_edit_global_find = new QAction("&Global Find...", this);
    q_action_edit_global_find->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_F));
    connect(q_action_edit_global_find, &QAction::triggered, this, &MainWindow::slot_global_find);
    q_menu_edit->addAction(q_action_edit_global_find);

    q_menu_edit->addSeparator();

    q_action_edit_navigation_mode = new QAction("&Navigation mode", this);
    q_action_edit_navigation_mode->setCheckable(true);
    q_action_edit_navigation_mode->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_G));
    connect(q_action_edit_navigation_mode, &QAction::triggered, this, &MainWindow::slot_navigation_mode);
    q_menu_edit->addAction(q_action_edit_navigation_mode);

    #ifndef CMAKE_MACOSX
    q_menu_edit->addSeparator();
    #endif

    q_action_edit_preferences = new QAction("&Preferences...", this);
    q_action_edit_preferences->setShortcuts(QKeySequence::Preferences);
    q_action_edit_preferences->setMenuRole(QAction::PreferencesRole);
    connect(q_action_edit_preferences, &QAction::triggered, this, &MainWindow::slot_preferences);
    q_menu_edit->addAction(q_action_edit_preferences);

    // Documents

    q_menu_document = menuBar()->addMenu("&Document");

    q_action_document_close = new QAction("&Close document", this);
    q_action_document_close->setShortcuts(QKeySequence::Close);
    connect(q_action_document_close, &QAction::triggered, this, &MainWindow::slot_document_close);
    q_menu_document->addAction(q_action_document_close);

    q_menu_document->addSeparator();

    q_action_document_next = new QAction("&Next document", this);
    #ifndef CMAKE_MACOSX
    q_action_document_next->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_PageDown));
    #else
    q_action_document_next->setShortcut(QKeySequence(Qt::META + Qt::Key_Tab));
    #endif
    connect(q_action_document_next, &QAction::triggered, this, &MainWindow::slot_document_next);
    q_menu_document->addAction(q_action_document_next);

    q_action_document_prev = new QAction("&Previous document", this);
    #ifndef CMAKE_MACOSX
    q_action_document_prev->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_PageUp));
    #else
    q_action_document_prev->setShortcut(QKeySequence(Qt::META + Qt::SHIFT + Qt::Key_Tab));
    #endif
    connect(q_action_document_prev, &QAction::triggered, this, &MainWindow::slot_document_prev);
    q_menu_document->addAction(q_action_document_prev);

    q_menu_document->addSeparator();

    q_action_document_switch_to_other_side = new QAction("&Switch to other side", this);
    #ifdef CMAKE_MACOSX
    q_action_document_switch_to_other_side->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Y));
    #else
    q_action_document_switch_to_other_side->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_H));
    #endif
    connect(q_action_document_switch_to_other_side, &QAction::triggered, this, &MainWindow::slot_document_switch_to_other_side);
    q_menu_document->addAction(q_action_document_switch_to_other_side);

    q_action_document_move_to_other_side = new QAction("&Move document to other side", this);
    q_action_document_move_to_other_side->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Y));
    connect(q_action_document_move_to_other_side, &QAction::triggered, this, &MainWindow::slot_document_move_to_other_side);
    q_menu_document->addAction(q_action_document_move_to_other_side);

    q_menu_document->addSeparator();

    q_action_document_fold = new QAction("&Fold document", this);
    q_action_document_fold->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_L));
    connect(q_action_document_fold, &QAction::triggered, this, &MainWindow::slot_document_fold);
    q_menu_document->addAction(q_action_document_fold);

    q_action_document_unfold = new QAction("&Unfold document", this);
    q_action_document_unfold->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_L));
    connect(q_action_document_unfold, &QAction::triggered, this, &MainWindow::slot_document_unfold);
    q_menu_document->addAction(q_action_document_unfold);

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
    connect(q_action_document_set_bookmark, &QAction::triggered, this, &MainWindow::slot_document_set_bookmark);
    q_menu_document->addAction(q_action_document_set_bookmark);

    q_action_document_jump_to_bookmark = new QAction("&Jump to bookmark", this);
    q_action_document_jump_to_bookmark->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_B));
    connect(q_action_document_jump_to_bookmark, &QAction::triggered, this, &MainWindow::slot_document_jump_to_bookmark);
    q_menu_document->addAction(q_action_document_jump_to_bookmark);

    q_menu_document->addSeparator();

    q_action_document_jump_to_file = new QAction("Jump to &file...", this);
    q_action_document_jump_to_file->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_J));
    connect(q_action_document_jump_to_file, &QAction::triggered, this, &MainWindow::slot_document_jump_to_file);
    q_menu_document->addAction(q_action_document_jump_to_file);

    // Plugins

    q_menu_plugins = menuBar()->addMenu("&Plugins");

    q_action_plugins_manage = new QAction("Manage plugins...", this);
    connect(q_action_plugins_manage, &QAction::triggered, this, &MainWindow::slot_plugins_manage);
    q_menu_plugins->addAction(q_action_plugins_manage);

    q_action_plugins_reload = new QAction("Reload plugins", this);
    q_action_plugins_reload->setShortcut(QKeySequence("F10"));
    connect(q_action_plugins_reload, &QAction::triggered, this, &MainWindow::slot_plugins_reload);
    q_menu_plugins->addAction(q_action_plugins_reload);

    q_menu_plugins->addSeparator();

    q_menu_plugins_actions = new QActionGroup(this);
    q_menu_plugins_actions->setExclusive(false);

    // Help

    q_menu_help = menuBar()->addMenu("&Help");

    q_action_help_about = new QAction("&About", this);
    q_action_help_about->setMenuRole(QAction::AboutRole);
    connect(q_action_help_about, &QAction::triggered, this, &MainWindow::slot_help_about);
    q_menu_help->addAction(q_action_help_about);

    q_action_help_enter_license = new QAction("&License...", this);
    connect(q_action_help_enter_license, &QAction::triggered, this, &MainWindow::slot_help_enter_license);
    q_menu_help->addAction(q_action_help_enter_license);

    q_action_help_online = new QAction("&Online Help", this);
    connect(q_action_help_online, &QAction::triggered, this, &MainWindow::slot_help_online);
    q_menu_help->addAction(q_action_help_online);
  }

  std::function<void()> func;
  menu_manager->init();

  // Pop-uppy things:
  complete_dialog = new CompleteDialog(this);
  sar_dialog = new SarDialog(this, this);
  feedback_box = new FeedbackBox(this);
  invoke_dialog = new InvokeDialog(this, this);
  jf_dialog = new JfDialog(this, this);
  preferences_dialog = new PreferencesDialog(this);
  register_dialog = new RegisterDialog(this);
  project_dialog = new ProjectDialog(this);
  ssh_connections_dialog = new SSHConnectionsDialog(this);

  preferences_changed_hook = master.pref_manager.change_hook.add(
      std::bind(&MainWindow::preferences_changed_callback, this));

  recent_projects_hook = master.recent_projects->recents_changed_hook.add(
      std::bind(&MainWindow::recent_projects_callback, this));

  recent_files_hook = master.recent_files->recents_changed_hook.add(
      std::bind(&MainWindow::recent_files_callback, this));

  // Icons
  app_icon = QIcon(":resources/icon.png");
  setWindowIcon(app_icon);

  // Accept drag and drop.
  setAcceptDrops(true);

  refresh_menu_keys();
  refresh_window_title();
  refresh_cursor_label();
  manage_pane_visibility();

  read_settings();
  lm_check(false);
}

////////////////////////////////////////////////////////////////////////////////

void MainWindow::editor_cursor_moved(int x, int y, int /* cursor_margin */) {
  if (!isVisible()) return;

  QPoint pt = mapFromGlobal(QPoint(x, y));
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
      if (window_height - window_cursor_y < 50) target.setY(window_cursor_y - 180);
      else if (window_height - window_cursor_y < 150) target.setY(window_cursor_y - 80);
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

void MainWindow::editor_redrawn() {
  if (feedback_box->isVisible()) {
    feedback_box->update();
  }
}

void MainWindow::editor_context(QPoint p) {
  // Create menu and execute it.
  QMenu menu;
  menu.addAction(q_action_edit_undo);
  menu.addSeparator();
  menu.addAction(q_action_edit_cut);
  menu.addAction(q_action_edit_copy);
  menu.addAction(q_action_edit_paste);
  menu.addAction(q_action_edit_kill);
  menu.addAction(q_action_edit_select_all);
  menu.addSeparator();
  menu.addAction(q_action_edit_comment);
  menu.addAction(q_action_edit_uncomment);
  menu.addSeparator();
  menu.addAction(q_action_edit_find_replace);
  menu.exec(p);
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

void MainWindow::recent_projects_callback() {
  const std::vector<std::string>& recent_projects = master.recent_projects->get_recents();

  for (unsigned int i = 0; i < 10; i++) {
    if (i >= recent_projects.size()) {
      q_action_file_recent_projects[i]->setVisible(false);
    } else {
      q_action_file_recent_projects[i]->setVisible(true);
      q_action_file_recent_projects[i]->setText(QString::fromStdString(recent_projects[i]));
    }
  }
}

void MainWindow::recent_files_callback() {
  const std::vector<std::string>& recent_files = master.recent_files->get_recents();

  for (unsigned int i = 0; i < 10; i++) {
    if (i >= recent_files.size()) {
      q_action_file_recent_files[i]->setVisible(false);
    } else {
      q_action_file_recent_files[i]->setVisible(true);
      q_action_file_recent_files[i]->setText(QString::fromStdString(recent_files[i]));
    }
  }
}

void MainWindow::preferences_changed_callback() {
  refresh_menu_keys();
}

QKeySequence pks_to_qt_key_sequence(ParsedKeySequence psk, const char* pref_name) {
  int qt_key = psk.code;
  if (psk.is_char) {
    if (qt_key >= 'a' && qt_key <= 'z') {
      qt_key = Qt::Key_A + psk.code - 'a';
    } else if (qt_key == ' ') {
      qt_key = Qt::Key_Space;
    } else if (qt_key == '\t') {
      qt_key = Qt::Key_Tab;
    } else if (qt_key == '\b') {
      qt_key = Qt::Key_Backspace;
    } else {
      printf("Unknown char for key: %d for %s\n", qt_key, pref_name);
    }
  } else {
    if (qt_key == KEY_DELETE) {
      qt_key = Qt::Key_Delete;
    } else if (qt_key == KEY_LEFT_ARROW) {
      qt_key = Qt::Key_Left;
    } else if (qt_key == KEY_RIGHT_ARROW) {
      qt_key = Qt::Key_Right;
    } else if (qt_key == KEY_UP_ARROW) {
      qt_key = Qt::Key_Up;
    } else if (qt_key == KEY_DOWN_ARROW) {
      qt_key = Qt::Key_Down;
    } else if (qt_key == KEY_PAGE_DOWN) {
      qt_key = Qt::Key_PageDown;
    } else if (qt_key == KEY_PAGE_UP) {
      qt_key = Qt::Key_PageUp;
    } else if (qt_key == KEY_HOME) {
      qt_key = Qt::Key_Home;
    } else if (qt_key == KEY_END) {
      qt_key = Qt::Key_End;
    } else {
      printf("Unknown key: %d for %s\n", qt_key, pref_name);
    }
  }
  if (psk.has_alt) qt_key |= Qt::ALT;
  if (psk.has_ctrl) qt_key |= Qt::CTRL;
  if (psk.has_meta) qt_key |= Qt::META;
  if (psk.has_shift) qt_key |= Qt::SHIFT;
  return QKeySequence(qt_key);
}

void set_menu_key(QAction* action, const char* pref_name) {
  std::vector<ParsedKeySequence> psks = KeyMapper::parse_key_sequences(master.pref_manager.get_key(pref_name));
  QList<QKeySequence> qks;
  for (ParsedKeySequence& psk : psks) {
    qks << pks_to_qt_key_sequence(psk, pref_name);
  }
  action->setShortcuts(qks);
}

void MainWindow::refresh_menu_keys() {
  set_menu_key(q_action_edit_undo, "keys.menu.edit.undo");
  set_menu_key(q_action_edit_cut, "keys.menu.edit.cut");
  set_menu_key(q_action_edit_copy, "keys.menu.edit.copy");
  set_menu_key(q_action_edit_paste, "keys.menu.edit.paste");
  set_menu_key(q_action_edit_kill, "keys.menu.edit.kill-line");
  set_menu_key(q_action_edit_select_all, "keys.menu.edit.select-all");
  set_menu_key(q_action_edit_select_word, "keys.menu.edit.select-word");

  set_menu_key(q_action_document_switch_to_other_side, "keys.menu.document.switch-other-side");
  set_menu_key(q_action_document_move_to_other_side, "keys.menu.document.move-other-side");

}

void MainWindow::refresh_cursor_label() {
  Doc* doc = get_active_document();
  if (doc == nullptr) {
    q_sidebar_position_label->hide();
  } else {
    q_sidebar_position_label->show();
    std::string text("Row: ");
    CursorLocation cl = doc->get_cursor();
    text += std::to_string(cl.row + 1);
    text += ", Col: ";
    text += std::to_string(cl.col);

    if (q_action_edit_navigation_mode->isChecked()) text += " NAV";
    q_sidebar_position_label->setText(QString::fromStdString(text));
  }
}

void MainWindow::refresh_window_title() {
  std::string title;
  Doc* document = get_active_document();
  if (document) {
    title += document->get_long_title();
    title += " - Syntaxic";
  } else {
    title = "Syntaxic";
  }
  if (!lm_valid) {
    title += " (Evaluation copy)";
  }
  setWindowTitle(QString::fromStdString(title));
}

void MainWindow::manage_pane_visibility() {
  bool visible1 = true, visible2 = false;
  if (docs_guis[1]->size() > 0) visible2 = true;
  if (docs_guis[0]->size() == 0) visible1 = false;
  if (visible1 == false && visible2 == false) visible1 = true;
  const bool first_active = get_active_collection() == docs_guis[0];

  docs_guis[0]->root_widget->setVisible(visible1);
  docs_guis[1]->root_widget->setVisible(visible2);

  if (visible1 && visible2) {
    if (master.pref_manager.get_bool("splitscreen.manage_size")) {
      int overflow = master.pref_manager.get_int("splitscreen.overflow_size");
      if (!first_active) overflow *= -1;

      QList<int> sizes = q_splitter->sizes();
      if (sizes.size() < 3) return;
      const int total_pane_sizes = sizes[1] + sizes[2];
      sizes[1] = total_pane_sizes/2 + overflow;
      sizes[2] = total_pane_sizes/2 - overflow;
      q_splitter->setSizes(sizes);
    }
  }
}

void MainWindow::write_settings() {
  QSettings settings;

  QList<int> sizes = q_splitter->sizes();
  settings.beginGroup("MainWindow");
  settings.setValue("size", size());
  settings.setValue("pos", pos());
  settings.setValue("sidebar_width", sizes[0]);
  settings.endGroup();
}

void MainWindow::read_settings() {
  QSettings settings;

  settings.beginGroup("MainWindow");
  resize(settings.value("size", QSize(600, 700)).toSize());
  move(settings.value("pos", QPoint(100, 100)).toPoint());
  QList<int> sizes = q_splitter->sizes();
  sizes[1] = 400;
  sizes[0] = settings.value("sidebar_width", 250).toInt();
  if (sizes[0] < 0) sizes[0] = 0;
  q_splitter->setSizes(sizes);
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
    QMessageBox::information(this, "Thank you for evaluating Syntaxic", "Thank you for evaluating Syntaxic!  If you like it, please buy a license to support further development.");
    return;
  }

  bool pass = false;
  if (well_formed(skey)) {
    LMKey lmkey = string_to_key(skey);
    pass = check(lmkey, email);
  }

  if (!pass) {
    lm_valid = false;
    QMessageBox::critical(this, "Thank you for evaluating Syntaxic", "Thank you for evaluating Syntaxic!  Unfortunately, your license doesn't pass validation. Please enter a correct license or contact support@kpartite.com for help.");
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
  q_action_edit_select_all->setEnabled(d);
  q_action_edit_select_word->setEnabled(d);
  q_action_edit_kill->setEnabled(d);
  q_action_edit_comment->setEnabled(d);
  q_action_edit_uncomment->setEnabled(d);
  q_action_edit_complete->setEnabled(d);
  q_action_edit_find_replace->setEnabled(d);
  q_action_edit_navigation_mode->setEnabled(d);
  q_action_document_close->setEnabled(d);
  q_action_document_fold->setEnabled(d);
  q_action_document_unfold->setEnabled(d);
  q_menu_document_type->setEnabled(d);
  q_action_document_set_bookmark->setEnabled(d);
}

void MainWindow::configure_project(Project* project) {
  project_dialog->project_spec = project->project_spec;
  int result = project_dialog->exec_dialog(false);
  if (result == QDialog::Accepted) {
    project->project_spec.filtering_pattern = project_dialog->project_spec.filtering_pattern;
    project->reconfigure_project();
    project->refresh_root_node(q_sidebar_model);
    project->save_project();
  }
}

MainWindow::~MainWindow() {
}

/////////////////////////////////////////////////////////////////////// QT Slots

void MainWindow::slot_new() {
  master.set_markovian(MARKOVIAN_NONE);
  master.new_document(this);
}

void MainWindow::slot_open() {
  master.set_markovian(MARKOVIAN_NONE);
  QString fileName = QFileDialog::getOpenFileName(this, "Open File",
      QString::fromStdString(master.settings.get_current_path()), "All Files (*)");
  std::string str = fileName.toStdString();
  if (str.empty()) return;
  master.open_document(str.c_str(), this);
  lm_check();
}

void MainWindow::slot_new_project() {
  master.set_markovian(MARKOVIAN_NONE);

  int result = project_dialog->exec_dialog(true);
  if (result == QDialog::Accepted) {
    master.new_project(project_dialog->project_spec);
  }
}

void MainWindow::slot_new_file_browser() {
  master.set_markovian(MARKOVIAN_NONE);
  std::unique_ptr<STree> fp(new FileBrowser(UtilPath::home_path()));
  master.add_file_provider(std::move(fp));
}

void MainWindow::slot_new_shell() {
  master.set_markovian(MARKOVIAN_NONE);
  master.new_shell();
}

void MainWindow::slot_new_ssh_connection() {
  if (ssh_connections_dialog->do_dialog()) {
    SSHConnectionInfo* sci = ssh_connections_dialog->chosen_connection;
    try {
      master_io_provider->add_ssh(sci->name, sci->cmd_line, sci->login_events);
    } catch (std::exception& e) {
      QMessageBox::critical(this, "SSH Error", QString::fromStdString(std::string("Failed to create connection: ") + e.what()));

    }
  }
}

void MainWindow::slot_new_js_console() {
  master.set_markovian(MARKOVIAN_NONE);
  master.new_js_console();
}

void MainWindow::slot_open_project() {
  master.set_markovian(MARKOVIAN_NONE);
  std::string out_file;
  int rv =  MainWindow::get_user_file(out_file, "Open project...", "Syntaxic projects (*.synproj)",
     UF_OPEN);
  if (rv == UI_YES) {
    master.open_project(out_file);
  }
}

void MainWindow::slot_save() {
  master.set_markovian(MARKOVIAN_NONE);
  Doc* doc = get_active_document();
  Document* document = dynamic_cast<Document*>(doc);
  if (document != nullptr) master.save_document(document);
  lm_check();
}

void MainWindow::slot_save_as() {
  master.set_markovian(MARKOVIAN_NONE);
  Doc* doc = get_active_document();
  Document* document = dynamic_cast<Document*>(doc);
  if (document != nullptr) master.save_document_as(document);
  lm_check();
}

void MainWindow::slot_recent_project(QAction* act) {
  master.set_markovian(MARKOVIAN_NONE);
  std::string proj = act->text().toStdString();
  master.open_project(proj);
}

void MainWindow::slot_recent_file(QAction* act) {
  master.set_markovian(MARKOVIAN_NONE);
  std::string file = act->text().toStdString();
  master.open_document(file.c_str(), this);
}

void MainWindow::slot_undo(){
  master.set_markovian(MARKOVIAN_NONE);
  Doc* doc = get_active_document();
  if (doc != nullptr) doc->handle_undo();
}

void MainWindow::slot_copy(){
  master.set_markovian(MARKOVIAN_NONE);
  Doc* doc = get_active_document();
  if (doc != nullptr) {
    QClipboard* clipboard = QGuiApplication::clipboard();
    std::string txt = doc->handle_copy();
    if (!txt.empty()) clipboard->setText(QString::fromStdString(txt));
  }
}

void MainWindow::slot_cut(){
  master.set_markovian(MARKOVIAN_NONE);
  Doc* document = get_active_document();
  if (document != nullptr) {
    QClipboard* clipboard = QGuiApplication::clipboard();
    std::string txt = document->handle_cut();
    if (!txt.empty()) clipboard->setText(QString::fromStdString(txt));
  }
}

void MainWindow::slot_paste() {
  master.set_markovian(MARKOVIAN_NONE);
  Doc* document = get_active_document();
  if (document != nullptr) {
    QClipboard* clipboard = QGuiApplication::clipboard();
    std::string text = clipboard->text().toStdString();
    text = utf8_convert_best(text.c_str(), text.size());
    document->handle_paste(text);
  }
  lm_check();
}

void MainWindow::slot_kill() {
  master.set_markovian(MARKOVIAN_KILL);
  Doc* document = get_active_document();
  if (document != nullptr) {
    std::string txt = document->handle_kill();
    if (txt.empty()) return;
    QClipboard* clipboard = QGuiApplication::clipboard();
    if (master.get_markovian_avalanche() > 1) {
      txt = clipboard->text().toStdString() + txt;
    }
    clipboard->setText(QString::fromStdString(txt));
  }
}

void MainWindow::slot_select_word() {
  master.set_markovian(MARKOVIAN_NONE);
  Doc* document = get_active_document();
  if (document != nullptr) {
    document->handle_select_word();
  }
}

void MainWindow::slot_select_all() {
  master.set_markovian(MARKOVIAN_NONE);
  Doc* document = get_active_document();
  if (document != nullptr) {
    document->handle_select_all();
  }
}

void MainWindow::slot_comment() {
  master.set_markovian(MARKOVIAN_NONE);
  Doc* document = get_active_document();
  if (document != nullptr) document->handle_comment();
}

void MainWindow::slot_uncomment() {
  master.set_markovian(MARKOVIAN_NONE);
  Doc* document = get_active_document();
  if (document != nullptr) document->handle_uncomment();
}

void MainWindow::slot_complete() {
  Doc* document = get_active_document();
  if (document == nullptr) return;
  DocsGui* dg = get_active_collection();
  if (dg == nullptr) return;
  complete_dialog->show(dg->editor);
  move_away(complete_dialog, PL_CURSOR);
}

void MainWindow::slot_find_replace(){
  master.set_markovian(MARKOVIAN_NONE);
  Doc* doc = get_active_document();
  Document* document = dynamic_cast<Document*>(doc);
  if (document != nullptr) {
    // Disable on MacOSX because it conflicts with Command+F and Command+R in the dialog.
    q_action_edit_find_replace->setEnabled(false);
    q_action_edit_select_word->setEnabled(false);

    // Pass selection into the SAR dialog.
    TextView* tv = document->get_text_view();
    std::string text = tv->selection_as_string_first_line();
    if (!text.empty()) tv->selection_move_to_origin();
    sar_dialog->execute(text);
  }
}

void MainWindow::slot_global_find() {
  master.set_markovian(MARKOVIAN_NONE);
  Doc* doc = get_active_document();
  Document* document = dynamic_cast<Document*>(doc);
  std::string text;
  if (document != nullptr) {
    TextView* tv = document->get_text_view();
    text = tv->selection_as_string_first_line();
  }

  bool ok;
  QString qresult = QInputDialog::getText(this, "Global find", "Enter text to find:", QLineEdit::Normal, QString::fromStdString(text), &ok);
  if (!ok) return;
  text = qresult.toStdString();

  master.global_find(text);
}

void MainWindow::slot_navigation_mode() {
  master.set_markovian(MARKOVIAN_NONE);
  bool nm = q_action_edit_navigation_mode->isChecked();
  for (int i = 0; i < 2; i++) {
    docs_guis[i]->editor->set_navigation_mode(nm);
  }
  refresh_cursor_label();
}

void MainWindow::slot_preferences() {
  master.set_markovian(MARKOVIAN_NONE);
  if (preferences_dialog->exec()) {
    master.reload_settings();
  }
}

void MainWindow::slot_document_close() {
  master.set_markovian(MARKOVIAN_NONE);
  Doc* doc = get_active_document();
  if (doc != nullptr) master.close_document(doc);
}

void MainWindow::slot_document_next() {
  master.set_markovian(MARKOVIAN_NONE);
  next_document();
}

void MainWindow::slot_document_prev() {
  master.set_markovian(MARKOVIAN_NONE);
  prev_document();
}

void MainWindow::slot_document_move_to_other_side() {
  master.set_markovian(MARKOVIAN_NONE);
  Doc* doc = get_active_document();
  if (doc == nullptr) return;
  DocsGui* dg = get_active_collection();
  if (dg == nullptr) return;

  DocsGui* other_dg = (dg == docs_guis[0]) ? docs_guis[1] : docs_guis[0];
  dg->remove_doc(doc);
  other_dg->add_doc(doc, true);
  manage_pane_visibility();
}

void MainWindow::slot_document_switch_to_other_side() {
  master.set_markovian(MARKOVIAN_NONE);
  DocsGui* dg = get_active_collection();
  if (dg == nullptr) return;

  DocsGui* other_dg = (dg == docs_guis[0]) ? docs_guis[1] : docs_guis[0];
  if (other_dg->root_widget->isVisible()) {
    other_dg->editor->setFocus();
  }
}

void MainWindow::slot_document_fold() {
  master.set_markovian(MARKOVIAN_NONE);
  Doc* document = get_active_document();
  if (document != nullptr) document->handle_fold(true);
}

void MainWindow::slot_document_unfold() {
  master.set_markovian(MARKOVIAN_NONE);
  Doc* document = get_active_document();
  if (document != nullptr) document->handle_fold(false);
}

void MainWindow::slot_document_temp() {
  Doc* doc = get_active_document();
  Document* document = dynamic_cast<Document*>(doc);
  master.set_markovian(MARKOVIAN_NONE);
  CursorLocation cl = document->get_text_view()->get_cursor();
  master.save_temp_bookmark(document->get_text_file()->get_absolute_path(), cl.row, cl.col);
  master.open_temp_document(this);
}

void MainWindow::slot_document_type(QAction* action) {
  master.set_markovian(MARKOVIAN_NONE);
  Doc* document = get_active_document();
  if (document != nullptr) {
    std::string new_type = action->text().toStdString();
    document->handle_change_type(new_type);
    DocsGui* docs_gui = get_active_collection();
    if (docs_gui) docs_gui->editor->update();
  }
}

void MainWindow::slot_document_set_bookmark(){
  master.set_markovian(MARKOVIAN_NONE);
  Doc* doc = get_active_document();
  Document* document = dynamic_cast<Document*>(doc);

  if (document != nullptr) {
    std::string path = document->get_text_file()->get_absolute_path();
    if (path.empty()) {
      // TODO: Not ideal.
      feedback("Can't set bookmark", "Cannot set bookmark in a file that is unsaved!");
      return;
    }
    CursorLocation cl = document->get_cursor();
    master.add_bookmark(path, cl.row, cl.col);
  }
}

void MainWindow::slot_document_jump_to_bookmark(){
  master.set_markovian(MARKOVIAN_BOOKMARK);
  master.jump_to_bookmark();
}

void MainWindow::slot_document_jump_to_file(){
  master.set_markovian(MARKOVIAN_NONE);
  jf_dialog->show();
}

void MainWindow::slot_plugins_manage() {
  master.open_document(master_js->get_syntaxic_js_file().c_str(), this);
}

void MainWindow::slot_plugins_reload() {
  std::string output = master_js->eval_syntaxic_js_file();
  if (output == "undefined") output = "No errors.";
  master.feedback("Plugins reloaded", output);
}


void MainWindow::slot_tools_invoke() {
  master.set_markovian(MARKOVIAN_NONE);
  invoke_dialog->show();
}

void MainWindow::slot_help_about(){
  master.set_markovian(MARKOVIAN_NONE);
  master.open_about();
}

void MainWindow::slot_help_enter_license() {
  master.set_markovian(MARKOVIAN_NONE);
  register_dialog->exec();
  lm_check_counter = 100;
  lm_check(false);
  refresh_window_title();
}
void MainWindow::slot_help_online() {
  master.set_markovian(MARKOVIAN_NONE);
  QDesktopServices::openUrl(QUrl("https://syntaxiceditor.com/help/"));
}

/////////////////////////////////////////////////////////////////////// Implement UIWindow interface

int MainWindow::get_num_collections() {
  return 2;
}

DocsGui* MainWindow::get_active_collection() {
  Doc* doc = get_active_document();
  if (doc == nullptr) return nullptr;
  return dynamic_cast<DocsGui*>(doc->get_collection());
}

DocCollection* MainWindow::get_collection(int index) {
  if (index < 0 || index > 1) return nullptr;
  return docs_guis[index];
}

void MainWindow::restore_document_type() {
  Doc* doc = get_active_document();
  if (doc != nullptr) {
    QList<QAction*> actions = q_action_document_type->actions();
    for (QAction* action: actions) {
      if (action->text().toStdString() == doc->get_appendage().file_type) {
        action->setChecked(true);
        return;
      }
    }
  }
}

void MainWindow::set_current_doc(Doc* d) {
  current_doc = d;

  refresh_window_title();
  refresh_cursor_label();
  menu_actions_enablement();
  restore_document_type();
  manage_pane_visibility();
}

void MainWindow::add_document(Doc* doc, bool switch_to_doc) {
  DocsGui* dg = get_active_collection();
  if (dg == nullptr) dg = docs_guis[0];
  dg->add_doc(doc, switch_to_doc);
}

void MainWindow::remove_document(Doc* doc) {
  for (int i = 0; i < 2; i++) {
    docs_guis[i]->remove_doc(doc);
  }
}

void MainWindow::go_to_document(Doc* doc) {
  for (int i = 0; i < 2; i++) {
    docs_guis[i]->go_to_doc(doc);
  }
}

void MainWindow::next_document() {
  DocsGui* dg = get_active_collection();
  if (dg != nullptr) dg->next_doc();
}

void MainWindow::prev_document() {
  DocsGui* dg = get_active_collection();
  if (dg != nullptr) dg->prev_doc();
}

void MainWindow::add_file_provider(STree* fp, bool expand) {
  fp->set_ui_window(this);
  q_sidebar_tree->add_provider_first(fp, expand);
}

void MainWindow::remove_file_provider(STree* fp) {
  fp->set_ui_window(this);
  q_sidebar_tree->remove_provider(fp);
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

int MainWindow::get_user_text_input(const std::string& title, const std::string& text, const std::string& entered_text, int /* type */, std::string& result) {
  bool ok;
  QString qresult = QInputDialog::getText(this, QString::fromStdString(title),
      QString::fromStdString(text), QLineEdit::Normal, QString::fromStdString(entered_text), &ok);
  if (!ok) return UI_CANCEL;
  result = qresult.toStdString();
  return UI_OK;
}

int MainWindow::get_user_file(std::string& output_file, const std::string& text, const std::string& wildcard, int type) {
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

Dock* MainWindow::add_new_dock() {
  std::unique_ptr<Dock> dock(new Dock(q_dock_site));
  q_dock_site->layout()->addWidget(dock.get());
  stored_docks.push_back(std::move(dock));
  return stored_docks.back().get();
}

void MainWindow::remove_dock(Dock* dock_to_remove) {
  for (unsigned int i = 0; i < stored_docks.size(); i++) {
    auto& dock = stored_docks[i];
    if (dock.get() == dock_to_remove) {
      stored_docks.erase(stored_docks.begin() + i);
      return;
    }
  }
}
