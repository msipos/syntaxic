#ifndef SYNTAXIC_CORE_UNDO_MANAGER_HPP
#define SYNTAXIC_CORE_UNDO_MANAGER_HPP

#include "core/common.hpp"

#include <string>
#include <vector>

class TextFile;

enum UndoElementType {
  UNDO_CHANGE_LINE,
  UNDO_REMOVE_LINE,
  UNDO_INSERT_LINE,
  UNDO_SAVE_POINT
};

class UndoElement {
private:
  UndoElementType type;
  int row;
  std::string line;
  int count;
  CursorLocation cursor_before;
public:
  inline UndoElement(UndoElementType t, int r, std::string l, int c, CursorLocation cl)
    : type(t), row(r), line(l), count(c), cursor_before(cl) {}

  inline int get_count() const { return count; }
  inline const std::string& get_line() const { return line; }
  inline int get_row() const { return row; }
  inline UndoElementType get_type() const { return type; }
  inline CursorLocation get_cursor_before() const { return cursor_before; }
};

class UndoManager {
private:
  TextFile& text_file;
  int counter;
  std::vector<UndoElement> elements;

public:
  UndoManager(TextFile& tf);
  UndoManager(const UndoManager&) = delete;
  UndoManager& operator=(const UndoManager&) = delete;

  /** Returns an identifier that should be used for components of this undo operation. */
  int new_activity();

  /** Call before changing the line. */
  void add_change_line(int id, int line_index, CursorLocation cl);
  /** Call after inserting the line. */
  void add_insert_line(int id, int line_index, CursorLocation cl);
  /** Call before removing the line. */
  void add_remove_line(int id, int line_index, CursorLocation cl);
  /** Call after a save. */
  void add_save_point();

  /** Conduct an undo operation, if any.  If there was any, return true and modify cl. */
  bool undo(CursorLocation& cl);

  /** Return if top of the stack is a save point. */
  bool is_save_point();
};

#endif