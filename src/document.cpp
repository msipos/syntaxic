#include "core/hooks.hpp"
#include "core/util.hpp"
#include "core/utf8_util.hpp"
#include "document.hpp"
#include "master.hpp"
#include "master_io_provider.hpp"
#include "master_navigators.hpp"
#include "uiwindow.hpp"

#include <algorithm>

Document::Document() : text_file(new TextFile(master_io_provider)), text_view(*text_file, text_file.get()), owning_tool(nullptr), m_is_temporary(false), m_is_read_only(false), m_is_big(false) {
  get_appendage().tabdef = master.pref_manager.get_tabdef();
  call_hook(DocEvent::OPENED);
}

Document::Document(std::unique_ptr<TextFile> tf) : text_file(std::move(tf)), text_view(*text_file, text_file.get()), owning_tool(nullptr), m_is_temporary(false), m_is_read_only(false), m_is_big(false) {
  get_appendage().tabdef = master.pref_manager.get_tabdef();
  call_hook(DocEvent::OPENED);
}

Document::Document(std::unique_ptr<TextFile> tf, bool b) : text_file(std::move(tf)), text_view(*text_file, text_file.get()), owning_tool(nullptr), m_is_temporary(false), m_is_read_only(false), m_is_big(b) {
  get_appendage().tabdef = master.pref_manager.get_tabdef();
  call_hook(DocEvent::OPENED);
}

bool Document::check_read_only() {
  if (is_read_only()) {
    get_window()->feedback("Read-only", "Read-only document can't be edited.");
    return true;
  }
  return false;
}

HookSource<Doc*, int>* Document::get_doc_hook() {
  return &document_hook;
}

const TextBuffer* Document::get_text_buffer() const {
  return text_file.get();
}

CursorLocation Document::get_cursor() const {
  return text_view.get_cursor();
}

SelectionInfo Document::get_selection() const {
  return text_view.get_selection();
}

std::string Document::get_short_title() const {
  if (is_temporary()) return optional_title;

  std::string title;
  if (text_file->has_unsaved_edits()) {
    title = "*";
  }
  if (text_file->is_new()) {
    title += "Untitled";
  } else {
    title += text_file->get_file_name();
  }
  return title;
}

std::string Document::get_long_title() const {
  if (is_temporary()) return optional_title;

  std::string title;
  if (text_file->has_unsaved_edits()) {
    title = "*";
  }
  if (text_file->is_new()) {
    title += "Untitled";
  } else {
    title += text_file->get_file_name();
    title += " [";
    title += elide_right(text_file->get_absolute_path(), 50);
    title += "]";
  }
  return title;
}

int Document::get_display_style() const {
  if (m_is_big) return DocFlag::BIG_DOC;
  return 0;
}

void Document::handle_raw_char(KeyPress key) {
  const bool navigation_mode = get_visual_payload().navigation_mode;
  DocAction::Type action = navigation_mode ? master.get_key_mapper_navigation().map_key(key) : master.get_key_mapper_main().map_key(key);

  if (key.char_code == '\t' && key.ctrl) {
    handle_char(key);
  } else if (action == DocAction::NONE) {
    handle_char(key);
  } else {
    handle_action(action, key);
  }
}

