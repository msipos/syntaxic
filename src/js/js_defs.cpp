#include "core/util_path.hpp"
#include "master_io_provider.hpp"

DFUNC void duk_addOverlay(int handle, int row, int col, int type, const char* text) {
  Doc* doc = master.js_get_doc(handle);
  if (doc == nullptr) return;
  doc->get_appendage().rich_text.add_overlay(doc->get_text_buffer(), row, col, OverlayType::NOTICE, text);
  doc->trigger_refresh();
}

DFUNC void duk_clearOverlays(int handle) {
  Doc* doc = master.js_get_doc(handle);
  if (doc != nullptr) doc->get_appendage().rich_text.overlays.clear();
}

DFUNC void duk_clearPluginMenu() {
  /* Deprecated. */
}

DFUNC void duk_docChar(int handle, int ch) {
  Doc* doc = master.js_get_doc(handle);
  if (doc == nullptr) return;
  doc->handle_raw_char(KeyPress(ch, false, false, false, false));
}

DFUNC std::string duk_docCopy(int handle) {
  Doc* doc = master.js_get_doc(handle);
  if (doc == nullptr) return "";
  return doc->handle_copy();
}

DFUNC std::string duk_docCut(int handle) {
  Doc* doc = master.js_get_doc(handle);
  if (doc == nullptr) return "";
  return doc->handle_cut();
}

DFUNC void duk_docMove(int handle, int row, int col, bool ctrl, bool shift) {
  Doc* doc = master.js_get_doc(handle);
  if (doc != nullptr) doc->handle_mouse(row, col, shift, ctrl);
}

DFUNC void duk_docPaste(int handle, const char* text) {
  Doc* doc = master.js_get_doc(handle);
  if (doc != nullptr) doc->handle_paste(text);
}

DFUNC void duk_feedback(const char* title, const char* text) {
  master.feedback(title, text);
}

DFUNC int duk_getCurrentDoc() {
  return master.js_get_current_doc();
}

DFUNC std::string duk_getDocLine(int handle, int row) {
  Doc* doc = master.js_get_doc(handle);
  if (doc == nullptr) return "";
  const TextBuffer* tb = doc->get_text_buffer();
  if (row < 0 || row >= tb->get_num_lines()) return "";
  return tb->get_line(row).to_string();
}

DFUNC int duk_getDocNumLines(int handle) {
  Doc* doc = master.js_get_doc(handle);
  if (doc == nullptr) return 0;
  const TextBuffer* tb = doc->get_text_buffer();
  return tb->get_num_lines();
}

DFUNC std::string getDocType(int handle) {
  Doc* doc = master.js_get_doc(handle);
  if (doc == nullptr) return "";
  return doc->get_appendage().file_type;
}

DFUNC std::string duk_getDocSelectedText(int handle) {
  Doc* doc = master.js_get_doc(handle);
  if (doc == nullptr) return "";
  return doc->get_selection_as_string();
}

DFUNC std::string duk_getDocText(int handle) {
  Doc* doc = master.js_get_doc(handle);
  if (doc == nullptr) return "";
  return doc->get_text_buffer()->to_string();
}

DFUNC int duk_getShellDoc() {
  return master.js_get_shell_doc();
}

DFUNC void duk_goTo(int handle, int row, int col) {
  Doc* doc = master.js_get_doc(handle);
  if (doc != nullptr) doc->handle_mouse(row, col, false, false);
}

DFUNC std::string duk_ioReadFile(const char* path) {
  std::vector<char> vec = master_io_provider->read_file(path);
  return std::string(vec.data(), vec.size());
}

DFUNC void duk_ioRemoveFile(const char* path) {
  master_io_provider->remove(path);
}

DFUNC std::string duk_ioTempFile() {
  return UtilPath::temp_file();
}

DFUNC void duk_ioWriteFile(const char* path, const char* contents) {
  master_io_provider->write_file_safe(path, contents, strlen(contents));
}

DFUNC std::string duk_promptText(const char* title, const char* text, const char* defaultText) {
  MainWindow* main_window = dynamic_cast<MainWindow*>(master.get_main_window());
  std::string result;
  main_window->get_user_text_input(title, text, defaultText, 0, result);
  return result;
}

///// Extras not handled by duktaper:

/** Syn.addMenu( str, callback ) */
static void syn_addMenu_callback(std::string name) {
  if (master_js->stored_menus.count(name) == 0) return;
  get_js_internal(global_ctx, name);
  int rv = duk_pcall(global_ctx, 0);
  if (rv != DUK_EXEC_SUCCESS) {
    master.feedback("Javascript Error", duk_to_string(global_ctx, -1));
  }
  duk_pop_2(global_ctx);
}

