#include "core/flow_grid.hpp"
#include "core/line.hpp"
#include "core/text_edit.hpp"
#include "core/text_buffer.hpp"
#include "core/text_file.hpp"
#include "core/text_view.hpp"
#include "core/utf8_util.hpp"

TextView::TextView(TextBuffer& tb, TextFile* tf) : preferred_x(0), cursor(0, 0), selection_active(false), text_buffer(tb), text_file(tf) {}

SelectionInfo TextView::get_selection() const {
  SelectionInfo si;
  si.active = selection_active;
  if (!selection_active) return si;

  if (selection_start_y < cursor.row) {
    si.row_start = selection_start_y;
    si.col_start = selection_start_x;
    si.row_end = cursor.row;
    si.col_end = cursor.col;
    si.forwards = true;
  } else {
    si.row_start = cursor.row;
    si.col_start = cursor.col;
    si.row_end = selection_start_y;
    si.col_end = selection_start_x;
    si.forwards = false;
  }
  if (si.row_start == si.row_end) {
    if (si.col_start > si.col_end) {
      std::swap(si.col_start, si.col_end);
      si.forwards = false;
    } else {
      si.forwards = true;
    }
  }

  si.lines_end = si.row_end;
  if (si.col_end == 0) si.lines_end--;
  // TODO: lines_start++ if end of line.
  si.lines_start = si.row_start;

  return si;
}

SelectionChecker::SelectionChecker(TextView& tv0, bool shift) : tv(tv0) {
  if (tv.selection_active && !shift) tv.selection_active = false;
  if (shift && !tv.selection_active) {
    tv.selection_start_x = tv.get_cursor().col;
    tv.selection_start_y = tv.get_cursor().row;
    tv.selection_active = true;
  }
}

SelectionChecker::~SelectionChecker() {
  if (tv.selection_active) {
    if (tv.selection_start_x == tv.get_cursor().col
        && tv.selection_start_y == tv.get_cursor().row) {
      tv.selection_active = false;
    }
  }
}

void TextView::selection_disappear(SimpleTextEdit& ste) {
  SelectionInfo si = get_selection();
  ste.remove_text(CursorLocation(si.row_start, si.col_start),
                  CursorLocation(si.row_end, si.col_end));
  cursor = ste.get_end_location();
  preferred_x = cursor.col;
  selection_active = false;
}

std::string TextView::selection_as_string() const {
  if (!selection_active) return "";

  SelectionInfo si = get_selection();
  return text_buffer.selection_as_string(si);
}

std::string TextView::selection_as_string_first_line() const {
  if (!selection_active) return "";
  SelectionInfo si = get_selection();
  if (si.row_start == si.row_end) {
    Line& line = text_buffer.get_line(si.row_start);
    return line.to_string(si.col_start, si.col_end);
  } else {
    Line& first_line = text_buffer.get_line(si.row_start);
    return first_line.to_string(si.col_start, first_line.size());
  }
}

void TextView::selection_move_to_origin() {
  if (!selection_active) return;

  SelectionInfo si = get_selection();
  // We are already at the beginning?
  if (si.row_start == cursor.row && si.col_start == cursor.col) return;

  // We must be at the end.
  selection_start_x = cursor.row;
  selection_start_y = cursor.col;
  cursor.row = si.row_start;
  cursor.col = si.col_start;
}