void Document::handle_action(DocAction::Type action, KeyPress key) {
  master.set_markovian(MARKOVIAN_NONE);

  bool shift = key.shift;
  switch (action) {
    case DocAction::MOVE_DOWN:
      text_view.cursor_down(shift, get_visual_payload().flow_grid);
      if (get_appendage().folded) text_view.folded_momentum_down(shift);
      call_hook(DocEvent::CURSOR_MOVED);
      break;
    case DocAction::MOVE_UP:
      text_view.cursor_up(shift, get_visual_payload().flow_grid);
      if (get_appendage().folded) text_view.folded_momentum_up(shift);
      call_hook(DocEvent::CURSOR_MOVED);
      break;
    case DocAction::MOVE_LEFT:
      text_view.cursor_left(shift);
      if (get_appendage().folded) text_view.folded_momentum_up(shift);
      call_hook(DocEvent::CURSOR_MOVED);
      break;
    case DocAction::MOVE_RIGHT:
      text_view.cursor_right(shift);
      if (get_appendage().folded) text_view.folded_momentum_down(shift);
      call_hook(DocEvent::CURSOR_MOVED);
      break;
    case DocAction::SKIP_LEFT:
      text_view.skip_left(shift);
      if (get_appendage().folded) text_view.folded_momentum_up(shift);
      call_hook(DocEvent::CURSOR_MOVED);
      break;
    case DocAction::SKIP_RIGHT:
      text_view.skip_right(shift);
      if (get_appendage().folded) text_view.folded_momentum_down(shift);
      call_hook(DocEvent::CURSOR_MOVED);
      break;
    case DocAction::SKIP_UP:
      if (get_appendage().folded) {
        text_view.break_fold_up();
        call_hook(DocEvent::FOLDED | DocEvent::CENTRALIZE);
      }
      else text_view.skip_up(shift);
      call_hook(DocEvent::CURSOR_MOVED);
      break;
    case DocAction::SKIP_DOWN:
      if (get_appendage().folded) {
        text_view.break_fold_down();
        call_hook(DocEvent::FOLDED | DocEvent::CENTRALIZE);
      }
      else text_view.skip_down(shift);
      call_hook(DocEvent::CURSOR_MOVED);
      break;
    case DocAction::MOVE_PAGE_UP:
      text_view.page_up(get_visual_payload().page_size, shift);
      if (get_appendage().folded) text_view.folded_momentum_up(shift);
      call_hook(DocEvent::PAGE_UP | DocEvent::CURSOR_MOVED);
      break;
    case DocAction::MOVE_PAGE_DOWN:
      text_view.page_down(get_visual_payload().page_size, shift);
      if (get_appendage().folded) text_view.folded_momentum_down(shift);
      call_hook(DocEvent::PAGE_DOWN | DocEvent::CURSOR_MOVED);
      break;
    case DocAction::MOVE_HOME:
      text_view.home(shift);
      call_hook(DocEvent::CURSOR_MOVED);
      break;
    case DocAction::MOVE_HOME_FILE:
      text_view.home_file(shift);
      if (get_appendage().folded) text_view.folded_momentum_down(shift);
      call_hook(DocEvent::CURSOR_MOVED);
      break;
    case DocAction::MOVE_END:
      text_view.end(shift);
      call_hook(DocEvent::CURSOR_MOVED);
      break;
    case DocAction::MOVE_END_FILE:
      text_view.end_file(shift);
      if (get_appendage().folded) text_view.folded_momentum_up(shift);
      call_hook(DocEvent::CURSOR_MOVED);
      break;
    case DocAction::DELETE_FORWARD:
      if (check_read_only()) return;
      text_view.delete_forward();
      call_hook(DocEvent::EDITED | DocEvent::CURSOR_MOVED | DocEvent::CHANGED_STATE);
      break;
    case DocAction::UNINDENT:
      if (check_read_only()) return;
      text_view.untab(get_appendage().tabdef);
      call_hook(DocEvent::EDITED | DocEvent::CURSOR_MOVED | DocEvent::CHANGED_STATE);
      break;
    case DocAction::INDENT:
      if (check_read_only()) return;
      text_view.tab(get_appendage().tabdef);
      call_hook(DocEvent::EDITED | DocEvent::CURSOR_MOVED | DocEvent::CHANGED_STATE);
      break;
    case DocAction::DELETE_WORD:
      if (check_read_only()) return;
      text_view.delete_word();
      call_hook(DocEvent::EDITED | DocEvent::CURSOR_MOVED | DocEvent::CHANGED_STATE);
      break;
    case DocAction::NONE:
      printf("Warning: Not expected action.\n");
      break;
  }
}

void Document::handle_char(KeyPress key) {
  master.set_markovian(MARKOVIAN_NONE);
  if (check_read_only()) return;

  if (key.is_char) {
    uint32_t char_code = key.char_code;
    if (char_code == '\t') {
      if (key.ctrl) {
        text_view.insert_char('\t');
      } else if (key.shift) {
        text_view.untab(get_appendage().tabdef);
      }
      else text_view.tab(get_appendage().tabdef);
    }

    if (char_code == 8) { // Backspace
      text_view.delete_backward();
      if (get_appendage().folded) text_view.folded_momentum_up(false);
    }
    if (char_code == 13) {
      text_view.newline(); // Newline
      call_hook(DocEvent::NEWLINE);
    }
    if (char_code >= 32) {
      text_view.insert_char(char_code);
    }
  }
  call_hook(DocEvent::EDITED | DocEvent::CHANGED_STATE | DocEvent::CURSOR_MOVED);
}

