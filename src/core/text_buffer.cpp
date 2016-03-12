#include "core/text_buffer.hpp"
#include "core/text_edit.hpp"
#include "utf8.h"

TextBuffer::TextBuffer() {
  lines.push_back(Line());
}

Line& TextBuffer::get_last_line() {
  if (lines.empty()) {
    lines.push_back(Line());
  }
  return lines.back();
}

const Line& TextBuffer::get_last_line() const {
  if (lines.empty()) {
    printf("WARNING: no lines in TextBuffer.\n");
  }
  return lines.back();
}

Line& TextBuffer::insert_line(int index) {
  lines.insert(lines.begin() + index, Line());
  return lines[index];
}

void TextBuffer::remove_line(int index) {
  lines.erase(lines.begin() + index);
}

void TextBuffer::trim_lines_to_size(unsigned int size) {
  if (lines.size() > size) {
    const int num_to_erase = lines.size() - size;
    lines.erase(lines.begin(), lines.begin() + num_to_erase);
  }
}

std::string TextBuffer::get_contents_as_string() const {
  std::string rv;
  for (int i = 0; i < get_num_lines(); i++) {
    const Line& line = get_line(i);
    rv += line.to_string();
    if (i != get_num_lines() - 1) rv += '\n';
  }
  return rv;
}

bool TextBuffer::check(CursorLocation cl) const {
  if (cl.row < 0) return false;
  if (cl.row >= get_num_lines()) return false;
  if (cl.col < 0) return false;
  const Line& l = get_line(cl.row);
  if (cl.col > (int) l.size()) return false;
  return true;
}

CursorLocation TextBuffer::fix(CursorLocation cl) const {
  if (cl.row < 0) cl.row = 0;
  if (cl.row >= get_num_lines()) cl.row = get_num_lines()-1;
  if (cl.col < 0) cl.col = 0;
  const Line& l = get_line(cl.row);
  if (cl.col > (int) l.size()) cl.col = l.size();
  return cl;
}

CursorLocation TextBuffer::check_and_fix(CursorLocation cl) const {
  if (!check(cl)) {
    printf("ERROR: Should not happen cl = %d, %d. Repairing.\n", cl.row, cl.col);
  }
  return fix(cl);
}

bool TextBuffer::can_move_left(CursorLocation cl) const {
  if (cl.col == 0 && cl.row == 0) return false;
  return true;
}

bool TextBuffer::can_move_right(CursorLocation cl) const {
  if (cl.row == get_num_lines() - 1) {
    const Line& l = get_line(cl.row);
    if (cl.col == (int) l.size()) return false;
  } else if (cl.row >= get_num_lines()) {
    return false;
  }
  return true;
}

CursorLocation TextBuffer::move_left(CursorLocation cl) const {
  cl = check_and_fix(cl);

  if (cl.col > 0) {
    cl.col--;
    return cl;
  } else {
    if (cl.row > 0) {
      cl.row--;
      const Line& l = get_line(cl.row);
      cl.col = l.size();
      return cl;
    } else {
      return cl;
    }
  }
}

CursorLocation TextBuffer::move_right(CursorLocation cl) const {
  cl = check_and_fix(cl);

  const Line& l = get_line(cl.row);
  if (cl.col < (int) l.size()) {
    cl.col++;
    return cl;
  } else {
    if (cl.row < get_num_lines() - 1) {
      cl.row++;
      cl.col = 0;
      return cl;
    } else {
      return cl;
    }
  }
}

Character TextBuffer::at_cursor(CursorLocation cl) const {
  cl = check_and_fix(cl);
  const Line& l = get_line(cl.row);
  if (cl.col == (int) l.size()) return Character('\n');
  else {
    return l.get_char(cl.col);
  }
}

int TextBuffer::char_left(CursorLocation cl) const {
  if (!can_move_left(cl)) return -1;
  cl = move_left(cl);
  return at_cursor(cl).c;
}

int TextBuffer::char_right(CursorLocation cl) const {
  if (!can_move_right(cl)) return -1;
  return at_cursor(cl).c;
}

inline bool word_char(uint32_t c) {
  if (isalnum(c)) return true;
  if (c == '_') return true;
  if (c == '$') return true;
  return false;
}

