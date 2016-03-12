#include "core/util.hpp"
#include "core/util_json.hpp"
#include "core/util_path.hpp"
#include "master.hpp"
#include "preferences.hpp"

#include <unordered_map>
#include <vector>
#include <QFontDatabase>
#include <QSettings>
#include <QStandardPaths>

class Prefs {
public:
  std::unordered_map<std::string, bool> bool_map;
  std::unordered_map<std::string, int> int_map;
  std::unordered_map<std::string, std::string> string_map;

  bool has_bool(const std::string& name) { return bool_map.count(name) > 0; }
  bool has_int(const std::string& name) { return int_map.count(name) > 0; }
  bool has_string(const std::string& name) { return string_map.count(name) > 0; }

  void set_bool(const std::string& name, bool b) { bool_map[name] = b; }
  void set_int(const std::string& name, int i) { int_map[name] = i; }
  void set_string(const std::string& name, const std::string& s) { string_map[name] = s; }

  void debug() {
    for (auto& p: bool_map) { printf("  %s : %d\n", p.first.c_str(), p.second); }
    for (auto& p: int_map) { printf("  %s : %d\n", p.first.c_str(), p.second); }
    for (auto& p: string_map) { printf("  %s : %s\n", p.first.c_str(), p.second.c_str()); }
  }
};

void load_json(const Json::Value value, Prefs& prefs);
Json::Value to_json(Prefs& prefs);

PrefManager::PrefManager() {
  default_prefs = std::unique_ptr<Prefs>(new Prefs);
  program_prefs = std::unique_ptr<Prefs>(new Prefs);
  temp_prefs = std::unique_ptr<Prefs>(new Prefs);
}

void PrefManager::init() {
  init_specs();
  init_defaults();

  program_config_name = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation).toStdString();
  program_config_name = UtilPath::join_components(program_config_name, "syntaxic_conf.json");
  //if (UtilPath::is_existing_file(program_config_name)) {
  //  load_program_settings();
  //}
  load_program_settings();
}

void PrefManager::debug() {
  printf("Temp:\n");
  temp_prefs->debug();
  printf("Program:\n");
  program_prefs->debug();
  printf("Default:\n");
  default_prefs->debug();
}

PrefManager::~PrefManager() {}

void PrefManager::init_defaults() {
  for (auto& cat: spec_categories) {
    for (auto& spec: cat.specs) {
      if (spec.type == PREF_BOOL) { default_prefs->set_bool(spec.var_name, spec.default_bool_value); }
      else if (spec.type == PREF_INT) { default_prefs->set_int(spec.var_name, spec.default_int_value); }
      else if (spec.type == PREF_CHOICE) {
        if (spec.choices.empty()) {
          printf("Warning: choice empty: %s\n", spec.var_name.c_str());
        } else if (spec.default_string_value.empty()) {
          default_prefs->set_string(spec.var_name, spec.choices[0]);
        } else {
          default_prefs->set_string(spec.var_name, spec.default_string_value);
        }
      } else if (spec.type == PREF_STRING) {
        default_prefs->set_string(spec.var_name, spec.default_string_value);
      } else if (spec.type == PREF_KEY) { default_prefs->set_string(spec.var_name, spec.default_string_value); }
    }
  }
}