void Document::handle_mouse(int row, int col, bool shift, bool ctrl, bool double_click) {
  master.set_markovian(MARKOVIAN_NONE);
  if (ctrl) {
    Line& l = text_file->get_line(row);
    master_navigators->jump(l.to_string(), col);
    return;
  }
  text_view.mouse(row, col, shift);
  if (double_click) text_view.select_word();
  if (get_appendage().folded) text_view.folded_momentum_up(shift);
  call_hook(DocEvent::CURSOR_MOVED);
}

void Document::handle_select_word() {
  master.set_markovian(MARKOVIAN_NONE);
  text_view.select_word();
  call_hook(DocEvent::CURSOR_MOVED);
}

void Document::handle_jump_to(int row, int col) {
  text_view.mouse(row, col, false);
  if (get_appendage().folded) text_view.folded_momentum_up(false);
  call_hook(DocEvent::CURSOR_MOVED | DocEvent::CENTRALIZE);
}

void Document::handle_undo() {
  master.set_markovian(MARKOVIAN_NONE);
  if (check_read_only()) return;
  text_view.undo();
  if (get_appendage().folded) text_view.folded_momentum_up(false);
  call_hook(DocEvent::CURSOR_MOVED | DocEvent::CENTRALIZE | DocEvent::CHANGED_STATE | DocEvent::EDITED);
}

std::string Document::handle_copy() {
  master.set_markovian(MARKOVIAN_NONE);
  return text_view.copy();
}

std::string Document::handle_cut() {
  master.set_markovian(MARKOVIAN_NONE);
  if (check_read_only()) return "";
  std::string txt = text_view.cut();
  if (get_appendage().folded) text_view.folded_momentum_up(false);
  if (txt != "") call_hook(DocEvent::CURSOR_MOVED | DocEvent::CENTRALIZE | DocEvent::CHANGED_STATE | DocEvent::EDITED);
  return txt;
}

void Document::handle_paste(const std::string& contents) {
  master.set_markovian(MARKOVIAN_NONE);
  if (check_read_only()) return;
  text_view.paste(contents);
  if (get_appendage().folded) text_view.folded_momentum_up(false);
  call_hook(DocEvent::CURSOR_MOVED | DocEvent::CENTRALIZE | DocEvent::CHANGED_STATE | DocEvent::EDITED);
}

std::string Document::handle_kill() {
  if (check_read_only()) return "";
  std::string txt = text_view.kill();
  if (get_appendage().folded) text_view.folded_momentum_down(false);
  call_hook(DocEvent::CURSOR_MOVED | DocEvent::CENTRALIZE | DocEvent::CHANGED_STATE | DocEvent::EDITED);
  return txt;
}

void Document::handle_select_all() {
  master.set_markovian(MARKOVIAN_NONE);
  text_view.select_all();
  if (get_appendage().folded) text_view.folded_momentum_up(true);
  call_hook(DocEvent::CURSOR_MOVED | DocEvent::CENTRALIZE);
}

void Document::handle_save() {
  master.set_markovian(MARKOVIAN_NONE);
  call_hook(DocEvent::BEFORE_SAVE);
  text_file->save(master.pref_manager.get_bool("saving.trim_trailing_spaces"));
  text_view.fix();
  call_hook(DocEvent::AFTER_SAVE | DocEvent::EDITED | DocEvent::CURSOR_MOVED | DocEvent::CHANGED_STATE);
  if (m_is_read_only && m_is_temporary) {
    m_is_read_only = false; m_is_temporary = false;
  }
}

