#include "core/text_edit.hpp"
#include "core/text_buffer.hpp"
#include "core/text_file.hpp"
#include "core/undo_manager.hpp"
#include "core/util.hpp"
#include "core/utf8_util.hpp"

SimpleTextEdit::SimpleTextEdit(TextBuffer& tb) : text_buffer(tb), text_file(nullptr), undo_manager(nullptr), start_location(0, 0) {
  if (text_file) {
    undo_manager = &(text_file->get_undo_manager());
    if (undo_manager) activity = undo_manager->new_activity();
  }
}

SimpleTextEdit::SimpleTextEdit(TextBuffer& tb, CursorLocation sl, TextFile* tf) : text_buffer(tb), text_file(tf), undo_manager(nullptr), start_location(sl) {
  if (text_file) {
    undo_manager = &(text_file->get_undo_manager());
    if (undo_manager) activity = undo_manager->new_activity();
  }
}

void SimpleTextEdit::remove_char(CursorLocation cl) {
  if (!text_buffer.check(cl)) {
    printf("ERROR: remove_char invalid cl.\n");
    return;
  }

  Line& l = text_buffer.get_line(cl.row);
  if (cl.col == (int) l.size()) {
    if (text_buffer.get_num_lines() == cl.row - 1) {
      printf("ERROR: remove_char on last char\n");
      return;
    }
    Line& nl = text_buffer.get_line(cl.row+1);
    std::string s = nl.to_string(0, nl.size());
    if (undo_manager) undo_manager->add_remove_line(activity, cl.row+1, start_location);
    text_buffer.remove_line(cl.row+1);
    if (undo_manager) undo_manager->add_change_line(activity, cl.row, start_location);
    l.append(s);
  } else {
    if (undo_manager) undo_manager->add_change_line(activity, cl.row, start_location);
    l.remove(cl.col);
  }
  end_location = cl;
  if (text_file) text_file->unsaved_edits = true;
}

void SimpleTextEdit::insert_char(CursorLocation cl, char32_t c, uint8_t markup) {
  if (!text_buffer.check(cl)) {
    printf("ERROR: insert_char invalid cl.\n");
    return;
  }

  Line& l = text_buffer.get_line(cl.row);
  if (c == '\n') {
    std::string s = "";
    if (cl.col != (int) l.size()) {
      if (undo_manager) undo_manager->add_change_line(activity, cl.row, start_location);
      s = l.to_string(cl.col, l.size());
      l.trim(cl.col);
    }
    Line& nl = text_buffer.insert_line(cl.row+1);
    nl.append(s);
    if (undo_manager) undo_manager->add_insert_line(activity, cl.row+1, start_location);
    end_location.row = cl.row + 1;
    end_location.col = 0;
  } else {
    if (undo_manager) undo_manager->add_change_line(activity, cl.row, start_location);
    l.insert(cl.col, c, markup);
    end_location = cl;
    end_location.col++;
  }
  if (text_file) text_file->unsaved_edits = true;
}

void SimpleTextEdit::remove_text(CursorLocation cl1, CursorLocation cl2) {
  if (!text_buffer.check(cl1)) {
    printf("ERROR: remove_text invalid cl1.\n"); return;
  }
  if (!text_buffer.check(cl2)) {
    printf("ERROR: remove_text invalid cl2.\n"); return;
  }

  if (cl1 == cl2) return;
  if (cl1 > cl2) std::swap(cl1, cl2);

  if (cl1.row == cl2.row) {
    if (undo_manager) undo_manager->add_change_line(activity, cl1.row, start_location);
    Line& l = text_buffer.get_line(cl1.row);
    l.remove(cl1.col, cl2.col);
  } else {
    if (undo_manager) undo_manager->add_change_line(activity, cl1.row, start_location);
    Line& l1 = text_buffer.get_line(cl1.row);
    Line& l2 = text_buffer.get_line(cl2.row);
    l1.remove(cl1.col, l1.size());
    l1.append(l2.to_string(cl2.col, l2.size()));
    for (int row = cl2.row; row > cl1.row; row--) {
      if (undo_manager) undo_manager->add_remove_line(activity, row, start_location);
      text_buffer.remove_line(row);
    }
  }

  end_location = cl1;
  if (text_file) text_file->unsaved_edits = true;
}

void SimpleTextEdit::insert_text(CursorLocation cl, std::string text, uint8_t markup) {
  if (!text_buffer.check(cl)) {
    printf("ERROR: insert_text invalid cl.\n"); return;
  }

  if (text.size() == 0) return;

  std::vector<std::string> splitted;
  utf8_string_split(splitted, text, '\n');
  #ifdef CMAKE_WINDOWS
  for (std::string& s: splitted) {
    utf8_rtrim(s, '\r');
  }
  #endif

  if (splitted.size() == 1) {
    if (undo_manager) undo_manager->add_change_line(activity, cl.row, start_location);
    Line& l = text_buffer.get_line(cl.row);
    const int old_size = l.size();
    l.insert(cl.col, text, markup);
    end_location = CursorLocation(cl.row, cl.col + l.size() - old_size);

  } else if (splitted.size() > 1) {
    if (undo_manager) undo_manager->add_change_line(activity, cl.row, start_location);
    Line& l = text_buffer.get_line(cl.row);
    std::string s = l.to_string(cl.col, l.size());
    l.trim(cl.col);
    l.append(splitted[0], markup);
    for (unsigned int i = 1; i < splitted.size(); i++) {
      Line& nl = text_buffer.insert_line(cl.row + i);
      nl.append(splitted[i], markup);
      end_location = CursorLocation(cl.row + i, nl.size());
      if (i == (splitted.size() - 1)) {
        nl.append(s, markup);
      }
      if (undo_manager) undo_manager->add_insert_line(activity, cl.row + i, start_location);
    }
  }
  if (text_file) text_file->unsaved_edits = true;
}