void TextView::cursor_up(bool shift, FlowGrid* flow_grid) {
  SelectionChecker sc(*this, shift);

  int pixel_x = 0;
  if (flow_grid != nullptr) {
    pixel_x = flow_grid->map_to_x(cursor.row, cursor.col);
    const RowInfo ri = flow_grid->get_row_info(cursor.row);
    if (ri.num_effective_rows > 1) {
      const int er = flow_grid->get_effective_row(cursor.row, cursor.col);
      if (er > 0) {
        cursor.col = flow_grid->get_row_index_of_effective_row(pixel_x, cursor.row, er-1);
        preferred_x = cursor.col;
        return;
      }
    }
  }

  cursor.row--;
  if (cursor.row < 0) {
    cursor.row = 0;
    cursor.col = 0;
    preferred_x = cursor.col;
  } else {
    if (flow_grid != nullptr) {
      const RowInfo ri = flow_grid->get_row_info(cursor.row);
      if (ri.num_effective_rows > 1) {
        cursor.col = flow_grid->get_row_index_of_effective_row(pixel_x, cursor.row, ri.num_effective_rows - 1);
        preferred_x = cursor.col;
        return;
      }
    }
    Line& line = text_buffer.get_line(cursor.row);
    const int size = line.size();
    cursor.col = preferred_x;
    if (cursor.col > size) cursor.col = size;
  }
}

void TextView::cursor_down(bool shift, FlowGrid* flow_grid) {
  SelectionChecker sc(*this, shift);

  int pixel_x = 0;
  if (flow_grid != nullptr) {
    pixel_x = flow_grid->map_to_x(cursor.row, cursor.col);
    const RowInfo ri = flow_grid->get_row_info(cursor.row);
    if (ri.num_effective_rows > 1) {
      const int er = flow_grid->get_effective_row(cursor.row, cursor.col);
      if (er < ri.num_effective_rows - 1) {
        cursor.col = flow_grid->get_row_index_of_effective_row(pixel_x, cursor.row, er+1);
        return;
      }
    }
  }

  cursor.row++;
  const int num_lines = text_buffer.get_num_lines();
  if (cursor.row >= num_lines) {
    cursor.row = num_lines - 1;
    Line& line = text_buffer.get_line(cursor.row);
    cursor.col = line.size();
    preferred_x = cursor.col;
  } else {

    if (flow_grid != nullptr) {
      const RowInfo ri = flow_grid->get_row_info(cursor.row);
      if (ri.num_effective_rows > 1) {
        cursor.col = flow_grid->get_row_index_of_effective_row(pixel_x, cursor.row, 0);
        preferred_x = cursor.col;
        return;
      }
    }

    Line& line = text_buffer.get_line(cursor.row);
    const int size = line.size();
    cursor.col = preferred_x;
    if (cursor.col > size) cursor.col = size;
  }
}

void TextView::cursor_left(bool shift) {
  SelectionChecker sc(*this, shift);
  cursor = text_buffer.move_left(cursor);
  preferred_x = cursor.col;
}

void TextView::cursor_right(bool shift) {
  SelectionChecker sc(*this, shift);
  cursor = text_buffer.move_right(cursor);
  preferred_x = cursor.col;
}

int TextView::char_left() {
  if (!text_buffer.can_move_left(cursor)) return -1;
  CursorLocation cl = text_buffer.move_left(cursor);
  return text_buffer.at_cursor(cl).c;
}

int TextView::char_right() {
  if (!text_buffer.can_move_right(cursor)) return -1;
  return text_buffer.at_cursor(cursor).c;
}

void TextView::skip_left(bool shift) {
  bool entered_alpha = false;
  for (;;) {
    int c = char_left();
    if (c == -1) break;
    if (isalnum(c)) entered_alpha = true;
    else {
      if (entered_alpha) break;
    }
    cursor_left(shift);
  }
}

void TextView::skip_right(bool shift) {
  bool entered_alpha = false;
  for (;;) {
    int c = char_right();
    if (c == -1) break;
    if (isalnum(c)) entered_alpha = true;
    else {
      if (entered_alpha) break;
    }
    cursor_right(shift);
  }
}

void TextView::skip_up(bool shift) {
  bool entered_para = false;
  int row = cursor.row;
  cursor.col = 0;
  while (row > 0) {
    Line& line = text_buffer.get_line(row);
    if (line.is_whitespace()) {
      if (entered_para) break;
    } else {
      entered_para = true;
    }
    row--;
    cursor_up(shift);
  }
}