void Document::handle_save_as(const std::string& path) {
  master.set_markovian(MARKOVIAN_NONE);
  text_file->change_path(path);
  if (m_is_read_only && m_is_temporary) {
    m_is_read_only = false; m_is_temporary = false;
  }
  call_hook(DocEvent::BEFORE_SAVE);
  text_file->save(master.pref_manager.get_bool("saving.trim_trailing_spaces"));
  text_view.fix();
  call_hook(DocEvent::AFTER_SAVE | DocEvent::EDITED | DocEvent::CURSOR_MOVED | DocEvent::CHANGED_STATE);
}

DocSearchResults Document::handle_search_update(const std::string& search_term, SearchSettings search_settings, bool search_back, bool move) {
  master.set_markovian(MARKOVIAN_NONE);
  get_appendage().rich_text.highlights.search.clear();

  DocSearchResults dsr = { 0, 0, 0, 0 };
  if (search_term.empty()) {
    call_hook(DocEvent::CURSOR_MOVED); // TODO: This is not really a cursor_moved...
    return dsr;
  }

  std::vector<SearchResult> results;
  text_file->search(search_term, results, search_settings);
  dsr.num_results = results.size();
  if (dsr.num_results == 0) {
    call_hook(DocEvent::CURSOR_MOVED); // TODO: This is not really a cursor_moved...
    return dsr;
  }

  // Highlight the results.
  for (const SearchResult& sr: results) {
    get_appendage().rich_text.highlights.search.push_back(Highlight(sr.row, sr.col, sr.col + sr.size));
  }

  // Now, possibly move the cursor.
  // TODO: This logic is a little hairy. Consider if there is maybe a simpler solution?
  const CursorLocation cl = text_view.get_cursor();
  bool found = false;
  for (unsigned int i = 0; i < results.size(); i++) {
    const SearchResult& sr = results[i];
    if (sr.row < cl.row || (sr.row == cl.row && sr.col < cl.col)) {
      // Behind. Ignore.
    } else if (sr.row == cl.row && sr.col == cl.col) {
      // Here
      if (!move) {
        found = true; dsr.cur_result = i; break;
      } else {
        found = true;
        if (search_back) dsr.cur_result = i-1;
        else dsr.cur_result = i+1;
        break;
      }
    } else {
      // After
      found = true;
      if (search_back) {
        dsr.cur_result = i-1;
      } else {
        dsr.cur_result = i;
      }
      break;
    }
  }
  if (!found) {
    if (search_back) {
      dsr.cur_result = results.size()-1;
    } else {
      dsr.cur_result = results.size();
    }
  }
  if (dsr.cur_result == -1) dsr.cur_result = dsr.num_results - 1;
  if (dsr.cur_result == dsr.num_results) dsr.cur_result = 0;

  if (dsr.cur_result >= 0 && dsr.cur_result < dsr.num_results) {
    const SearchResult& sr = results[dsr.cur_result];
    text_view.mouse(sr.row, sr.col, false);
    call_hook(DocEvent::CURSOR_MOVED | DocEvent::CENTRALIZE);
    dsr.row = sr.row;
    dsr.col = sr.col;
  }
  return dsr;
}

DocSearchResults Document::handle_search_replace(const std::string& search_term, SearchSettings search_settings, const std::string& replacement) {
  DocSearchResults dsr = { 0, 0, 0, 0 };
  master.set_markovian(MARKOVIAN_NONE);
  if (check_read_only()) return dsr;
  dsr = handle_search_update(search_term, search_settings, false, false);

  if (dsr.num_results == 0) return dsr;
  text_view.replace(search_term, replacement);
  if (get_appendage().folded) text_view.folded_momentum_up(false);
  call_hook(DocEvent::EDITED | DocEvent::CHANGED_STATE | DocEvent::CURSOR_MOVED | DocEvent::CENTRALIZE);
  dsr = handle_search_update(search_term, search_settings, false, false);
  return dsr;
}

int Document::handle_search_replace_all(const std::string& search_term, SearchSettings search_settings, const std::string& replacement) {
  master.set_markovian(MARKOVIAN_NONE);
  if (check_read_only()) return 0;
  int rv = text_view.replace_all(search_term, search_settings, replacement);
  if (get_appendage().folded) text_view.folded_momentum_up(false);
  call_hook(DocEvent::EDITED | DocEvent::CHANGED_STATE | DocEvent::CURSOR_MOVED | DocEvent::CENTRALIZE);
  return rv;
}

