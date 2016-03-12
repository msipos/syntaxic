#ifndef QTGUI_GLYPH_STORE_HPP
#define QTGUI_GLYPH_STORE_HPP

#include <memory>
#include <QFont>
#include <QFontMetrics>
#include <QStaticText>
#include <vector>

class GlyphInfo {
public:
  bool created;
  int width;
  QStaticText static_text;

  GlyphInfo();
};

class GlyphCodePage {
public:
  int offset;
  std::vector<GlyphInfo> glyphs;

  GlyphCodePage(int o);
};

#define GLYPH_CODE_PAGE_SIZE 256

/** An optimization object that returns QStaticText for each glyph. */
class GlyphStore {
private:
  QFontMetrics font_metrics;
  int line_height;
  int tab_width;
  int x_width;

  std::vector<std::unique_ptr<GlyphCodePage>> glyph_code_pages;
  void create_glyph(int char_code, GlyphInfo& gi);
  GlyphInfo& get_glyph_info(int char_code);

public:
  GlyphStore(QFont font);

  int get_width(int char_code);
  QStaticText& get_static_text(int char_code);
  void set_font(QFont font);
  inline int get_line_height() { return line_height; }
  void set_tab_width(int tw);
  inline int get_x_width() { return x_width; }
};

#endif