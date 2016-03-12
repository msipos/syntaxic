#include "core/util.hpp"
#include "core/util_path.hpp"
#include "master.hpp"
#include "master_js.hpp"
#include "process_impl.hpp"
#include "qtgui/main_window.hpp"
#include "qtgui/menu_manager.hpp"
#include "qtgui/qtmain.hpp"

#include "duktape.h"
#include <functional>
#include <QFile>
#include <QProcess>

struct MasterJSImpl {
  duk_context *ctx;
};

MasterJS* master_js = nullptr;
duk_context* global_ctx = nullptr;

static void attach_js_internal(duk_context* ctx, const std::string& name, int stack_idx);
static void get_js_internal(duk_context* ctx, const std::string& name);
static void delete_js_internal(duk_context* ctx, const std::string& name);

static void fatal_function(duk_context* /* ctx */, duk_errcode_t /* code */, const char *msg) {
  printf("Fatal JS error: %s\n", msg);
  if (master_js != nullptr) {
    //printf("Rebooting JS heap...\n");
    //master_js->reboot_heap();
  }
}

MasterJS::MasterJS() : pimpl(new MasterJSImpl) {
  master_js = this;
  pimpl->ctx = nullptr;
  reboot_heap();
  hook = all_docs_hook.add(std::bind(&MasterJS::hook_callback, this, std::placeholders::_1, std::placeholders::_2));
}

MasterJS::~MasterJS() {
  master_js = nullptr;
}

void MasterJS::hook_callback(Doc* doc, int type) {
  std::string stype;
  if (type & DocEvent::BEFORE_SAVE) {
    stype = "before_save";
  } else if (type & DocEvent::AFTER_SAVE) {
    stype = "after_save";
  } else if (type & DocEvent::NEWLINE) {
    stype = "newline";
  }
  if (stype.empty()) return;

  if (stored_events.count(stype) == 0) return;

  std::string errors;
  for (std::string& func_name: stored_events[stype]) {
    get_js_internal(global_ctx, func_name);
    duk_push_int(global_ctx, master.js_get_doc_handle(doc));
    int rv = duk_pcall(global_ctx, 1);
    if (rv != DUK_EXEC_SUCCESS) {
      errors += duk_to_string(global_ctx, -1);
    }
    duk_pop_2(global_ctx);
  }
  if (!errors.empty()) {
    master.feedback("Javascript Errors", errors);
  }
}

static std::string debug_stack(duk_context* ctx, int stack_idx) {
  int type = duk_get_type(ctx, stack_idx);
  std::string output;
  switch(type) {
    case DUK_TYPE_NONE:
      output = "<Unknown type value>";
      break;
    case DUK_TYPE_UNDEFINED:
      output = "undefined";
      break;
    case DUK_TYPE_NULL:
      output = "null";
      break;
    case DUK_TYPE_BOOLEAN:
      if (duk_get_boolean(ctx, -1)) output = "true";
      else output = "false";
      break;
    case DUK_TYPE_NUMBER:
      {
        double d = duk_get_number(ctx, -1);
        output = std::to_string(d);
      }
      break;
    case DUK_TYPE_STRING:
      output = duk_get_string(ctx, -1);
      output = std::string("\"") + output + "\"";
      break;
    case DUK_TYPE_OBJECT:
      {
        duk_dup(ctx, stack_idx);
        std::string tmp(duk_safe_to_string(ctx, -1));
        duk_pop(ctx);
        duk_dup(ctx, stack_idx);
        const char* str = duk_json_encode(ctx, -1);
        if (str) {
          output += std::string(str);
        } else {
          output += tmp;
        }
        duk_pop(ctx);
//        duk_get_prototype(ctx, stack_idx);
//        std::string proto(duk_safe_to_string(ctx, -1));
//      	output += " (proto: " +  proto + ")";
//      	duk_pop(ctx);
      }
      break;
    case DUK_TYPE_BUFFER:
      output = "Buffer";
      break;
    case DUK_TYPE_POINTER:
      output = "Pointer";
      break;
    case DUK_TYPE_LIGHTFUNC:
      output = "Lightfunc";
      break;
  }
  return output;
}

/////// Internal data

/** Attach a JS object at stack_idx to heap stash object. Note stack_idx must be positive! */
static void attach_js_internal(duk_context* ctx, const std::string& name, int stack_idx) {
  master_js->stored_objs.insert(name);
  duk_push_heap_stash(ctx);
  duk_dup(ctx, stack_idx);
  duk_put_prop_string(ctx, -2, name.c_str());
  duk_pop_2(ctx);
}

/** Stacks 2 things, of which -1 is what we want. */
static void get_js_internal(duk_context* ctx, const std::string& name) {
  duk_push_heap_stash(ctx);
  duk_get_prop_string(ctx, -1, name.c_str());
}

/** Delete internally held JS object. */
static void delete_js_internal(duk_context* ctx, const std::string& name) {
  master_js->stored_objs.erase(name);
  duk_push_heap_stash(ctx);
  duk_del_prop_string(ctx, -1, name.c_str());
}

/////// API

#include "js/js_defs.hpp"
#include "js/js_defs.cpp"
#include "js/js_impl.cpp"

//////// Other stuff


void MasterJS::reboot_heap() {
  stored_objs.clear();
  stored_menus.clear();
  stored_events.clear();

  if (pimpl->ctx) duk_destroy_heap(pimpl->ctx);
  pimpl->ctx = duk_create_heap(nullptr, nullptr, nullptr, nullptr, fatal_function);
  global_ctx = pimpl->ctx;

  // This will instantiate Syn object
  init_wrappers(pimpl->ctx);
  // This will further populate the Syn object:
  init_extras(pimpl->ctx);
}

std::string MasterJS::eval_string(const std::string& str) {
  std::string output;
  if (duk_peval_string(pimpl->ctx, str.c_str())) {
    output = std::string("Error: ") + duk_safe_to_string(pimpl->ctx, -1);
    duk_pop(pimpl->ctx);
    return output;
  }

  if (duk_get_top(pimpl->ctx) == 0) {
    return "no stack";
  }
  output = debug_stack(pimpl->ctx, -1);
  duk_pop(pimpl->ctx);
  return output;
}

std::string MasterJS::eval_file(const std::string& path) {
  std::vector<char> contents;
  read_file(contents, path);
  std::string str(contents.data(), contents.size());
  return eval_string(str);
}

std::string MasterJS::get_syntaxic_js_file() {
  return UtilPath::join_components(my_application->get_config_dir(), "syntaxic.js");
}

void MasterJS::make_default_syntaxic_js_file() {
  std::string path = get_syntaxic_js_file();
  if (!UtilPath::is_existing_file(path)) {
    QFile qf(":resources/syntaxic.js");
    qf.open(QIODevice::ReadOnly);
    std::string contents = qf.readAll().toStdString();
    write_file(contents.c_str(), contents.size(), path);
  }
}

std::string MasterJS::eval_syntaxic_js_file() {
  MainWindow* main_window = dynamic_cast<MainWindow*>(master.get_main_window());
  main_window->menu_manager->clear_plugin_menu();

  reboot_heap();

  std::string path = get_syntaxic_js_file();
  if (UtilPath::is_existing_file(path)) {
    return eval_file(path);
  }
  return "";
}