CursorLocation TextBuffer::cursor_end_buffer() const {
  const Line& l = get_last_line();
  return CursorLocation(get_num_lines()-1, l.size());
}

std::string TextBuffer::get_word(CursorLocation cursor) const {
  const Line& line = get_line(cursor.row);

  int left_x = cursor.col;
  while (left_x > 0) {
    left_x--;
    int c = line.get_char(left_x).c;
    if (!word_char(c)) {
      left_x++;
      break;
    }
  }
  int right_x = cursor.col;
  while (right_x < (int) line.size()) {
    int c = line.get_char(right_x).c;
    if (word_char(c)) right_x++;
    else break;
  }
  if (left_x == right_x) return "";
  return line.to_string(left_x, right_x);
}

std::string TextBuffer::get_word(CursorLocation& cursor, WordDef word_def) const {
  const Line& line = get_line(cursor.row);

  int left_x = 0;
  bool in_word = false;
  int p = 0;
  for (unsigned int i = 0; i < line.size(); i++) {
    int c = line.get_char(i).c;
    if (in_word) {
      if (!word_def.continue_word(p, c)) {
        in_word = false;
        int right_x = int(i);
        if (left_x <= cursor.col && right_x >= cursor.col) {
          return line.to_string(left_x, right_x);
        }
      }
    } else {
      if (word_def.start_word(c)) {
        left_x = i;
        in_word = true;
        if (left_x > cursor.col) return "";
      }
    }
  }

  if (in_word) {
    int right_x = int(line.size());
    if (left_x <= cursor.col && right_x >= cursor.col) {
      return line.to_string(left_x, right_x);
    }
  }
  return "";
}


void TextBuffer::search(const std::string& term, std::vector<SearchResult>& results, SearchSettings search_settings) const {
  int i = 0;
  for (const Line& line : lines) {
    line.search(term, results, i, search_settings.word, search_settings.case_insensitive);
    i++;
  }
}

void TextBuffer::search_word(const std::string& term, std::vector<SearchResult>& results) const {
  int i = 0;
  for (const Line& line : lines) {
    line.search(term, results, i, true);
    i++;
  }
}


std::string TextBuffer::to_utf8(LineEndings line_endings) {
  std::string rv;
  for (int i = 0; i < get_num_lines(); i++) {
    const Line& line = get_line(i);
    rv.append(line.to_string());
    if (i != get_num_lines() - 1) {
      if (line_endings == WINDOWS) rv += '\r';
      rv += '\n';
    }
  }
  return rv;
}


void TextBuffer::from_utf8(const std::string& s) {
  lines.clear();
  lines.push_back(Line());
  const char* cur = s.c_str();
  const char* end = s.c_str() + s.size();

  while (cur != end) {
    uint32_t c = utf8::next(cur, end);
    if (c == '\n') {
      lines.back().trim_r();
      lines.push_back(Line());
    } else {
      lines.back().append(c);
    }
  }
}

void TextBuffer::from_buffer(const TextBuffer& tb) {
  lines.clear();
  for (int i = 0; i < tb.get_num_lines(); i++) {
    Line line;
    line.from_line(tb.get_line(i));
    lines.push_back(std::move(line));
  }
}

void TextBuffer::append(const std::string& s, uint8_t markup) {
  SimpleTextEdit ste(*this);
  ste.insert_text(cursor_end_buffer(), s, markup);
}

std::string TextBuffer::selection_as_string(const SelectionInfo& si) const {
  if (!si.active) return "";

  std::string buffer;
  if (si.row_start == si.row_end) {
    const Line& line = get_line(si.row_start);
    buffer += line.to_string(si.col_start, si.col_end);
  } else {
    const Line& first_line = get_line(si.row_start);
    buffer += first_line.to_string(si.col_start, first_line.size());
    buffer += '\n';
    for (int i = si.row_start+1; i < si.row_end; i++) {
      const Line& line = get_line(i);
      buffer += line.to_string(0, line.size());
      buffer += '\n';
    }
    const Line& last_line = get_line(si.row_end);
    buffer += last_line.to_string(0, si.col_end);
  }
  return buffer;
}