void PrefManager::init_specs() {
  {
    PrefSpecCategory cat("appearance");
    cat.spec(PREF_FONT, "appearance.editor_font", "Editor font").long_text("Font used for the editor.");
    cat.spec(PREF_BOOL, "appearance.highlight_cursor_line", "Highlight cursor line").def_bool(true).long_text("Highlight the cursor line.");
    cat.spec(PREF_BOOL, "appearance.show_line_numbers", "Show line numbers").def_bool(true).long_text("Show line numbers to the left of the text editor.");
    spec_categories.push_back(cat);
  }

  {
    PrefSpecCategory cat("folding");
    cat.spec(PREF_INT, "folding.line_height", "Fold line height").def_int(4).min_max(0, 8).long_text("Height of the folded line preview in pixels.  If set to 0, then no preview will be shown.");
    cat.spec(PREF_INT, "folding.alpha", "Fold image opacity").def_int(40).min_max(0, 100).long_text("Opacity of the folded line preview. 0 means invisible, 100 fully opaque.");
    spec_categories.push_back(cat);
  }

  {
    PrefSpecCategory cat("indentation");
    cat.spec(PREF_BOOL, "indentation.use_spaces", "Use spaces for indentation").def_bool(true).long_text("Use spaces instead of tabs for indentation.");
    cat.spec(PREF_INT, "indentation.width", "Width of an indentation").def_int(4).min_max(0, 8).long_text("Indentation width.  If tabs are used, this is the width of a tab character.  If spaces are used, this is the number of spaces inserted when TAB key is pressed");
    spec_categories.push_back(cat);
  }

  {
    PrefSpecCategory cat("keys");
    cat.spec(PREF_KEY, "keys.home", "Home keys.").long_text("Keys to move to the beginning of line.")
    #ifdef CMAKE_WINDOWS
    .def_string("Home");
    #elif CMAKE_MACOSX
    .def_string("Home|Ctrl-A|Cmd-Left");
    #else
    .def_string("Home|Ctrl-A");
    #endif
    cat.spec(PREF_KEY, "keys.end", "End keys.").long_text("Keys to move to the end of the line.")
    #ifdef CMAKE_MACOSX
    .def_string("End|Ctrl-E|Cmd-Right");
    #else
    .def_string("End|Ctrl-E");
    #endif
    cat.spec(PREF_KEY, "keys.home-file", "Move to start of file.").long_text("Keys to move to start of file.")
    #ifdef CMAKE_MACOSX
    .def_string("Cmd-Up");
    #else
    .def_string("Ctrl-Home");
    #endif
    cat.spec(PREF_KEY, "keys.end-file", "Move end of file.").long_text("Keys to move to end of file.")
    #ifdef CMAKE_MACOSX
    .def_string("Cmd-Down");
    #else
    .def_string("Ctrl-End");
    #endif
    cat.spec(PREF_KEY, "keys.skip-left", "Skip word on left.").long_text("Keys to skip a word on the left.")
    #ifdef CMAKE_MACOSX
    .def_string("Alt-Left");
    #else
    .def_string("Ctrl-Left");
    #endif
    cat.spec(PREF_KEY, "keys.skip-right", "Skip word on right.").long_text("Keys to skip a word on the right.")
    #ifdef CMAKE_MACOSX
    .def_string("Alt-Right");
    #else
    .def_string("Ctrl-Right");
    #endif
    cat.spec(PREF_KEY, "keys.delete", "Delete forward keys.").def_string("Delete|Ctrl-D").long_text("Keys to delete forwards.");
    cat.spec(PREF_KEY, "keys.delete-word", "Delete word keys.").def_string("Ctrl-Backspace").long_text("Keys to delete word to the left.");

    // Edit menu

    cat.spec(PREF_KEY, "keys.menu.edit.undo", "Undo keys.").long_text("Keys for Undo operation.")
    #ifdef CMAKE_MACOSX
    .def_string("Cmd-Z");
    #else
    .def_string("Ctrl-Z");
    #endif
    cat.spec(PREF_KEY, "keys.menu.edit.cut", "Cut keys.").long_text("Keys for Cut operation.")
    #ifdef CMAKE_MACOSX
    .def_string("Cmd-X");
    #else
    .def_string("Ctrl-X");
    #endif
    cat.spec(PREF_KEY, "keys.menu.edit.copy", "Copy keys.").long_text("Keys for Copy operation.")
    #ifdef CMAKE_MACOSX
    .def_string("Cmd-C");
    #else
    .def_string("Ctrl-C");
    #endif
    cat.spec(PREF_KEY, "keys.menu.edit.paste", "Paste keys.").long_text("Keys for Paste operation.")
    #ifdef CMAKE_MACOSX
    .def_string("Cmd-V");
    #else
    .def_string("Ctrl-V");
    #endif
    cat.spec(PREF_KEY, "keys.menu.edit.kill-line", "Kill line keys.").long_text("Keys for Kill Line operation.")
    #ifdef CMAKE_MACOSX
    .def_string("Cmd-K");
    #else
    .def_string("Ctrl-K");
    #endif
    cat.spec(PREF_KEY, "keys.menu.edit.select-all", "Select all keys.").long_text("Keys for Select All operation.")
    #ifdef CMAKE_WINDOWS
    .def_string("Ctrl-A");
    #elif CMAKE_MACOSX
    .def_string("Cmd-Shift-A");
    #else
    .def_string("Ctrl-Shift-A");
    #endif
    cat.spec(PREF_KEY, "keys.menu.edit.select-word", "Select word keys.").long_text("Keys for Select Word operation.")
    #ifdef CMAKE_MACOSX
    .def_string("Cmd-R");
    #else
    .def_string("Ctrl-R");
    #endif

    // Document menu

    cat.spec(PREF_KEY, "keys.menu.document.switch-other-side", "Switch to other side.").long_text("Keys for 'Switch to other side' operation.")
    #ifdef CMAKE_MACOSX
    .def_string("Cmd-P");
    #else
    .def_string("Ctrl-P");
    #endif
    cat.spec(PREF_KEY, "keys.menu.document.move-other-side", "Move to other side.").long_text("Keys for 'Move document to other side' operation.")
    #ifdef CMAKE_MACOSX
    .def_string("Cmd-Shift-P");
    #else
    .def_string("Ctrl-Shift-P");
    #endif

    cat.spec(PREF_KEY, "keys.navigation.left", "Move left (navigation mode).").def_string("H").long_text("Key to move left in navigation mode");
    cat.spec(PREF_KEY, "keys.navigation.right", "Move right (navigation mode).").def_string("L").long_text("Key to move right in navigation mode");
    cat.spec(PREF_KEY, "keys.navigation.down", "Move down (navigation mode).").def_string("J").long_text("Key to move down in navigation mode");
    cat.spec(PREF_KEY, "keys.navigation.up", "Move up (navigation mode).").def_string("K").long_text("Key to move up in navigation mode");
    cat.spec(PREF_KEY, "keys.navigation.page_up", "Page up (navigation mode).").def_string("U").long_text("Key to move up a page in navigation mode");
    cat.spec(PREF_KEY, "keys.navigation.page_down", "Page down (navigation mode).").def_string("M").long_text("Key to move down a page in navigation mode");
    cat.spec(PREF_KEY, "keys.navigation.skip_left", "Skip left (navigation mode).").def_string("G").long_text("Key to skip left in navigation mode");
    cat.spec(PREF_KEY, "keys.navigation.skip_right", "Slip right (navigation mode).").def_string(";").long_text("Key to skip right in navigation mode");
    spec_categories.push_back(cat);
  }

  {
    PrefSpecCategory cat("loading");
    cat.spec(PREF_INT, "loading.big_file_limit", "Limit for big file mode").def_int(100).min_max(25, 10000).long_text("Limit (in KB) above which files will be opened in 'fast' mode (no syntax highlighting, faster text reflow).");
    spec_categories.push_back(cat);
  }

  {
    PrefSpecCategory cat("ruler");
    cat.spec(PREF_BOOL, "ruler.show", "Show the ruler").def_bool(true).long_text("Display the ruler. Ruler is a differently colored area to the right of the editor.");
    cat.spec(PREF_INT, "ruler.width", "Ruler width").def_int(100).min_max(40, 200).long_text("Column where the background color of the editor changes.");
    spec_categories.push_back(cat);
  }

  {
    PrefSpecCategory cat("saving");
    cat.spec(PREF_BOOL, "saving.trim_trailing_spaces", "Trim trailing whitespace when saving").def_bool(true).long_text("Delete all trailing whitespace when saving a file.");
    spec_categories.push_back(cat);
  }

  {
    PrefSpecCategory cat("sidebar");
    cat.spec(PREF_STRING, "sidebar.default_filter", "Default filter for file browsers").long_text("Files that match these globs will be filtered out in the sidebar.  Separate multiple globs with spaces.  For example \"*.pyc *.o\" (without quotes) will filter out all .pyc and all .o files.").def_string("");
    spec_categories.push_back(cat);
  }

  {
    PrefSpecCategory cat("splitscreen");
    cat.spec(PREF_BOOL, "splitscreen.manage_size", "Automatically manage size of editor panes").def_bool(true).long_text("Enabling this option will automatically resize panes when switching.");
    cat.spec(PREF_INT, "splitscreen.overflow_size", "How much active pane overflows into inactive one").def_int(0).min_max(0, 500).long_text("This option is only used if 'splitscreen.manage_size' is enabled.  It defines the number of pixels that the active pane overflows into inactive pane.");
    spec_categories.push_back(cat);
  }

  {
    PrefSpecCategory cat("theme");
    PrefSpec& spec = cat.spec(PREF_CHOICE, "theme.editor_theme", "Editor theme").long_text("Coloring theme for the editor.");
    for (auto& theme : master.theme_engine.available_themes) {
      spec.choice(theme);
    }
    {
      PrefSpec& spec = cat.spec(PREF_CHOICE, "theme.shell_theme", "Shell theme").long_text("Coloring theme for the shell.");
      for (auto& theme : master.theme_engine.available_themes) {
        spec.choice(theme);
      }
      spec.def_string("Shell");
    }
    cat.spec(PREF_CHOICE, "theme.gui_theme", "GUI theme").long_text("Coloring theme for the user interface (except for the editor).  Note: you must restart the editor to apply the GUI theme changes.").choice("Dark").choice("Native");
    spec_categories.push_back(cat);
  }

  {
    PrefSpecCategory cat("wrapping");
    cat.spec(PREF_BOOL, "wrapping.wrap", "Enable word wrap").def_bool(true).long_text("Wrap text in the editor.");
    cat.spec(PREF_INT, "wrapping.indent", "Indentation for wrapped lines").def_int(0).min_max(0, 16).long_text("Indentation, in number of characters, that is applied to lines wrapped with word wrap.");
    cat.spec(PREF_BOOL, "wrapping.draw_guides", "Draw word wrap guides").def_bool(true).long_text("Draw guide (vertical lines) that indicate where line wrapping occurs.");
    spec_categories.push_back(cat);
  }
}