void TextView::skip_down(bool shift) {
  bool entered_para = false;
  int row = cursor.row;
  cursor.col = 0;
  while (row < text_buffer.get_num_lines() - 1) {
    Line& line = text_buffer.get_line(row);
    if (line.is_whitespace()) {
      if (entered_para) break;
    } else {
      entered_para = true;
    }
    row++;
    cursor_down(shift);
  }
}

void TextView::delete_word() {
  bool entered_alpha = false;
  CursorLocation cl1 = cursor;
  CursorLocation cl2 = cursor;
  for (;;) {
    int c = text_buffer.char_left(cl1);
    if (c == -1) break;
    if (isalnum(c)) entered_alpha = true;
    else {
      if (entered_alpha) break;
    }
    cl1 = text_buffer.move_left(cl1);
  }

  if (!(cl1 == cl2)) {
    SimpleTextEdit ste(text_buffer, cursor, text_file);
    ste.remove_text(cl1, cl2);
  }
  cursor = cl1;
}

void TextView::next_token_start(int type) {
  int row = cursor.row;
  int col = cursor.col;

  while (row < text_buffer.get_num_lines()) {
    Line& line = text_buffer.get_line(row);
    std::vector<Token>& tokens = line.appendage().tokens;
    for (Token& t : tokens) {
      if (t.get_type() == type && t.is_start()) {
        if (t.offset > col) {
          cursor.row = row;
          cursor.col = t.offset;
          return;
        }
      }
    }
    row++;
    col = -1;
  }
}

void TextView::prev_token_start(int type) {
  int row = cursor.row;
  int col = cursor.col;

  while (row >= 0) {
    Line& line = text_buffer.get_line(row);
    std::vector<Token>& tokens = line.appendage().tokens;
    for (int i = tokens.size() - 1; i >= 0; i--) {
      Token& t = tokens[i];
      if (t.get_type() == type && t.is_start()) {
        if (t.offset < col) {
          cursor.row = row;
          cursor.col = t.offset;
          return;
        }
      }
    }
    row--;
    col = 10000000;
  }
}

void TextView::insert_char(int c) {
  SimpleTextEdit ste(text_buffer, cursor, text_file);

  if (selection_active) selection_disappear(ste);
  ste.insert_char(cursor, c);
  cursor = ste.get_end_location();

  // TODO: Indentation should be in settings.
  if (c == '\n') {
    // Copy indentation from line above.
    int above_row = cursor.row - 1;
    if (above_row >= 0) {
      Line& line = text_buffer.get_line(above_row);
      for (unsigned int i = 0; i < line.size(); i++) {
        uint32_t c = line.get_char(i).c;
        if (c == ' ') ste.insert_char(cursor, c);
        else if (c == '\t') ste.insert_char(cursor, c);
        else break;
        cursor = ste.get_end_location();
      }
    }
  }

  preferred_x = cursor.col;
}

void TextView::delete_forward() {
  if (selection_active) {
    SimpleTextEdit ste(text_buffer, cursor, text_file);
    selection_disappear(ste);
    return;
  }

  if (!text_buffer.can_move_right(cursor)) return;

  SimpleTextEdit ste(text_buffer, cursor, text_file);

  ste.remove_char(cursor);
  cursor = ste.get_end_location();
  preferred_x = cursor.col;
}

void TextView::delete_backward() {
  if (selection_active) {
    SimpleTextEdit ste(text_buffer, cursor, text_file);
    selection_disappear(ste);
    return;
  }
  if (!text_buffer.can_move_left(cursor)) return;

  SimpleTextEdit ste(text_buffer, cursor, text_file);
  CursorLocation cl = text_buffer.move_left(cursor);
  ste.remove_char(cl);
  cursor = ste.get_end_location();
  preferred_x = cursor.col;
}

void TextView::newline() {
  insert_char('\n');
}

