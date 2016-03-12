#ifndef QTGUI_MAIN_WINDOW_HPP
#define QTGUI_MAIN_WINDOW_HPP

#include "core/hooks.hpp"
#include "uiwindow.hpp"

#include <QMainWindow>
#include <QPoint>

enum Modality {
  MODALITY_NORMAL, MODALITY_SEARCH_REPLACE
};

/** Used with move_away(). */
enum PreferredLocation {
  PL_HCENTER, PL_BOTTOM_RIGHT, PL_CURSOR
};

class QAction;
class QActionGroup;
class QDragEnterEvent;
class QDropEvent;
class QFrame;
class QLabel;
class QMenu;
class QPalette;
class QScrollArea;
class QSplitter;
class CompleteDialog;
class DocsGui;
class Dock;
class Editor;
class FeedbackBox;
class STree;
class InvokeDialog;
class JfDialog;
class MenuManager;
class MyTabBar;
class PreferencesDialog;
class Project;
class ProjectDialog;
class RegisterDialog;
class SarDialog;
class SidebarList;
class SidebarModel;
class SidebarTree;
class SSHConnectionsDialog;

class MainWindow : public QMainWindow, public UIWindow {
  Q_OBJECT

  friend class DocsGui;
  friend class Editor;
  friend class MenuManager;

private:
  ///////////////////////////////////////////////////////////// QT:

  QMenu* q_menu_file;
    QAction* q_action_new;
    QAction* q_action_new_project;
    QAction* q_action_new_file_browser;
    QAction* q_action_new_shell;
    QAction* q_action_new_ssh_connection;
    QAction* q_action_new_js_console;
    QAction* q_action_open;
    QAction* q_action_open_project;
    QAction* q_action_save;
    QAction* q_action_save_as;
    QMenu* q_menu_file_recent_projects;
      QAction* q_action_file_recent_projects[10];
    QMenu* q_menu_file_recent_files;
      QAction* q_action_file_recent_files[10];
    QAction* q_action_quit;
  QMenu* q_menu_edit;
    QAction* q_action_edit_undo;
    QAction* q_action_edit_cut;
    QAction* q_action_edit_copy;
    QAction* q_action_edit_paste;
    QAction* q_action_edit_kill;
    QAction* q_action_edit_select_word;
    QAction* q_action_edit_select_all;
    QAction* q_action_edit_comment;
    QAction* q_action_edit_uncomment;
    QAction* q_action_edit_complete;
    QAction* q_action_edit_find_replace;
    QAction* q_action_edit_global_find;
    QAction* q_action_edit_navigation_mode;
    QAction* q_action_edit_preferences;
  QMenu* q_menu_document;
    QAction* q_action_document_close;
    QAction* q_action_document_next;
    QAction* q_action_document_prev;
    QAction* q_action_document_move_to_other_side;
    QAction* q_action_document_switch_to_other_side;
    QAction* q_action_document_fold;
    QAction* q_action_document_unfold;
    QAction* q_action_document_temp;
    QMenu* q_menu_document_type;
    QActionGroup* q_action_document_type;
    QAction* q_action_document_set_bookmark;
    QAction* q_action_document_jump_to_bookmark;
    QAction* q_action_document_jump_to_file;
  QMenu* q_menu_plugins;
    QAction* q_action_plugins_manage;
    QAction* q_action_plugins_reload;
    QActionGroup* q_menu_plugins_actions;
  QMenu* q_menu_help;
    QAction* q_action_help_about;
    QAction* q_action_help_enter_license;
    QAction* q_action_help_online;

  QSplitter* q_splitter;
    // Sidebar:
      SidebarTree* q_sidebar_tree;
      SidebarModel* q_sidebar_model;
      QWidget* q_dock_site;
      QLabel* q_sidebar_position_label;
    // Main area:
      DocsGui* docs_guis[2];

  // Popupy things:
  CompleteDialog* complete_dialog;
  FeedbackBox* feedback_box;
  SarDialog* sar_dialog;
  InvokeDialog* invoke_dialog;
  JfDialog* jf_dialog;
  PreferencesDialog* preferences_dialog;
  ProjectDialog* project_dialog;
  RegisterDialog* register_dialog;
  SSHConnectionsDialog* ssh_connections_dialog;

public:
  QIcon app_icon;
  std::unique_ptr<MenuManager> menu_manager;

private:

