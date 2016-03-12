#ifndef CORE_TEXT_VIEW_HPP
#define CORE_TEXT_VIEW_HPP

#include "core/common.hpp"

#include <string>

class FlowGrid;
class SimpleTextEdit;
class TextBuffer;
class TextFile;
class TextView;

class SelectionChecker {
private:
  TextView& tv;
public:
  SelectionChecker(TextView& tv0, bool shift);
  ~SelectionChecker();
};

/** Manages cursor, selection. */
class TextView {
friend class SelectionChecker;

private:
  int preferred_x;
  CursorLocation cursor;
  bool selection_active;
  int selection_start_x, selection_start_y;

  TextBuffer& text_buffer;
  TextFile* text_file;

  // Helper functions
  void selection_disappear(SimpleTextEdit&);

  void tab(SimpleTextEdit& ste, int row, int tabdef);
  void untab(SimpleTextEdit& ste, int row, int tabdef);
  void comment(SimpleTextEdit& ste, int row, const std::string& comment);
  void uncomment(SimpleTextEdit& ste, int row, const std::string& comment);

public:
  TextView(TextBuffer& tb, TextFile* tf = nullptr);
  TextView(const TextView&) = delete;
  TextView& operator=(const TextView&) = delete;

  inline CursorLocation get_cursor() const { return cursor; }
  inline bool is_selection_active() const { return selection_active; }

  // Helpers:
  int char_left();
  int char_right();

  // Selection related:
  SelectionInfo get_selection() const;
  std::string selection_as_string() const;
  std::string selection_as_string_first_line() const;
  void selection_move_to_origin();

  // Actions on TextView:
  void cursor_up(bool shift, FlowGrid* fg = nullptr);
  void cursor_down(bool shift, FlowGrid* fg = nullptr);
  void cursor_left(bool shift);
  void cursor_right(bool shift);
  void insert_char(int c);
  void delete_forward();
  void delete_backward();
  void newline();
  void page_up(int page_size, bool shift);
  void page_down(int page_size, bool shift);
  void home(bool shift);
  void end(bool shift);
  void home_file(bool shift);
  void end_file(bool shift);
  void mouse(int row, int col, bool shift);
  std::string cut();
  void paste(const std::string& s);
  std::string copy();
  std::string kill();
  void select_all();
  void select_none();
  void replace(const std::string& term, const std::string& replacement);
  int replace_all(const std::string& term, SearchSettings search_settings, const std::string& replacement);
  void rotating_tab(int index);
  void tab(int tabdef);
  void untab(int tabdef);
  void comment(const std::string& comment);
  void uncomment(const std::string& comment);
  void skip_left(bool shift);
  void skip_right(bool shift);
  void skip_up(bool shift);
  void skip_down(bool shift);
  void delete_word();
  /** Skip to next start of token of this type or document end. */
  void next_token_start(int type);
  /** Skip to previous start of token of this type or document start. */
  void prev_token_start(int type);

  void select_word();
  void select_word_left();
  /** Return identifier at the cursor (if any). */
  std::string identifier();
  /** Return identifier at the cursor only to the left (if any). */
  std::string identifier_left();
  /** Return a "navigable" at the cursor (if any). Navigable is like an identifier but allows
   / and \ and :. */
  std::string navigable();
  void undo();
  /** If the TextBuffer has been modified externally to the TextView class then we
   must let it know. */
  void fix();

  void folded_momentum_down(bool shift);
  void folded_momentum_up(bool shift);
  void break_fold_down();
  void break_fold_up();
  void break_fold(int start_row, int end_row, bool unfold_chain=true, bool fold_punct=true);
  void fold(int start_row, int end_row);
};

#endif