void Document::handle_change_type(const std::string& new_type) {
  get_appendage().file_type = new_type;
  master.stat_lang.set_document_type(get_appendage().statlang_id, new_type);
  call_hook(DocEvent::CHANGED_TYPE);
}

void Document::handle_comment() {
  master.set_markovian(MARKOVIAN_NONE);
  if (check_read_only()) return;

  std::string comment, comment2;
  master.stat_lang.get_document_comment(get_appendage().statlang_id, comment, comment2);
  if (comment.empty()) {
    std::string text = "No definition of a single-line comment for type ";
    text += master.stat_lang.get_document_type(get_appendage().statlang_id);
    master.feedback("Cannot comment out", text);
    return;
  }
  text_view.comment(comment);
  call_hook(DocEvent::EDITED | DocEvent::CURSOR_MOVED | DocEvent::CHANGED_STATE);
}

void Document::handle_uncomment() {
  master.set_markovian(MARKOVIAN_NONE);
  if (check_read_only()) return;

  std::string comment, comment2;
  master.stat_lang.get_document_comment(get_appendage().statlang_id, comment, comment2);
  if (comment.empty()) {
    std::string text = "No definition of a single-line comment for type ";
    text += master.stat_lang.get_document_type(get_appendage().statlang_id);
    master.feedback("Cannot uncomment", text);
    return;
  }
  text_view.uncomment(comment);
  call_hook(DocEvent::EDITED | DocEvent::CURSOR_MOVED | DocEvent::CHANGED_STATE);
}

void Document::handle_complete(std::vector<std::string>& matches, bool complete_prefix) {
  master.set_markovian(MARKOVIAN_NONE);
  if (check_read_only()) return;

  std::string current_id = text_view.identifier_left();

  if (current_id == "") {
    master.feedback("Can't complete", "Completion only possible for identifiers.");
    return;
  }

  std::string whole_word = text_view.identifier();
  std::string common_prefix;
  master.stat_lang.query_symboldb(current_id, whole_word, matches, common_prefix);

  if (matches.size() == 0) {
    std::string text = "Term '";
    text += current_id;
    text += "' has no completion candidates.";
    master.feedback("Unique", text);
  }

  if (complete_prefix && common_prefix.size() > current_id.size()) {
    handle_complete(common_prefix);
    if (matches.size() == 1) {
      matches.clear();
    }
  }
}

void Document::handle_complete(const std::string& str) {
  master.set_markovian(MARKOVIAN_NONE);
  if (check_read_only()) return;

  std::string current_id = text_view.identifier_left();
  if (!is_prefix(current_id, str)) return;
  text_view.select_word_left();
  text_view.paste(str);

  call_hook(DocEvent::EDITED | DocEvent::CURSOR_MOVED | DocEvent::CHANGED_STATE);
}

void Document::handle_escape() {
  text_view.select_none();
  get_appendage().rich_text.overlays.clear();
  call_hook(DocEvent::CURSOR_MOVED);
}

void Document::handle_fold(bool toggle) {
  get_appendage().folded = toggle;

  for (int i = 0; i < text_file->get_num_lines(); i++) {
    Line& l = text_file->get_line(i);
    l.appendage().folded = false;
  }

  if (toggle == false) {
    call_hook(DocEvent::CURSOR_MOVED | DocEvent::EDITED | DocEvent::FOLDED | DocEvent::CENTRALIZE);
    return;
  }

  text_view.fold(0, text_file->get_num_lines() - 1);
  call_hook(DocEvent::CURSOR_MOVED | DocEvent::EDITED | DocEvent::FOLDED | DocEvent::CENTRALIZE);
}

std::unique_ptr<Document> make_temp_read_only_document(const std::string& title, const std::string& contents) {
  std::unique_ptr<TextFile> tf(new TextFile(master_io_provider));
  tf->from_utf8(contents);
  std::unique_ptr<Document> doc(new Document(std::move(tf)));
  doc->set_optional_title(title);
  doc->set_temporary(true);
  doc->set_read_only(true);
  return std::move(doc);
}