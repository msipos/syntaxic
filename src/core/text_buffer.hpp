#ifndef SYNTAXIC_CORE_TEXT_BUFFER_HPP
#define SYNTAXIC_CORE_TEXT_BUFFER_HPP

#include "core/common.hpp"
#include "core/line.hpp"
#include "core/word_def.hpp"

#include <string>
#include <vector>

/** A collection of lines. */
class TextBuffer {
protected:
  std::vector<Line> lines;
  inline void append_line(const char* line, int line_len) { lines.push_back(Line(line, line_len)); }

public:
  TextBuffer();

  // Line interface:

  inline Line& get_line(int index) { return lines.at(index); }
  inline const Line& get_line(int index) const { return lines.at(index); }
  Line& get_last_line();
  const Line& get_last_line() const;
  inline int get_num_lines() const { return lines.size(); }
  Line& insert_line(int index);
  void remove_line(int index);
  /** If there are more than _size_ lines, then delete first N lines like a console. */
  void trim_lines_to_size(unsigned int size);
  /** Utility function to take the entire TextFile and return it as a UTF8 string. */
  std::string get_contents_as_string() const;
  inline std::string to_string() const { return get_contents_as_string(); }

  // Cursor helpers:

  bool check(CursorLocation cl) const;
  CursorLocation fix(CursorLocation cl) const;
  CursorLocation check_and_fix(CursorLocation cl) const;
  CursorLocation move_left(CursorLocation cl) const;
  bool can_move_left(CursorLocation cl) const;
  CursorLocation move_right(CursorLocation cl) const;
  bool can_move_right(CursorLocation cl) const;
  Character at_cursor(CursorLocation cl) const;
  int char_left(CursorLocation cl) const;
  int char_right(CursorLocation cl) const;
  /** Return cursor at the end of the buffer. */
  CursorLocation cursor_end_buffer() const;
  /** Return the word at the cursor (if any). In this case, word stands for any number of
  consecutive letters, numbers, _ and $. */
  std::string get_word(CursorLocation cl) const;
  /** Return the word at the cursor (if any). */
  std::string get_word(CursorLocation& cl, WordDef word_def) const;

  // Selection

  /** Return a range of the text. */
  std::string selection_as_string(const SelectionInfo& si) const;

  // Search:

  void search(const std::string& term, std::vector<SearchResult>& results, SearchSettings search_settings) const;
  void search_word(const std::string& term, std::vector<SearchResult>& results) const;

  // To and from UTF8:

  std::string to_utf8(LineEndings le);
  /** Deletes the previous contents of the buffer. */
  void from_utf8(const std::string& s);

  // From another buffer:
  void from_buffer(const TextBuffer& tb);

  // Editing helpers
  void append(const std::string& s, uint8_t markup=0);
};

#endif