PrefSpec* PrefManager::find_pref_spec(const std::string& name) {
  for (PrefSpecCategory& category: spec_categories) {
    for (PrefSpec& spec: category.specs) {
      if (spec.var_name == name) return &spec;
    }
  }
  return nullptr;
}

void PrefManager::load_program_settings() {
  // TODO: For the time we are not using JSON here.
  //std::vector<char> contents;
  //read_file(contents, program_config_name);
  //Json::Value json = parse_json(contents.data(), contents.size());
  //load_json(json, *program_prefs);

  QSettings qs;
  qs.beginGroup("preferences");

  for (auto& cat: spec_categories) {
    for (auto& spec: cat.specs) {
      if (!qs.contains(QString::fromStdString(spec.var_name))) continue;

      QVariant variant = qs.value(QString::fromStdString(spec.var_name));
      if (spec.type == PREF_BOOL) {
        program_prefs->bool_map[spec.var_name] = variant.toBool();
      } else if (spec.type == PREF_INT) {
        program_prefs->int_map[spec.var_name] = variant.toInt();
      } else if (spec.type == PREF_STRING || spec.type == PREF_CHOICE || spec.type == PREF_KEY) {
        program_prefs->string_map[spec.var_name] = variant.toString().toStdString();
      }
    }
  }

  // Fonts have to be specially treated:
  for (auto& cat: spec_categories) {
    for (auto& spec: cat.specs) {
      if (spec.type == PREF_FONT) {
        std::string face_name = spec.var_name + ".face";
        program_prefs->string_map[face_name] = qs.value(QString::fromStdString(face_name)).toString().toStdString();
        std::string size_name = spec.var_name + ".size";
        program_prefs->int_map[size_name] = qs.value(QString::fromStdString(size_name)).toInt();
      }
    }
  }

  qs.endGroup();
}

