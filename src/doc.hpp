#ifndef SYNTAXIC_DOC_HPP
#define SYNTAXIC_DOC_HPP

#include "core/common.hpp"
#include "core/hooks.hpp"
#include "core/rich_text.hpp"
#include "core/visual_payload.hpp"

#include <string>
#include <vector>

/** Hook event types. */
namespace DocEvent {
  enum Type {
    // Cursor has moved.
    CURSOR_MOVED = 1,
    // Cursor should be centralized.
    CENTRALIZE = 2,
    // Viewport should page up.
    PAGE_UP = 4,
    // Viewport should page down.
    PAGE_DOWN = 8,
    // Text has likely changed.
    EDITED = 16,
    // Savedness or another state of the file has changed.
    CHANGED_STATE = 32,
    // Document has just been opened.
    OPENED = 64,
    // Document has changed its path. (?)
    CHANGED_PATH = 128,
    // Document has changed the statlang type.
    CHANGED_TYPE = 256,
    // Document has folded or unfolded
    FOLDED = 512,
    // Document is closing.
    CLOSING = 1024,
    // Document is about to be saved.  It can be edited.
    BEFORE_SAVE = 2048,
    // Document has been saved.
    AFTER_SAVE = 4096,
    // Newline was pressed in the document
    NEWLINE = 8192,
  };
}

/** Some keyboard driven actions. Usually these require no additional info apart from the keypress and flowgrid. */
namespace DocAction {
  enum Type {
    NONE = 0,
    MOVE_DOWN,
    MOVE_UP,
    MOVE_LEFT,
    SKIP_LEFT,
    MOVE_RIGHT,
    SKIP_RIGHT,
    SKIP_DOWN,
    SKIP_UP,
    MOVE_PAGE_UP,
    MOVE_PAGE_DOWN,
    MOVE_HOME,
    MOVE_END,
    MOVE_HOME_FILE,
    MOVE_END_FILE,
    DELETE_FORWARD,
    INDENT,
    UNINDENT,
    DELETE_WORD
  };
}

namespace DocFlag {
  enum Type {
    /** Is a read only document. */
    READ_ONLY = 1,
    /** Is a temporary document, i.e. don't warn about closing it, and have no *. */
    TEMP_DOC = 2,
    /** Should be rendered with a shell theme. */
    SHELL_THEME = 4,
    /** Certain features should be disabled, as this is a big document and we don't want editor to be slow. */
    BIG_DOC = 8
  };
}

class Doc;
class TextBuffer;
class UIWindow;

struct DocSearchResults {
  int num_results;
  int cur_result;
  // Currently found result (in case of num_results > 0).
  int row, col;
};

class DocAppendage {
public:
  /** This is used to memorize when switching tabs. */
  int scroll_x, scroll_y;

  /** Editor cursor position last memorized. */
  int cursor_x, cursor_y;

  /** Used by StatLang. */
  std::string file_type;
  int statlang_id;

  /** Tab definition. */
  int tabdef;

  /** Is it folded? */
  // TODO: Get rid of this, it is not used any more.
  bool folded;

  /** Rich text additions. */
  RichText rich_text;

  inline DocAppendage() : scroll_x(0), scroll_y(0), statlang_id(0), tabdef(2), folded(false) {}
};

class DocCollection {
protected:
  int current_index;
  std::vector<Doc*> docs;

public:
  inline DocCollection() : current_index(-1) {}

  inline int get_current_index() const { return current_index; }
  Doc* get_current_doc() const;
  inline unsigned int size() const { return docs.size(); }

  virtual void add_doc(Doc* doc, bool switch_to_doc) = 0;
  virtual void remove_doc(Doc* doc) = 0;
  virtual void go_to_doc(Doc* doc) = 0;
};

/** Mostly this is a virtualized interface for Document. */
class Doc {
private:
  DocAppendage appendage;
  DocCollection* collection;
  UIWindow* window;
  VisualPayload visual_payload;

protected:
  void call_hook(int flags);

public:
  Doc();
  virtual ~Doc();

  // Disable copy constructors
  Doc(const Doc&) = delete;
  Doc& operator=(const Doc&) = delete;

  inline DocAppendage& get_appendage() { return appendage; }
  inline const DocAppendage& get_appendage() const { return appendage; }
  inline DocCollection* get_collection() { return collection; }
  inline void set_collection(DocCollection* c) { collection = c; }
  inline UIWindow* get_window() { return window; }
  inline void set_window(UIWindow* w) { window = w; }
  inline VisualPayload get_visual_payload() { return visual_payload; }
  inline void set_visual_payload(VisualPayload vp) { visual_payload = vp; }

  // Needed functions

  /** Get the hook that triggers on this document. */
  virtual HookSource<Doc*, int>* get_doc_hook() = 0;
  /** Get the contents TextBuffer. */
  virtual const TextBuffer* get_text_buffer() const = 0;
  /** Get current cursor location. */
  virtual CursorLocation get_cursor() const = 0;
  /** Get selection information. */
  virtual SelectionInfo get_selection() const = 0;
  /** Get short title. */
  virtual std::string get_short_title() const = 0;

  // Optional functions

  /** Get long title of the document. Default implementation returns short title. */
  virtual std::string get_long_title() const;

  /** Return combination of DocFlags. */
  virtual int get_display_style() const;

  // Actions, default implementation is always to do nothing at all.

  /** Handle a "raw" keypress. This can defer to handle_action and handle_char if*/
  virtual void handle_raw_char(KeyPress key);
  /** Action on a document. This is only called by handle_raw_char.*/
  virtual void handle_action(DocAction::Type action, KeyPress key);
  /** A character which is not an action.  This is only called by handle_raw_char. */
  virtual void handle_char(KeyPress key);
  /** Mouse click. */
  virtual void handle_mouse(int row, int col, bool shift, bool ctrl, bool double_click=false);
  /** Select the current word. */
  virtual void handle_select_word();
  /** Jump to this location. Like handle_mouse but no shift and there is a CENTRALIZE trigger.*/
  virtual void handle_jump_to(int row, int col);
  /** Some actions that come directly. */
  virtual void handle_undo();
  virtual std::string handle_copy();
  virtual std::string handle_cut();
  virtual void handle_paste(const std::string& contents);
  virtual void handle_select_all();
  virtual std::string handle_kill();
  virtual void handle_save();
  virtual void handle_save_as(const std::string& path);
  virtual DocSearchResults handle_search_update(const std::string& search_term, SearchSettings search_settings, bool search_back, bool move);
  virtual DocSearchResults handle_search_replace(const std::string& search_term, SearchSettings search_settings, const std::string& replacement);
  virtual int handle_search_replace_all(const std::string& search_term, SearchSettings search_settings, const std::string& replacement);
  virtual void handle_change_type(const std::string& new_type);
  virtual void handle_comment();
  virtual void handle_uncomment();
  virtual void handle_complete(std::vector<std::string>& matches, bool complete_prefix);
  virtual void handle_complete(const std::string& str);
  /** Process is running and producting output. This is only invoked for virtual documents. */
  virtual void handle_console(const std::string& text);
  virtual void handle_escape();
  virtual void handle_fold(bool toggle);

  /** Opportunity for the document to raise an issue with closing. Return true if closing allowed, otherwise false. */
  virtual bool handle_about_to_close();

  // Handy functions

  virtual std::string get_selection_as_string() const;
  virtual void trigger_refresh();
};

/** Hook that activates for every document. */
extern HookSource<Doc*, int> all_docs_hook;

#endif