void TextView::page_up(int page_size, bool shift) {
  for (int i = 0; i < page_size; i++) {
    cursor_up(shift);
  }
}

void TextView::page_down(int page_size, bool shift) {
  for (int i = 0; i < page_size; i++) {
    cursor_down(shift);
  }
}

void TextView::home(bool shift) {
  SelectionChecker sc(*this, shift);

  Line& cur_line = text_buffer.get_line(cursor.row);
  int start = cur_line.get_start();
  if (cursor.col == start) {
    cursor.col = 0;
  } else {
    cursor.col = start;
  }
  preferred_x = cursor.col;
}

void TextView::home_file(bool shift) {
  SelectionChecker sc(*this, shift);

  cursor.row = 0;
  cursor.col = 0;
  preferred_x = 0;
}

void TextView::end_file(bool shift) {
  SelectionChecker sc(*this, shift);

  cursor.row = text_buffer.get_num_lines() - 1;
  Line& line = text_buffer.get_line(cursor.row);
  cursor.col = line.size();
  preferred_x = cursor.col;
}

void TextView::end(bool shift) {
  SelectionChecker sc(*this, shift);

  Line& cur_line = text_buffer.get_line(cursor.row);
  int end = cur_line.get_end();
  if (cursor.col == end) {
    cursor.col = cur_line.size();
  } else {
    cursor.col = end;
  }
  preferred_x = cursor.col;
}

void TextView::mouse(int row, int col, bool shift) {
  SelectionChecker sc(*this, shift);

  int new_cursor_y = row;
  if (new_cursor_y >= text_buffer.get_num_lines()) {
    new_cursor_y = text_buffer.get_num_lines() - 1;
  }
  if (new_cursor_y < 0) new_cursor_y = 0;
  cursor.row = new_cursor_y;
  Line& line = text_buffer.get_line(cursor.row);
  int new_cursor_x = col;
  if (new_cursor_x > (int) line.size()) {
    new_cursor_x = line.size();
  }
  if (new_cursor_x < 0) new_cursor_x = 0;
  cursor.col = new_cursor_x;
  preferred_x = cursor.col;
}

std::string TextView::cut() {
  if (!selection_active) return "";

  std::string sel = selection_as_string();
  SimpleTextEdit ste(text_buffer, cursor, text_file);
  selection_disappear(ste);
  return sel;
}

std::string TextView::copy() {
  if (!selection_active) return "";

  return selection_as_string();
}

void TextView::paste(const std::string& txt) {
  SimpleTextEdit ste(text_buffer, cursor, text_file);

  if (selection_active) {
    selection_disappear(ste);
  }
  ste.insert_text(cursor, txt);
  cursor = ste.get_end_location();
}

std::string TextView::kill() {
  if (selection_active) return cut();
  if (!text_buffer.can_move_right(cursor)) return "";

  SimpleTextEdit ste(text_buffer, cursor, text_file);
  Line& line = text_buffer.get_line(cursor.row);
  // 2 possibilities:
  if (cursor.col >= int(line.size())) {
    // end of line, then bring next line over here
    cursor.col = line.size();
    ste.remove_char(cursor);
    cursor = ste.get_end_location();
    preferred_x = cursor.col;
    Line& l = text_buffer.get_line(cursor.row);
    return "\n";
  } else {
    // middle of line, trim remainder of line
    CursorLocation cl2 = cursor;
    cl2.col = line.size();
    std::string text = line.to_string(cursor.col, line.size());
    ste.remove_text(cursor, cl2);
    return text;
  }

}

void TextView::select_all() {
  home_file(false);
  end_file(true);
}

void TextView::select_none() {
  selection_active = false;
}

void TextView::replace(const std::string& term, const std::string& replacement) {
  SimpleTextEdit ste(text_buffer, cursor, text_file);
  int size = utf8_size(term);
  CursorLocation cl2 = cursor;
  for (int i = 0; i < size; i++) {
    cl2 = text_buffer.move_right(cl2);
  }
  ste.remove_text(cursor, cl2);
  ste.insert_text(cursor, replacement);
}