void load_json(const Json::Value json_value, Prefs& prefs) {
  for (unsigned int i = 0; i < json_value.size(); i++) {
    const Json::Value json_prop = json_value[i];

    std::string prop_name = json_prop["name"].asString();
    std::string prop_type = json_prop["type"].asString();

    if (prop_type == "bool") {
      bool b = json_prop["value"].asBool();
      prefs.bool_map[prop_name] = b;
    } else if (prop_type == "int") {
      int i = json_prop["value"].asInt();
      prefs.int_map[prop_name] = i;
    }
  }
}

void PrefManager::start_setting() { /* Ignore. */ }

void PrefManager::end_setting() {
  // Save to JSON:

  // Serialize to JSON:
  //Json::Value json = to_json(*program_prefs);
  //std::string contents = json.toStyledString();
  //write_file(contents.c_str(), contents.size(), program_config_name);

  // Move temp prefs into program prefs
  for (auto& p : temp_prefs->bool_map) {
    program_prefs->bool_map[p.first] = p.second;
  }
  for (auto& p : temp_prefs->int_map) {
    program_prefs->int_map[p.first] = p.second;
  }
  for (auto& p : temp_prefs->string_map) {
    program_prefs->string_map[p.first] = p.second;
  }
  clear_temp();

  // Record program prefs
  QSettings qs;
  qs.beginGroup("preferences");

  for (auto& p : program_prefs->bool_map) {
    qs.setValue(QString::fromStdString(p.first), QVariant(p.second));
  }
  for (auto& p : program_prefs->int_map) {
    qs.setValue(QString::fromStdString(p.first), QVariant(p.second));
  }
  for (auto& p : program_prefs->string_map) {
    qs.setValue(QString::fromStdString(p.first), QVariant(QString::fromStdString(p.second)));
  }

  qs.endGroup();

  // Notify that preferences have been updated.
  change_hook.call();
}

