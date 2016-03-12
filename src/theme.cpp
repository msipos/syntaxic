#include "core/util.hpp"
#include "theme.hpp"

#include "json/json.h"
#include <stdexcept>

void ThemeEngine::init_theme_engine(const char* json_contents, unsigned int json_size) {
  Json::Reader json_reader;
  Json::Value json_root;
  bool parsing_successful = json_reader.parse(json_contents, json_contents+json_size, json_root);

  if (!parsing_successful) { throw std::runtime_error("Could not parse theme information"); }
  if (json_root.type() != Json::objectValue) {
    throw std::runtime_error("Could not parse theme information");
  }

  Json::Value themes = json_root["themes"];
  for (unsigned int i = 0; i < themes.size(); i++) {
    std::string theme = themes[i].asString();
    available_themes.push_back(theme);
  }

  Theme theme = get_theme("Blue");
}

uint32_t string_to_color(const std::string& s) {
  return std::stoul(s, nullptr, 16);
}

Theme ThemeEngine::get_theme(const std::string& theme_name) {
  std::string theme_file = under_root2("themes", theme_name + ".json");
  std::vector<char> contents;
  read_file(contents, theme_file);
  Json::Reader json_reader;
  Json::Value json_root;
  bool parsing_successful = json_reader.parse(contents.data(), contents.data() + contents.size(),
      json_root, false);

  if (!parsing_successful) { throw std::runtime_error("Could not parse theme " + theme_name +
      json_reader.getFormattedErrorMessages()); }
  if (json_root.type() != Json::objectValue) {
    throw std::runtime_error("Could not parse theme " + theme_name +
        json_reader.getFormattedErrorMessages());
  }

  Theme theme;
  {
    Json::Value syntax_root = json_root["syntax"];
    for (int i = 0; i < 16; i++) {
      Json::Value arr = syntax_root[i];
      theme.text_colors[i] = string_to_color(arr[0].asString());
      theme.selected_text_colors[i] = string_to_color(arr[1].asString());
      theme.text_fonts[i] = arr[2].asInt();
    }
  }
  theme.cursor_color = string_to_color(json_root["cursor_color"].asString());
  theme.bg_color = string_to_color(json_root["bg_color"].asString());
  theme.cursor_highlight_bg_color = string_to_color(json_root["cursor_highlight_bg_color"].asString());
  theme.ruler_color = string_to_color(json_root["ruler_color"].asString());
  theme.selected_bg_color = string_to_color(json_root["selected_bg_color"].asString());
  theme.highlight_text_color = string_to_color(json_root["highlight_text_color"].asString());
  theme.highlight_bg_color = string_to_color(json_root["highlight_bg_color"].asString());
  theme.overlay_line_color = string_to_color(json_root["overlay_line_color"].asString());
  theme.overlay_text_color = string_to_color(json_root["overlay_text_color"].asString());
  theme.gutter_bg_color = string_to_color(json_root["gutter_bg_color"].asString());
  theme.gutter_text_color = string_to_color(json_root["gutter_text_color"].asString());
  return theme;
}

Theme ThemeEngine::get_default_theme() {
  Theme th;
  th.text_colors[0] = 0xD0D0D0FF;  // none
  th.text_colors[1] = 0xD0D0D0FF;  // identifier
  th.text_colors[2] = 0xAD7FA8FF;  // keyword
  th.text_colors[3] = 0x33E2E2FF;  // other keyword
  th.text_colors[4] = 0xFFFFFFFF;  // operator
  th.text_colors[5] = 0x0070EFFF;  // preprocessor
  th.text_colors[6] = 0xFFFFFFFF;  // important identifier
  th.text_colors[7] = 0x30BC08FF;  // string
  th.text_colors[8] = 0xA08040FF;  // number
  th.text_colors[9] = 0x0080FFFF;  // comment
  th.text_colors[10] = 0xFF0000FF; // error token
  th.text_colors[11] = 0xFFFFFFFF;

  th.selected_text_colors[0] = 0xFFFFFFFF;  // none
  th.selected_text_colors[1] = 0xFFFFFFFF;  // identifier
  th.selected_text_colors[2] = 0xCD9FC8FF;  // keyword
  th.selected_text_colors[3] = 0x55FFFFFF;  // other keyword
  th.selected_text_colors[4] = 0xFFFFFFFF;  // operator
  th.selected_text_colors[5] = 0x00308FFF;  // preprocessor
  th.selected_text_colors[6] = 0xFFFFFFFF;  // important identifier
  th.selected_text_colors[7] = 0x60EF30FF;  // string
  th.selected_text_colors[8] = 0x784141FF;  // number
  th.selected_text_colors[9] = 0x00308FFF;  // comment
  th.selected_text_colors[10] = 0xFF0000FF; // error token
  th.selected_text_colors[11] = 0xFFFFFFFF;

  th.text_fonts[0] = 0;  // none
  th.text_fonts[1] = 0;  // identifier
  th.text_fonts[2] = 1;  // keyword
  th.text_fonts[3] = 0;  // other keyword
  th.text_fonts[4] = 0;  // operator
  th.text_fonts[5] = 0;  // preprocessor
  th.text_fonts[6] = 0;  // important identifier
  th.text_fonts[7] = 0;  // string
  th.text_fonts[8] = 0;  // number
  th.text_fonts[9] = 2;  // comment
  th.text_fonts[10] = 0; // error token
  th.text_fonts[11] = 0;

  th.cursor_color = 0xFFFFFFFF;
  th.bg_color = 0x003040FF;
  th.ruler_color = 0x083848FF;
  th.cursor_highlight_bg_color = 0x104050FF;
  th.selected_bg_color = 0x0090FFFF;
  th.highlight_text_color = 0xFFFFFFFF;
  th.highlight_bg_color = 0x006090FF;
  th.overlay_line_color = 0xC03030FF;
  th.overlay_text_color = 0xFF5050FF;
  th.gutter_bg_color = 0x001F29FF;
  th.gutter_text_color = 0x757575FF;
  return th;
}