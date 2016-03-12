#include "core/text_buffer.hpp"
#include "doc.hpp"

Doc::Doc() : collection(nullptr), window(nullptr) {}

Doc::~Doc() {
  all_docs_hook.call(this, DocEvent::CLOSING);
}

Doc* DocCollection::get_current_doc() const {
  if (current_index < 0) return nullptr;
  if (current_index >= int(docs.size())) {
    printf("Warning: get_current_doc but current_index is too big.\n");
    return nullptr;
  }
  return docs[current_index];
}

HookSource<Doc*, int> all_docs_hook;

void Doc::call_hook(int flags) {
  get_doc_hook()->call(this, flags);
  all_docs_hook.call(this, flags);
}

std::string Doc::get_long_title() const {
  return get_short_title();
}

int Doc::get_display_style() const {
  return 0;
}

void Doc::handle_raw_char(KeyPress /* key */) {}
void Doc::handle_action(DocAction::Type /* action */, KeyPress /* key */) {}
/** A character which is not an action. */
void Doc::handle_char(KeyPress /* key */) {}
/** Mouse click. */
void Doc::handle_mouse(int /* row */, int /* col */, bool /* shift */, bool /* ctrl */, bool /* double_click */) {}
/** Select the current word. */
void Doc::handle_select_word() {}
/** Jump to this location. Like handle_mouse but no shift and there is a CENTRALIZE trigger.*/
void Doc::handle_jump_to(int /* row */, int /* col */) {}
/** Some actions that come directly. */
void Doc::handle_undo() {}
std::string Doc::handle_copy() { return ""; }
std::string Doc::handle_cut() { return ""; }
void Doc::handle_paste(const std::string& /* contents */) {}
void Doc::handle_select_all() {}
std::string Doc::handle_kill() { return ""; }
void Doc::handle_save() {}
void Doc::handle_save_as(const std::string& /* path */) {}
DocSearchResults Doc::handle_search_update(const std::string& /* search_term */, SearchSettings /* search_settings */, bool /* search_back */, bool /* move */) { return { 0, 0, 0, 0}; }
DocSearchResults Doc::handle_search_replace(const std::string& /* search_term */, SearchSettings /* search_settings */, const std::string& /* replacement */) { return { 0, 0, 0, 0}; }
int Doc::handle_search_replace_all(const std::string& /* search_term */, SearchSettings /* search_settings */, const std::string& /* replacement */) { return 0; }
void Doc::handle_change_type(const std::string& /* new_type */) {}
void Doc::handle_comment() {}
void Doc::handle_uncomment() {}
void Doc::handle_complete(std::vector<std::string>& /* matches */, bool /* complete_prefix */) {}
void Doc::handle_complete(const std::string& /* str */) {}
/** Process is running and producting output. This is only invoked for documents. */
void Doc::handle_console(const std::string& /* text */) {}
void Doc::handle_escape() {}
void Doc::handle_fold(bool /* toggle */) {}
bool Doc::handle_about_to_close() { return true; }

std::string Doc::get_selection_as_string() const {
  const TextBuffer* tb = get_text_buffer();
  SelectionInfo si = get_selection();
  return tb->selection_as_string(si);
}

void Doc::trigger_refresh() {
  call_hook(DocEvent::CURSOR_MOVED | DocEvent::EDITED | DocEvent::CHANGED_STATE);
}