#ifndef SYNTAXIC_QTGUI_QTHEME_HPP
#define SYNTAXIC_QTGUI_QTHEME_HPP

#include "theme.hpp"

#include <QColor>

struct QTheme {
  QColor text_colors[16];
  QColor selected_text_colors[16];
  int text_fonts[16];

  QColor cursor_color;
  QColor bg_color;
  QColor ruler_color;
  QColor cursor_highlight_bg_color;
  QColor selected_bg_color;
  QColor highlight_text_color;
  QColor highlight_bg_color;
  QColor overlay_line_color;
  QColor overlay_text_color;
  QColor gutter_bg_color;
  QColor gutter_text_color;

  QTheme();
  QTheme(const Theme& theme);
};

#endif