int TextView::replace_all(const std::string& term, SearchSettings search_settings, const std::string& replacement) {
  std::vector<SearchResult> results;
  text_buffer.search(term, results, search_settings);

  SimpleTextEdit ste(text_buffer, cursor, text_file);
  for (int i = results.size() - 1; i >= 0; i--) {
    SearchResult sr = results[i];
    CursorLocation cl1(sr.row, sr.col);
    CursorLocation cl2 = cl1;
    for (int i = 0; i < sr.size; i++) {
      cl2 = text_buffer.move_right(cl2);
    }
    ste.remove_text(cl1, cl2);
    ste.insert_text(cl1, replacement);
  }
  mouse(cursor.row, cursor.col, false);
  return results.size();
}

void TextView::select_word() {
  Line& line = text_buffer.get_line(cursor.row);

  int left_x = cursor.col;
  while (left_x > 0) {
    left_x--;
    int c = line.get_char(left_x).c;
    if ((!isalnum(c)) && (c != '_')) {
      left_x++;
      break;
    }
  }
  int right_x = cursor.col;
  while (right_x < (int) line.size()) {
    int c = line.get_char(right_x).c;
    if (isalnum(c) || c == '_') right_x++;
    else break;
  }
  if (left_x != right_x) {
    selection_active = true;
    selection_start_x = left_x;
    selection_start_y = cursor.row;
    cursor.col = right_x;
  }
}

void TextView::select_word_left() {
  Line& line = text_buffer.get_line(cursor.row);

  int left_x = cursor.col;
  while (left_x > 0) {
    left_x--;
    int c = line.get_char(left_x).c;
    if ((!isalpha(c)) && (c != '_')) {
      left_x++;
      break;
    }
  }
  int right_x = cursor.col;
  if (left_x != right_x) {
    selection_active = true;
    selection_start_x = left_x;
    selection_start_y = cursor.row;
    cursor.col = right_x;
  }
}

std::string TextView::identifier() {
  Line& line = text_buffer.get_line(cursor.row);

  int left_x = cursor.col;
  while (left_x > 0) {
    left_x--;
    int c = line.get_char(left_x).c;
    if ((!isalpha(c)) && (c != '_')) {
      left_x++;
      break;
    }
  }
  int right_x = cursor.col;
  while (right_x < (int) line.size()) {
    int c = line.get_char(right_x).c;
    if (isalpha(c) || c == '_') right_x++;
    else break;
  }
  if (left_x == right_x) return "";
  return line.to_string(left_x, right_x);
}

std::string TextView::identifier_left() {
  Line& line = text_buffer.get_line(cursor.row);

  int left_x = cursor.col;
  while (left_x > 0) {
    left_x--;
    int c = line.get_char(left_x).c;
    if ((!isalpha(c)) && (c != '_')) {
      left_x++;
      break;
    }
  }
  int right_x = cursor.col;
  if (left_x == right_x) return "";
  return line.to_string(left_x, right_x);
}

bool navigable_char(char c) {
  if (isalnum(c)) return true;
  switch (c) {
    case '_':
    case ':':
    case '.':
    case '/':
    case '\\':
      return true;
  }
  return false;
}

std::string TextView::navigable() {
  Line& line = text_buffer.get_line(cursor.row);

  int left_x = cursor.col;
  while (left_x > 0) {
    left_x--;
    int c = line.get_char(left_x).c;
    if (!navigable_char(c)) {
      left_x++;
      break;
    }
  }
  int right_x = cursor.col;
  while (right_x < (int) line.size()) {
    int c = line.get_char(right_x).c;
    if (navigable_char(c)) right_x++;
    else break;
  }
  if (left_x == right_x) return "";
  return line.to_string(left_x, right_x);
}

