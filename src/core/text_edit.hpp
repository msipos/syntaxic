#ifndef CORE_TEXT_EDIT_HPP
#define CORE_TEXT_EDIT_HPP

#include "core/common.hpp"

#include <string>

class TextBuffer;
class TextFile;
class UndoManager;

/** Simple text editing operations, handles undo manager. */
class SimpleTextEdit {
private:
  TextBuffer& text_buffer;
  TextFile* text_file;
  UndoManager* undo_manager;
  int activity;
  CursorLocation start_location, end_location;

public:
  SimpleTextEdit(TextBuffer& tb);
  SimpleTextEdit(TextBuffer& tb, CursorLocation sl, TextFile* tf=nullptr);
  SimpleTextEdit(const SimpleTextEdit&) = delete;
  SimpleTextEdit& operator=(const SimpleTextEdit&) = delete;

  void remove_char(CursorLocation cl);
  void insert_char(CursorLocation cl, char32_t c, uint8_t markup=0);
  void remove_text(CursorLocation cl1, CursorLocation cl2);
  void insert_text(CursorLocation cl, std::string text, uint8_t markup=0);

  inline CursorLocation get_end_location() { return end_location; }
};

#endif