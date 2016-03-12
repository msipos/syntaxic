#include "core/text_edit.hpp"
#include "console.hpp"
#include "master.hpp"
#include "master_navigators.hpp"

Console::Console() : line_view(line_buffer), current_history(0) {
  text_end = text_buffer.cursor_end_buffer();
  prompt = " > ";
}

Console::~Console() {
}

void Console::update_output_buffer() {
  // Trim the line
  {
    SimpleTextEdit ste(text_buffer);
    ste.remove_text(text_end, text_buffer.cursor_end_buffer());
  }

  // Append back the line
  {
    std::string s_line = line_buffer.to_string();
    SimpleTextEdit ste(text_buffer);
    if (!prompt.empty()) ste.insert_text(text_buffer.cursor_end_buffer(), prompt);
    ste.insert_text(text_buffer.cursor_end_buffer(), s_line);
  }

  call_hook(DocEvent::EDITED | DocEvent::CURSOR_MOVED);
}

HookSource<Doc*, int>* Console::get_doc_hook(){
  return &document_hook;
}

const TextBuffer* Console::get_text_buffer() const {
  return &text_buffer;
}

CursorLocation Console::get_cursor() const {
  CursorLocation cl = text_end;
  CursorLocation cl2 = line_view.get_cursor();
  cl.col = cl.col + prompt.size() + cl2.col;
  return cl;
}

SelectionInfo Console::get_selection() const {
  SelectionInfo si;
  si.active = false;
  return si;
}

std::string Console::get_short_title() const {
  return "Console";
}

void Console::console_output(const std::string& contents, uint8_t markup) {
  text_buffer.trim_lines_to_size(1000);

  // Trim the line
  {
    SimpleTextEdit ste(text_buffer);
    ste.remove_text(text_end, text_buffer.cursor_end_buffer());
  }

  // Append new data

  {
    SimpleTextEdit ste(text_buffer);
    ste.insert_text(text_buffer.cursor_end_buffer(), contents, markup);
  }
  text_end = text_buffer.cursor_end_buffer();

  // Append back the line
  {
    std::string s_line = line_buffer.to_string();
    SimpleTextEdit ste(text_buffer);
    if (!prompt.empty()) ste.insert_text(text_buffer.cursor_end_buffer(), prompt);
    ste.insert_text(text_buffer.cursor_end_buffer(), s_line);
  }

  call_hook(DocEvent::EDITED | DocEvent::CURSOR_MOVED);
}

/** Default implementation does an echo. */
void Console::console_input(const std::string& contents) {
  console_output(prompt + contents + "\nCommand was: " + contents + "\n");
}

void Console::update_from_history() {
  std::string* line;
  if (current_history == 0) {
    line = &current_line;
  } else {
    line = &(history[history.size() - current_history]);
  }
  line_buffer.from_utf8(*line);
  line_view.end_file(false);
  update_output_buffer();
}

void Console::handle_raw_char(KeyPress key) {
  master.set_markovian(MARKOVIAN_NONE);

  const bool navigation_mode = get_visual_payload().navigation_mode;
  DocAction::Type action = navigation_mode ? master.get_key_mapper_navigation().map_key(key) : master.get_key_mapper_main().map_key(key);

  bool shift = key.shift;
  if (action == DocAction::NONE) {
    if (key.is_char) {
      uint32_t char_code = key.char_code;
      //if (char_code == '\t') {
      //  if (shift) line_view.untab(get_appendage().tabdef);
      //  else line_view.tab(get_appendage().tabdef);
      //}
      if (char_code == 8) { // Backspace
        line_view.delete_backward();
      } else if (char_code == 13) {
        // Newline:
        line_view.home_file(false);
        std::string input = line_buffer.to_string();
        line_buffer.from_utf8("");
        console_input(input);
        if (history.size() == 0 || history.back() != input) history.push_back(input);
        current_history = 0;
        if (history.size() > 200) {
          history.erase(history.begin());
        }
      } else if (char_code >= 32) {
        line_view.insert_char(char_code);
      }
    }
    update_output_buffer();
  } else {
    switch (action) {
      case DocAction::MOVE_UP:
        // Save current line
        if (current_history == 0) {
          current_line = line_buffer.to_string();
        } else {
          if (current_history > int(history.size())) break;
          history[history.size() - current_history] = line_buffer.to_string();
        }
        current_history += 1;
        if (current_history > int(history.size())) current_history--;
        update_from_history();
        break;
      case DocAction::MOVE_DOWN:
        // Save current line
        if (current_history == 0) {
          break;
        } else {
          if (current_history > int(history.size())) break;
          history[history.size() - current_history] = line_buffer.to_string();
        }
        current_history -= 1;
        update_from_history();
        break;
      case DocAction::MOVE_LEFT:
        line_view.cursor_left(shift);
        call_hook(DocEvent::CURSOR_MOVED);
        break;
      case DocAction::MOVE_RIGHT:
        line_view.cursor_right(shift);
        call_hook(DocEvent::CURSOR_MOVED);
        break;
      case DocAction::MOVE_HOME:
        line_view.home(shift);
        call_hook(DocEvent::CURSOR_MOVED);
        break;
      case DocAction::MOVE_END:
        line_view.end(shift);
        call_hook(DocEvent::CURSOR_MOVED);
        break;
      case DocAction::SKIP_LEFT:
        line_view.skip_left(shift);
        call_hook(DocEvent::CURSOR_MOVED);
        break;
      case DocAction::SKIP_RIGHT:
        line_view.skip_right(shift);
        call_hook(DocEvent::CURSOR_MOVED);
        break;
      case DocAction::DELETE_FORWARD:
        line_view.delete_forward();
        call_hook(DocEvent::EDITED | DocEvent::CURSOR_MOVED | DocEvent::CHANGED_STATE);
        update_output_buffer();
        break;
      case DocAction::DELETE_WORD:
        line_view.delete_word();
        call_hook(DocEvent::EDITED | DocEvent::CURSOR_MOVED | DocEvent::CHANGED_STATE);
        update_output_buffer();
        break;
      default:
        // Ignore on purpose.
        break;
    }
  }
}

void Console::handle_mouse(int row, int col, bool /* shift */, bool ctrl, bool /* double_click */) {
  if (ctrl) {
    Line& l = text_buffer.get_line(row);
    master_navigators->jump(l.to_string(), col);
    return;
  }
}

void Console::handle_paste(const std::string& contents) {
  line_view.paste(contents);
  update_output_buffer();
}
