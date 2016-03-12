#ifndef SYNTAXIC_THEME_HPP
#define SYNTAXIC_THEME_HPP

#include <cstdint>
#include <string>
#include <vector>

// RGBA format, i.e. 0xRRGGBBAA

struct Theme {
  uint32_t text_colors[16];
  uint32_t selected_text_colors[16];
  int text_fonts[16];

  uint32_t cursor_color;
  uint32_t bg_color;
  uint32_t ruler_color;
  uint32_t cursor_highlight_bg_color;
  uint32_t selected_bg_color;
  uint32_t highlight_text_color;
  uint32_t highlight_bg_color;
  uint32_t overlay_line_color;
  uint32_t overlay_text_color;
  uint32_t gutter_bg_color;
  uint32_t gutter_text_color;
};

class ThemeEngine {
public:
  std::vector<std::string> available_themes;

  void init_theme_engine(const char* json_contents, unsigned int json_size);
  Theme get_theme(const std::string& theme_name);
  /** If all fails, use this theme. */
  Theme get_default_theme();
};

#endif