static duk_ret_t syn_addPluginMenuItem(duk_context* ctx) {
  const char* menu_def = duk_require_string(ctx, 0);
  const char* menu_key = duk_require_string(ctx, 1);
  duk_require_object_coercible(ctx, 2);

  std::string s_menu_def(menu_def);
  s_menu_def = "Plugins>" + s_menu_def;

  // Add to internals
  master_js->stored_menus.insert(s_menu_def);
  attach_js_internal(ctx, s_menu_def, 2);

  MainWindow* main_window = dynamic_cast<MainWindow*>(master.get_main_window());
  std::function<void()> func = std::bind(syn_addMenu_callback, s_menu_def);
  main_window->menu_manager->add_menu(s_menu_def, menu_key, true, func, 1);

  return 0;
}

/** Syn.addPluginCallback( str, callback ) */
static duk_ret_t syn_addPluginCallback(duk_context* ctx) {
  const char* type = duk_require_string(ctx, 0);
  duk_require_object_coercible(ctx, 1);

  std::string stype(type);
  int len = master_js->stored_events[stype].size();
  std::string func_name(stype + "_" + std::to_string(len));
  attach_js_internal(ctx, func_name, 1);
  master_js->stored_events[stype].push_back(func_name);

  return 0;
}

static duk_ret_t syn_getDocCursorLocation(duk_context* ctx) {
  int handle = duk_require_int(ctx, 0);
  Doc* doc = master.js_get_doc(handle);
  if (doc == nullptr) return 0;
  CursorLocation cl = doc->get_cursor();

  duk_push_object(ctx);
  duk_push_int(ctx, cl.row); duk_put_prop_string(ctx, -2, "row");
  duk_push_int(ctx, cl.col); duk_put_prop_string(ctx, -2, "col");
  return 1;
}

static duk_ret_t syn_getDocSelection(duk_context* ctx) {
  int handle = duk_require_int(ctx, 0);
  Doc* doc = master.js_get_doc(handle);
  if (doc == nullptr) return 0;
  SelectionInfo si = doc->get_selection();
  if (!si.active) return 0;

  duk_push_object(ctx);
  duk_push_int(ctx, si.row_start); duk_put_prop_string(ctx, -2, "start_row");
  duk_push_int(ctx, si.col_start); duk_put_prop_string(ctx, -2, "start_col");
  duk_push_int(ctx, si.row_end); duk_put_prop_string(ctx, -2, "end_row");
  duk_push_int(ctx, si.col_end); duk_put_prop_string(ctx, -2, "end_col");
  duk_push_int(ctx, si.lines_start); duk_put_prop_string(ctx, -2, "lines_start");
  duk_push_int(ctx, si.lines_end); duk_put_prop_string(ctx, -2, "lines_end");
  return 1;
}

/** Syn.system( obj ) */
static duk_ret_t syn_system(duk_context* ctx) {
  duk_require_object_coercible(ctx, 0);

  duk_get_prop_string(ctx, -1, "command");
  std::string command(duk_require_string(ctx, -1));
  duk_pop(ctx);

  duk_get_prop_string(ctx, -1, "stdin");
  std::string s_stdin;
  if (duk_get_type(ctx, -1) == DUK_TYPE_STRING) {
    s_stdin = std::string(duk_require_string(ctx, -1));
  }
  duk_pop(ctx);

  int exit_code = 0;
  std::string s_stdout, s_stderr;
  {
    QProcess q_process;

    q_process.start(QString::fromStdString(command));
    q_process.waitForStarted(10000);
    if (!s_stdin.empty()) {
      q_process.write(s_stdin.c_str(), s_stdin.size());
    }
    q_process.closeWriteChannel();

    q_process.waitForFinished(10000);

    if (q_process.state() != QProcess::NotRunning) {
      q_process.kill();
    }
    exit_code = q_process.exitCode();
    s_stdout = q_process.readAllStandardOutput().toStdString();
    s_stderr = q_process.readAllStandardError().toStdString();
  }

  duk_push_object(ctx);
  duk_push_int(ctx, exit_code);
  duk_put_prop_string(ctx, -2, "exitCode");
  duk_push_string(ctx, s_stdout.c_str());
  duk_put_prop_string(ctx, -2, "stdout");
  duk_push_string(ctx, s_stderr.c_str());
  duk_put_prop_string(ctx, -2, "stderr");
  return 1;
}

//////////////// INIT EXTRAS

static void add_func(duk_context* ctx, duk_ret_t (*func)(duk_context*), const char* name, int num_params) {
  duk_push_c_function(ctx, func, num_params);
  duk_put_prop_string(ctx, -2, name);
}

#define ADD_FUNC(name, num_params) add_func(ctx, syn_ ## name, #name, num_params)

static void init_extras(duk_context* ctx) {
  duk_get_global_string(ctx, "Syn");
  ADD_FUNC(addPluginMenuItem, 3);
  ADD_FUNC(addPluginCallback, 2);
  ADD_FUNC(getDocCursorLocation, 1);
  ADD_FUNC(getDocSelection, 1);
  ADD_FUNC(system, 1);
  duk_pop(ctx); // Syn
}