  ///////////////////////////////////////////////////////////// Events:

  virtual void closeEvent(QCloseEvent* event);
  virtual void dragEnterEvent(QDragEnterEvent *event);
  virtual void dropEvent(QDropEvent* event);

  ///////////////////////////////////////////////////////////// Internal data:

  Hook<> recent_projects_hook;
  void recent_projects_callback();

  Hook<> recent_files_hook;
  void recent_files_callback();

  Hook<> preferences_changed_hook;
  void preferences_changed_callback();

  /** Callback from DocsGuis. */
  void set_current_doc(Doc* d);
  Doc* current_doc;

  int window_cursor_x, window_cursor_y;

  // Docks
  std::vector<std::unique_ptr<Dock>> stored_docks;

  // License
  bool lm_valid;
  int lm_check_counter;
  void lm_check(bool show_message=true);

  void refresh_menu_keys();
  void refresh_cursor_label();
  void refresh_window_title();
  void manage_pane_visibility();

  ///////////////////////////////////////////////////////////// Document interface:

  void restore_document_type();
  void next_document();
  void prev_document();
  void write_settings();
  void read_settings();

private slots:
  void slot_new();
  void slot_open();
  void slot_new_project();
  void slot_open_project();
  void slot_new_file_browser();
  void slot_new_shell();
  void slot_new_ssh_connection();
  void slot_new_js_console();
  void slot_save();
  void slot_save_as();
  void slot_recent_project(QAction*);
  void slot_recent_file(QAction*);
  void slot_undo();
  void slot_copy();
  void slot_cut();
  void slot_paste();
  void slot_kill();
  void slot_select_word();
  void slot_select_all();
  void slot_comment();
  void slot_uncomment();
  void slot_complete();
  void slot_find_replace();
  void slot_global_find();
  void slot_navigation_mode();
  void slot_preferences();
  void slot_document_close();
  void slot_document_move_to_other_side();
  void slot_document_switch_to_other_side();
  void slot_document_fold();
  void slot_document_unfold();
  void slot_document_temp();
  void slot_document_type(QAction*);
  void slot_document_set_bookmark();
  void slot_document_jump_to_bookmark();
  void slot_document_jump_to_file();
  void slot_tools_invoke();
  void slot_plugins_manage();
  void slot_plugins_reload();
  void slot_help_about();
  void slot_help_enter_license();
  void slot_help_online();

public slots:
  void slot_document_next();
  void slot_document_prev();

public:
  MainWindow(QWidget* parent);

  ///////////////////////////////////////////////////////////// Cursor/scroll control/editor:

  inline Doc* get_active_document() { return current_doc; }
  DocsGui* get_active_collection();

  /** Allow the Editor widget to call back with current cursor position. */
  void editor_cursor_moved(int x, int y, int cursor_margin);

  /** Move the widget such that it doesn't overlap the cursor. */
  void move_away(QWidget* widget, PreferredLocation pl);

  /** Allow the Editor widget to call back on paint. */
  void editor_redrawn();

  /** Allow the editor to call back on context event. */
  void editor_context(QPoint p);

  /** Enable/disable menu actions depending on document. */
  void menu_actions_enablement();

  /** Configure project. */
  void configure_project(Project*);

  ///////////////////////////////////////////////////////////// Other:

  // UIWindow interface:
  virtual int get_num_collections();
  virtual DocCollection* get_collection(int index);
  virtual void add_document(Doc* doc, bool switch_to_doc);
  virtual void remove_document(Doc* doc);
  virtual void go_to_document(Doc* doc);
  virtual void add_file_provider(STree* fp, bool expand=true);
  virtual void remove_file_provider(STree* fp);

  virtual int get_user_input(const std::string& title, const std::string& text, int type);
  virtual int get_user_text_input(const std::string& title, const std::string& text, const std::string& entered_text, int type, std::string& result);
  virtual int get_user_file(std::string& output_file, const std::string& text, const std::string& wildcard, int type);
  virtual void feedback(const std::string& title, const std::string& text);
  virtual ~MainWindow();

  // Dock interface:
  Dock* add_new_dock();
  void remove_dock(Dock* dock);
};

#endif