#ifndef SYNTAXIC_PREFERENCES_HPP
#define SYNTAXIC_PREFERENCES_HPP

#include "core/hooks.hpp"

#include <memory>
#include <string>
#include <vector>
#include <QFont>

class Prefs;

enum PrefType {
  PREF_SPACE, PREF_BOOL, PREF_STRING, PREF_INT, PREF_FONT, PREF_CHOICE, PREF_KEY
};

struct PrefSpec {
  PrefType type;
  std::string var_name;
  std::string user_text;
  std::string user_long_text;
  std::string user_note;

  int min_int, max_int;

  int default_int_value;
  bool default_bool_value;
  std::string default_string_value;

  std::vector<std::string> choices;

  inline PrefSpec() : type(PREF_SPACE) {}
  inline PrefSpec(PrefType t, const std::string& name, const std::string& text) : type(t),
      var_name(name), user_text(text), default_int_value(0), default_bool_value(0) {}

  inline PrefSpec& note(const std::string& s) { user_note = s; return *this; }
  inline PrefSpec& min_max(int imin, int imax) { min_int = imin; max_int = imax; return *this; }
  inline PrefSpec& long_text(const std::string& s) { user_long_text = s; return *this; }
  inline PrefSpec& def_int(int i) { default_int_value = i; return *this; }
  inline PrefSpec& def_bool(bool b) { default_bool_value = b; return *this; }
  inline PrefSpec& def_string(const char* str) { default_string_value = str; return *this; }
  inline PrefSpec& choice(const std::string& s) { choices.push_back(s); return *this; }
};

class PrefSpecCategory {
public:
  std::string name;
  std::vector<PrefSpec> specs;

  inline PrefSpecCategory(const std::string& s) : name(s) {}

  inline PrefSpec& spec(PrefType t, const std::string& name, const std::string& text) {
    specs.push_back(PrefSpec(t, name, text)); return specs.back();
  }
};

class PrefManager {
private:
  std::string program_config_name;

  std::unique_ptr<Prefs> default_prefs;
  std::unique_ptr<Prefs> program_prefs;
  std::unique_ptr<Prefs> temp_prefs;

  void init_defaults();
  void init_specs();
  void load_program_settings();

public:
  PrefManager();
  ~PrefManager();

  HookSource<> change_hook;

  void init();
  void debug();

  bool get_bool(const std::string& name) const;
  int get_int(const std::string& name) const;
  std::string get_string(const std::string& name) const;
  QFont get_font(const std::string& font) const;
  std::string get_key(const std::string& name) const;
  int get_tabdef();

  void start_setting();

  void set_bool(const std::string& name, bool b);
  void set_int(const std::string& name, int i);
  void set_string(const std::string& name, const std::string& s);
  void set_font(const std::string& name, QFont font);
  void set_key(const std::string& name, const std::string& key);

  /** Save the temporary preferences into real preferences and emit a notification. */
  void end_setting();
  /** Don't save temporary preferences, instead cancel them. */
  void clear_temp();

  std::vector<PrefSpecCategory> spec_categories;
  PrefSpec* find_pref_spec(const std::string& name);
};

#endif