void TextView::rotating_tab(int tab_index) {
  //Indentation ind;
  Line& line = text_buffer.get_line(cursor.row);
  //int delta = line.xindex_to_delta(cursor.col);
  //if (line.is_whitespace()) delta = 0;
  //if (delta < 0) delta = 0;

  if (cursor.row > 0) {
    Line& prev_line = text_buffer.get_line(cursor.row - 1);

    //prev_line.get_indentation(ind);
    //if (ind.num_spaces > 0) {
      tab_index = tab_index % 3;
      //if (tab_index == 0) ind.num_spaces += 2;
      //if (tab_index == 2) ind.num_spaces -= 2;
      //if (ind.num_spaces < 0) ind.num_spaces = 0;
      //line.apply_indentation(ind);
      //cursor.col = line.delta_to_xindex(delta);
      return;
    //}
  }

  tab_index = tab_index % 2;
  //if (tab_index == 0) ind.num_spaces = 2;
  //else ind.num_spaces = 0;
  //line.apply_indentation(ind);
  //cursor.col = line.delta_to_xindex(delta);
}

void TextView::tab(SimpleTextEdit& ste, int row, int tabdef) {
  CursorLocation cl(row, 0);
  if (tabdef < 0) {
    ste.insert_char(cl, '\t');
    if (row == cursor.row) cursor.col++;
  } else {
    Line& line = text_buffer.get_line(row);
    Indentation ind = line.get_indentation();
    const int cur_tab = ind.num_spaces / tabdef;
    int next_spaces = (cur_tab+1) * tabdef;
    for (int i = 0; i < next_spaces - ind.num_spaces; i++) {
      ste.insert_char(cl, ' ');
      if (row == cursor.row) cursor.col++;
    }
  }
}

void TextView::untab(SimpleTextEdit& ste, int row, int tabdef) {
  Line& line = text_buffer.get_line(row);
  CursorLocation cl(row, 0);
  if (tabdef < 0) {
    tabdef *= -1;
    if (line.get_char(0).c == '\t') {
      ste.remove_char(cl);
      if (row == cursor.row && cursor.col > 0) cursor.col--;
      return;
    }
  } else {
    Indentation ind = line.get_indentation();
    int to_remove = ind.num_spaces % tabdef;
    if (to_remove == 0) to_remove = tabdef;
    for (int i = 0; i < to_remove; i++) {
      if (line.size() == 0) break;
      if (i == 0 && line.get_char(0).c == '\t') {
        ste.remove_char(cl);
        if (row == cursor.row && cursor.col > 0) cursor.col--;
        break;
      } else if (line.get_char(0).c == ' ') ste.remove_char(cl);
      else break;
      if (row == cursor.row && cursor.col > 0) cursor.col--;
    }
  }
}

void TextView::tab(int tabdef) {
  SimpleTextEdit ste(text_buffer, cursor, text_file);

  if (selection_active) {
    SelectionInfo si = get_selection();
    for (int row = si.lines_start; row <= si.lines_end; row++) {
      tab(ste, row, tabdef);
    }
  } else {
    tab(ste, cursor.row, tabdef);
  }
}

void TextView::untab(int tabdef) {
  SimpleTextEdit ste(text_buffer, cursor, text_file);

  if (selection_active) {
    SelectionInfo si = get_selection();
    for (int row = si.lines_start; row <= si.lines_end; row++) {
      untab(ste, row, tabdef);
    }
  } else {
    untab(ste, cursor.row, tabdef);
  }
}

void TextView::comment(SimpleTextEdit& ste, int row, const std::string& comment) {
  CursorLocation cl(row, 0);
  ste.insert_text(cl, comment);
  if (row == cursor.row) cursor.col += utf8_size(comment);
}

