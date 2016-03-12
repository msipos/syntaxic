#ifndef SYNTAXIC_MASTER_JS_HPP
#define SYNTAXIC_MASTER_JS_HPP

#include "core/hooks.hpp"
#include "doc.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

struct MasterJSImpl;

class MasterJS {
private:
  std::unique_ptr<MasterJSImpl> pimpl;

  Hook<Doc*, int> hook;
  void hook_callback(Doc* doc, int type);

public:
  std::unordered_set<std::string> stored_menus;
  std::unordered_set<std::string> stored_objs;
  std::unordered_map<std::string, std::vector<std::string>> stored_events;

  MasterJS();
  ~MasterJS();

  void reboot_heap();
  std::string eval_string(const std::string& str);
  std::string eval_file(const std::string& path);
  void make_default_syntaxic_js_file();
  std::string get_syntaxic_js_file();
  std::string eval_syntaxic_js_file();
};

extern MasterJS* master_js;

#endif