Json::Value to_json(Prefs& prefs) {
  Json::Value root(Json::arrayValue);

  for (auto& p: prefs.bool_map) {
    Json::Value prop(Json::objectValue);
    prop["name"] = p.first;
    prop["type"] = "bool";
    prop["value"] = p.second;
    root.append(prop);
  }

  for (auto& p: prefs.int_map) {
    Json::Value prop(Json::objectValue);
    prop["name"] = p.first;
    prop["type"] = "int";
    prop["value"] = p.second;
    root.append(prop);
  }

  return root;
}

bool PrefManager::get_bool(const std::string& name) const {
  if (temp_prefs->has_bool(name)) return temp_prefs->bool_map[name];
  if (program_prefs->has_bool(name)) return program_prefs->bool_map[name];
  if (default_prefs->has_bool(name)) return default_prefs->bool_map[name];
  printf("Warning: asked for unknown '%s' bool.\n", name.c_str());
  return false;
}

int PrefManager::get_int(const std::string& name) const {
  if (temp_prefs->has_int(name)) return temp_prefs->int_map[name];
  if (program_prefs->has_int(name)) return program_prefs->int_map[name];
  if (default_prefs->has_int(name)) return default_prefs->int_map[name];
  printf("Warning: asked for unknown '%s' int.\n", name.c_str());
  return 0;
}

std::string PrefManager::get_string(const std::string& name) const {
  if (temp_prefs->has_string(name)) return temp_prefs->string_map[name];
  if (program_prefs->has_string(name)) return program_prefs->string_map[name];
  if (default_prefs->has_string(name)) return default_prefs->string_map[name];
  printf("Warning: asked for unknown '%s' string.\n", name.c_str());
  return "";
}

std::string PrefManager::get_key(const std::string& name) const {
  if (temp_prefs->has_string(name)) return temp_prefs->string_map[name];
  if (program_prefs->has_string(name)) return program_prefs->string_map[name];
  if (default_prefs->has_string(name)) return default_prefs->string_map[name];
  printf("Warning: asked for unknown key '%s'.\n", name.c_str());
  return "";
}

void PrefManager::clear_temp() {
  temp_prefs->bool_map.clear();
  temp_prefs->int_map.clear();
  temp_prefs->string_map.clear();
}

void PrefManager::set_bool(const std::string& name, bool b) { temp_prefs->set_bool(name, b); }
void PrefManager::set_int(const std::string& name, int i) { temp_prefs->set_int(name, i); }
void PrefManager::set_string(const std::string& name, const std::string& s) { temp_prefs->set_string(name, s); }
void PrefManager::set_key(const std::string& name, const std::string& key) { temp_prefs->set_string(name, key); }

// Fonts are different, they are kept as a (string, int) tuple.

QFont PrefManager::get_font(const std::string& name) const {
  std::string font_face = get_string(name + ".face");
  if (font_face.empty()) {
    QFont std_font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font_face = std_font.family().toStdString();
  }
  int font_size = get_int(name + ".size");
  if (font_size == 0) {
    QFont std_font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font_size = std_font.pointSize();
  }
  if (font_size < 4 || font_size > 25) font_size = 10;
  return QFont(QString::fromStdString(font_face), font_size);
}

void PrefManager::set_font(const std::string& name, QFont font) {
  set_string(name + ".face", font.family().toStdString());
  set_int(name + ".size", font.pointSize());
}

// This is a little special

int PrefManager::get_tabdef() {
  int tab_width = get_int("indentation.width");
  bool spaces = get_bool("indentation.use_spaces");
  return spaces ? tab_width : -tab_width;
}