void TextView::uncomment(SimpleTextEdit& ste, int row, const std::string& comment) {
  Line& line = text_buffer.get_line(row);
  int comment_size = utf8_size(comment);
  const char* ccomment = comment.c_str();
  int j = 0;
  for (unsigned int i = 0; i < line.size(); i++) {
    if (j >= comment_size) return;
    uint32_t c = line.get_char(i).c;
    if (c == ' ' || c == '\t') continue;
    uint32_t cj = utf8_get(ccomment + utf8_count(ccomment, j));
    if (c == cj) {
      CursorLocation cl(row, i);
      ste.remove_char(cl);
      if (row == cursor.row && cursor.col > 0) cursor.col--;
      j++;
      i--; // We just removed a character.
      continue;
    }
    return;
  }
}

void TextView::comment(const std::string& com) {
  SimpleTextEdit ste(text_buffer, cursor, text_file);

  if (selection_active) {
    SelectionInfo si = get_selection();
    for (int row = si.lines_start; row <= si.lines_end; row++) {
      comment(ste, row, com);
    }
  } else {
    comment(ste, cursor.row, com);
  }
}

void TextView::uncomment(const std::string& com) {
  SimpleTextEdit ste(text_buffer, cursor, text_file);

  if (selection_active) {
    SelectionInfo si = get_selection();
    for (int row = si.lines_start; row <= si.lines_end; row++) {
      uncomment(ste, row, com);
    }
  } else {
    uncomment(ste, cursor.row, com);
  }
}

void TextView::fix() {
  if (text_buffer.get_num_lines() == 0) {
    text_buffer.insert_line(0);
  }

  if (cursor.row < 0) {
    cursor.row = 0;
    cursor.col = 0;
    return;
  }
  if (cursor.row >= text_buffer.get_num_lines()) {
    cursor.row = text_buffer.get_num_lines() - 1;
  }
  Line& line = text_buffer.get_line(cursor.row);
  if (cursor.col < 0) cursor.col = 0;
  if (cursor.col > int(line.size())) cursor.col = line.size();
}

void TextView::undo() {
  CursorLocation cl;
  if (text_file && text_file->get_undo_manager().undo(cl)) {
    cursor = cl;
  }
}

void TextView::folded_momentum_down(bool shift) {
  while (cursor.row < text_buffer.get_num_lines() - 1) {
    Line& line = text_buffer.get_line(cursor.row);
    if (!line.appendage().folded) return;
    cursor_down(shift);
  }
  if (cursor.row == text_buffer.get_num_lines() - 1) {
    Line& line = text_buffer.get_line(cursor.row);
    line.appendage().folded = false;
  }
}

void TextView::folded_momentum_up(bool shift) {
  while (cursor.row > 0) {
    Line& line = text_buffer.get_line(cursor.row);
    if (!line.appendage().folded) return;
    cursor_up(shift);
  }
  if (cursor.row == 0) {
    Line& line = text_buffer.get_line(cursor.row);
    line.appendage().folded = false;
  }
}

void TextView::break_fold_down() {
  cursor_down(false);
  CursorLocation cl = cursor;
  {
    Line& line = text_buffer.get_line(cursor.row);
    if (!line.appendage().folded) return;
  }
  const int start_row = cursor.row;
  int end_row = start_row;
  while (cursor.row < text_buffer.get_num_lines() - 1) {
    Line& line = text_buffer.get_line(cursor.row);
    if (!line.appendage().folded) break;
    end_row = cursor.row;
    cursor_down(false);
  }
  break_fold(start_row, end_row);

  // Always break fold here
  Line& line = text_buffer.get_line(cursor.row);
  line.appendage().folded = false;

  cursor = cl;
  folded_momentum_down(false);
}

