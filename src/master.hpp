#ifndef SYNTAXIC_MASTER_HPP
#define SYNTAXIC_MASTER_HPP

#include "core/common.hpp"
#include "stree.hpp"
#include "keymapper.hpp"
#include "known_documents.hpp"
#include "preferences.hpp"
#include "project.hpp"
#include "settings.hpp"
#include "statlang/statlang.hpp"
#include "theme.hpp"
#include "tool.hpp"

#include <unordered_set>
#include <vector>

class Document;
class Recents;
class UIWindow;

struct Bookmark {
  std::string abs_path;
  int row, col;
};

#define MARKOVIAN_NONE     0
#define MARKOVIAN_BOOKMARK 1
#define MARKOVIAN_KILL     2

class Master {
private:
  std::vector<Bookmark> bookmarks;
  Bookmark temp_bookmark;
  std::vector<std::unique_ptr<Doc>> documents;
  std::unique_ptr<Document> temp_doc;

  std::unique_ptr<UIWindow> main_window;

  std::vector<std::unique_ptr<UIWindow>> uiwindows;
  std::vector<std::unique_ptr<SynTool>> tools;
  std::vector<std::unique_ptr<STree>> file_providers;

  KeyMapper key_mapper_main, key_mapper_navigation;
  int markovian;
  int markovian_avalanche;

public:
  Master();
  Master(const Master&) = delete;
  Master& operator=(const Master&) = delete;
  ~Master();

  Settings settings;
  StatLang stat_lang;
  ThemeEngine theme_engine;
  PrefManager pref_manager;
  std::unique_ptr<Recents> recent_projects;
  std::unique_ptr<Recents> recent_files;

  void debug();

  void set_markovian(int m);
  inline int get_markovian() { return markovian; }
  inline int get_markovian_avalanche() { return markovian_avalanche; }

  //// Key mappers are publicly available
  inline KeyMapper& get_key_mapper_main() { return key_mapper_main; }
  inline KeyMapper& get_key_mapper_navigation() { return key_mapper_navigation; }

  void init_master();

  //// Window interface

  /** Make a new window. */
  void create_main_window();
  void add_uiwindow(std::unique_ptr<UIWindow> w);
  inline UIWindow* get_main_window() { return main_window.get(); }

  inline std::vector<std::unique_ptr<Doc>>& get_documents() {
    return documents;
  }

  //// File providers

  void add_file_provider(std::unique_ptr<STree> fp, bool expand=true);
  void remove_file_provider(STree* fp);
  void save_file_providers();

  ///////////////////////////////////////////////////////////////////// Main callbacks

  /** Open a new file.   If preferred_window is not null-ptr, then that window will be
   used to open the file. */
  void new_document(UIWindow* preferred_window);

  /** Create a new shell. */
  void new_shell();

  /** Create a new JS console. */
  void new_js_console();

  /** p is a utf-8 encoded path.  If preferred_window is not null-ptr, then that window will be
   used to open the file.

   If file is already opened, then go_to_document() will be called instead. */
  void open_document(const char* p, UIWindow* preferred_window, int row=-1, int col=-1);

  /** Open up temp document in this window. */
  void open_temp_document(UIWindow* preferred_window);

  /** Open a temporary, read-only document. */
  void open_temp_read_only_document(const std::string& title, const std::string& text);
  void open_temp_read_only_document(const std::string& title, const TextBuffer& tb);

  /** Save a temp bookmark. */
  void save_temp_bookmark(const std::string& abs_path, int row, int col);

  /** Jump to temp bookmark, if any. */
  void jump_temp_bookmark();

  /** Save the document.  Return true if it was saved. May prompt for user input and refuse saving.
  */
  bool save_document(Document* document);

  /** Save the document as.  Return true if it was saved. May prompt for user input and refuse
  saving. */
  bool save_document_as(Document* document);

  /** Close this document.  Return true if it has been closed. May prompt for user input and refuse
  closing. */
  bool close_document(Doc* document);

  /** New project. */
  void new_project(ProjectSpec spec);

  /** Open project. */
  void open_project(const std::string& path);

  /** Attempt to quit. Return false if the user canceled it. */
  bool quit();

  /** Open about page. */
  void open_about();

  /** Open help page. */
  void open_help();

  /** Open welcome page. */
  void open_welcome();

  /** Delete all windows. */
  void delete_windows();

  /** Add a bookmark. */
  void add_bookmark(const std::string& abs_path, int row, int col);

  /** Jump to bookmark. */
  void jump_to_bookmark();

  /** Feedback to all the windows. */
  void feedback(const std::string& title, const std::string& text);

  /** Get all known documents. */
  void get_known_documents(std::vector<KnownDocument>& known_docs);

  /** Trigger a settings reload for all documents. */
  void reload_settings();

  /** Get list of commands that were performed previously. */
  void get_past_invokes(std::vector<std::string>& commands);

  /** Add an invoke into the list of previous ones. */
  void add_invoke(const std::string& cmd);

  /** Run a tool. */
  void run_tool(const std::string& cmd, bool wants_temp_buffer, const std::string& cwd);

  /** Let the master know that the tool has finished running. */
  void tool_finished(SynTool*);

  /** Try to go to a navigable. */
  void go_to_navigable(const std::string& navigable);

  /** Do a global find. */
  void global_find(const std::string& term);


  //////// Useful for JS interface having to do with doc handles.
  int js_get_current_doc();
  int js_get_shell_doc();
  Doc* js_get_doc(int handle);
  int js_get_doc_handle(Doc* doc);
};

extern Master master;

#endif