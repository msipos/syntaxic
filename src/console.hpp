#ifndef SYNTAXIC_CONSOLE_HPP
#define SYNTAXIC_CONSOLE_HPP

#include "core/common.hpp"
#include "core/text_buffer.hpp"
#include "core/text_view.hpp"
#include "doc.hpp"

#include <vector>

class Console : public Doc {
private:

  TextBuffer text_buffer;
  TextBuffer line_buffer;
  TextView line_view;
  CursorLocation text_end;

  HookSource<Doc*, int> document_hook;

  void update_output_buffer();

  std::vector<std::string> history;
  std::string current_line;
  int current_history;
  void update_from_history();

protected:
  std::string prompt;

public:

  Console();
  virtual ~Console();

  // Implement needed functions

  /** Get the hook that triggers on this document. */
  virtual HookSource<Doc*, int>* get_doc_hook();
  /** Get the contents TextBuffer. */
  virtual const TextBuffer* get_text_buffer() const;
  /** Get current cursor location. */
  virtual CursorLocation get_cursor() const;
  /** Get selection information. */
  virtual SelectionInfo get_selection() const;
  /** Get short title. */
  virtual std::string get_short_title() const;

  // Our:

  void console_output(const std::string& contents, uint8_t markup=0);
  virtual void console_input(const std::string& contents);

  // Optional functions

  virtual void handle_raw_char(KeyPress key) override;
  virtual void handle_mouse(int row, int col, bool shift, bool ctrl, bool double_click) override;
  virtual void handle_paste(const std::string& contents);
};

#endif