void TextView::break_fold_up() {
  cursor_up(false);
  CursorLocation cl = cursor;
  {
    Line& line = text_buffer.get_line(cursor.row);
    if (!line.appendage().folded) return;
  }
  const int start_row = cursor.row;
  int end_row = start_row;
  while (cursor.row > 0) {
    Line& line = text_buffer.get_line(cursor.row);
    if (!line.appendage().folded) break;
    end_row = cursor.row;
    cursor_up(false);
  }
  break_fold(end_row, start_row);

  // Always break fold at this point
  Line& line = text_buffer.get_line(cursor.row);
  line.appendage().folded = false;

  cursor = cl;
  folded_momentum_up(false);
}

void TextView::break_fold(int start_row, int end_row, bool unfold_chain, bool fold_punct) {
  if (start_row == end_row) {
    Line& l = text_buffer.get_line(start_row);
    l.appendage().folded = false;
    return;
  }

  // Find min and next levels
  int min_level = 10000, next_level = 10000;
  for (int i = start_row; i <= end_row; i++) {
    Line& l = text_buffer.get_line(i);
    if (l.is_whitespace()) continue;
    if (fold_punct && l.is_non_word()) continue;
    int indent = l.get_start();
    if (indent < min_level) {
      next_level = min_level;
      min_level = indent;
    } else if (indent < next_level && indent != min_level) {
      next_level = indent;
    }
  }

  // Unfold min level
  for (int i = start_row; i <= end_row; i++) {
    Line& l = text_buffer.get_line(i);
    if (l.is_whitespace()) continue;
    int indent = l.get_start();
    if (indent < min_level) {
      l.appendage().folded = false;
    } else if (indent == min_level) {
      if (fold_punct) {
        if (!l.is_non_word()) {
          l.appendage().folded = false;
        }
      } else {
        l.appendage().folded = false;
      }
    }
  }

  // Unfold all whitespace
  for (int i = start_row; i <= end_row; i++) {
    Line& l = text_buffer.get_line(i);
    if (l.is_whitespace()) l.appendage().folded = false;
  }

  // Fold whitespace if it is next to a folded line.
  bool prev_folded = false;
  for (int i = start_row; i <= end_row; i++) {
    Line& l = text_buffer.get_line(i);
    if (l.is_whitespace() && prev_folded) l.appendage().folded = true;
    prev_folded = l.appendage().folded;
  }
  prev_folded = false;
  for (int i = end_row; i >= start_row; i--) {
    Line& l = text_buffer.get_line(i);
    if (l.is_whitespace() && prev_folded) l.appendage().folded = true;
    prev_folded = l.appendage().folded;
  }

  // Finally, unfold all small blocks (less than n lines)
  if (!unfold_chain) return;
  int chain = 0;
  for (int i = start_row; i <= end_row; i++) {
    Line& l = text_buffer.get_line(i);
    if (l.appendage().folded) {
      chain++;
    } else {
      if (chain > 0 && chain < 3) {
        for (int j = i - chain; j < i; j++) {
          Line& l2 = text_buffer.get_line(j);
          l2.appendage().folded = false;
        }
        chain = 0;
      }
    }
  }
}

void TextView::fold(int start_row, int end_row) {
  for (int i = start_row; i <= end_row; i++) {
    Line& l = text_buffer.get_line(i);
    l.appendage().folded = true;
  }
  break_fold(start_row, end_row, false);

  // Unfold current line
  for(;;) {
    Line& curl = text_buffer.get_line(cursor.row);

    if (curl.appendage().folded) {

      // If current line is empty, then just unfold it
      if (curl.is_whitespace()) {
        curl.appendage().folded = false;
        break;
      }

      // Find the folded region
      start_row = cursor.row;
      while (start_row > 0) {
        Line& l = text_buffer.get_line(start_row-1);
        if (!l.appendage().folded) break;
        start_row--;
      }

      end_row = cursor.row;
      while (end_row < text_buffer.get_num_lines() - 1) {
        Line& l = text_buffer.get_line(end_row+1);
        if (!l.appendage().folded) break;
        end_row++;
      }

      break_fold(start_row, end_row, false);
    } else {
      break;
    }
  }
}