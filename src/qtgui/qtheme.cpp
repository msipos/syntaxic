#include "master.hpp"
#include "qtgui/qtheme.hpp"

QColor rgba_to_qcolor(uint32_t rgba) {
  return QColor((rgba & 0xFF000000) >> 24,
                (rgba & 0x00FF0000) >> 16,
                (rgba & 0x0000FF00) >> 8,
                (rgba & 0x000000FF));
}

QTheme::QTheme() {
  Theme theme = master.theme_engine.get_default_theme();
  for (int i = 0; i < 16; i++) {
    text_colors[i] = rgba_to_qcolor(theme.text_colors[i]);
    selected_text_colors[i] = rgba_to_qcolor(theme.selected_text_colors[i]);
    text_fonts[i] = theme.text_fonts[i];
  }

  cursor_color = rgba_to_qcolor(theme.cursor_color);
  bg_color = rgba_to_qcolor(theme.bg_color);
  ruler_color = rgba_to_qcolor(theme.ruler_color);
  cursor_highlight_bg_color = rgba_to_qcolor(theme.cursor_highlight_bg_color);
  selected_bg_color = rgba_to_qcolor(theme.selected_bg_color);
  highlight_text_color = rgba_to_qcolor(theme.highlight_text_color);
  highlight_bg_color = rgba_to_qcolor(theme.highlight_bg_color);
  overlay_line_color = rgba_to_qcolor(theme.overlay_line_color);
  overlay_text_color = rgba_to_qcolor(theme.overlay_text_color);
  gutter_bg_color = rgba_to_qcolor(theme.gutter_bg_color);
  gutter_text_color = rgba_to_qcolor(theme.gutter_text_color);
}

QTheme::QTheme(const Theme& theme) {
  for (int i = 0; i < 16; i++) {
    text_colors[i] = rgba_to_qcolor(theme.text_colors[i]);
    selected_text_colors[i] = rgba_to_qcolor(theme.selected_text_colors[i]);
    text_fonts[i] = theme.text_fonts[i];
  }

  cursor_color = rgba_to_qcolor(theme.cursor_color);
  bg_color = rgba_to_qcolor(theme.bg_color);
  ruler_color = rgba_to_qcolor(theme.ruler_color);
  cursor_highlight_bg_color = rgba_to_qcolor(theme.cursor_highlight_bg_color);
  selected_bg_color = rgba_to_qcolor(theme.selected_bg_color);
  highlight_text_color = rgba_to_qcolor(theme.highlight_text_color);
  highlight_bg_color = rgba_to_qcolor(theme.highlight_bg_color);
  overlay_line_color = rgba_to_qcolor(theme.overlay_line_color);
  overlay_text_color = rgba_to_qcolor(theme.overlay_text_color);
  gutter_bg_color = rgba_to_qcolor(theme.gutter_bg_color);
  gutter_text_color = rgba_to_qcolor(theme.gutter_text_color);
}