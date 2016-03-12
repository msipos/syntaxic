#ifndef SYNTAXIC_DOCUMENT_HPP
#define SYNTAXIC_DOCUMENT_HPP

#include "core/common.hpp"
#include "core/hooks.hpp"
#include "core/text_file.hpp"
#include "core/text_view.hpp"
#include "doc.hpp"
#include "tool.hpp"

enum VirtualState {
  VS_OK, VS_RUNNING, VS_FAILED
};

class Document : public Doc {
private:
  std::unique_ptr<TextFile> text_file;
  TextView text_view;
  HookSource<Doc*, int> document_hook;
  SynTool* owning_tool;
  std::string optional_title;
  bool m_is_temporary, m_is_read_only, m_is_big;

  bool check_read_only();

public:
  Document();
  explicit Document(std::unique_ptr<TextFile> tf);
  explicit Document(std::unique_ptr<TextFile> tf, bool big_file);
  Document(const Document&) = delete;
  Document& operator=(const Document&) = delete;

  // Required Doc methods.

  virtual HookSource<Doc*, int>* get_doc_hook();
  /** Get the contents TextBuffer. */
  virtual const TextBuffer* get_text_buffer() const;
  /** Get current cursor location. */
  virtual CursorLocation get_cursor() const;
  /** Get selection information. */
  virtual SelectionInfo get_selection() const;
  /** Get short title. */
  virtual std::string get_short_title() const;

  // Optional Doc methods.

  virtual std::string get_long_title() const;
  virtual int get_display_style() const;

  // Other

  inline TextFile* get_text_file() { return text_file.get(); }
  inline TextView* get_text_view() { return &text_view; }
  inline void set_owning_tool(SynTool* st) { owning_tool = st; }
  inline bool is_temporary() const { return m_is_temporary; }
  inline bool is_read_only() const { return m_is_read_only; }
  inline bool is_big() const { return m_is_big; }
  inline void set_temporary(bool b) { m_is_temporary = b; }
  inline void set_read_only(bool b) { m_is_read_only = b; }
  inline void set_optional_title(const std::string& t) { optional_title = t; }
  inline void set_big(bool b) { m_is_big = b; }

  virtual void handle_raw_char(KeyPress key);
  virtual void handle_action(DocAction::Type action, KeyPress key) override;
  virtual void handle_char(KeyPress key) override;
  virtual void handle_mouse(int row, int col, bool shift, bool ctrl, bool double_click=false) override;
  virtual void handle_select_word() override;
  virtual void handle_jump_to(int row, int col) override;
  virtual void handle_undo() override;
  virtual std::string handle_copy() override;
  virtual std::string handle_cut() override;
  virtual void handle_paste(const std::string& contents) override;
  virtual void handle_select_all() override;
  virtual std::string handle_kill() override;
  virtual void handle_save() override;
  virtual void handle_save_as(const std::string& path) override;
  virtual DocSearchResults handle_search_update(const std::string& search_term, SearchSettings search_settings, bool search_back, bool move) override;
  virtual DocSearchResults handle_search_replace(const std::string& search_term, SearchSettings search_settings, const std::string& replacement) override;
  virtual int handle_search_replace_all(const std::string& search_term, SearchSettings search_settings, const std::string& replacement) override;
  virtual void handle_change_type(const std::string& new_type) override;
  virtual void handle_comment() override;
  virtual void handle_uncomment() override;
  virtual void handle_complete(std::vector<std::string>& matches, bool complete_prefix) override;
  virtual void handle_complete(const std::string& str) override;
  virtual void handle_escape() override;
  virtual void handle_fold(bool toggle) override;
};

std::unique_ptr<Document> make_temp_read_only_document(const std::string& title, const std::string& contents);

#endif