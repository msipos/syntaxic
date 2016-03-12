#include "qtgui/glyph_store.hpp"

#include "utf8.h"
#include <QString>

GlyphInfo::GlyphInfo() : created(false) {}

GlyphCodePage::GlyphCodePage(int o) : offset(o) {
  glyphs.resize(GLYPH_CODE_PAGE_SIZE);
}

GlyphStore::GlyphStore(QFont font) : font_metrics(font), tab_width(2) {
  line_height = font_metrics.height();
  x_width = get_width('x');
}

void GlyphStore::create_glyph(int char_code, GlyphInfo& gi) {
  char str[5] = {0, 0, 0, 0, 0};
  utf8::append(char_code, str);
  QString qstr = str;
  gi.static_text = QStaticText(qstr);
  gi.width = font_metrics.width(qstr);
  gi.created = true;
}

GlyphInfo& GlyphStore::get_glyph_info(int char_code) {
  const int code_page = char_code / GLYPH_CODE_PAGE_SIZE;
  const int page_offset = code_page*GLYPH_CODE_PAGE_SIZE;
  const int offset = char_code - page_offset;

  for (auto& ptr: glyph_code_pages) {
    if (ptr->offset == page_offset) {
      GlyphInfo& gi = ptr->glyphs[offset];
      if (!gi.created) create_glyph(char_code, gi);
      if (gi.width == 0 && char_code != 0xFFFD) {
        return get_glyph_info(0xFFFD);
      }
      return gi;
    }
  }
  glyph_code_pages.push_back(std::unique_ptr<GlyphCodePage>(new GlyphCodePage(page_offset)));
  GlyphInfo& gi = glyph_code_pages.back()->glyphs[offset];
  if (!gi.created) create_glyph(char_code, gi);
  return gi;
}

int GlyphStore::get_width(int char_code) {
  const GlyphInfo& gi = get_glyph_info(char_code);
  if (gi.width == 0 && char_code != 0xFFFD) return get_width(0xFFFD);
  return gi.width;
}

QStaticText& GlyphStore::get_static_text(int char_code) {
  GlyphInfo& gi = get_glyph_info(char_code);
  if (gi.width == 0 && char_code != 0xFFFD) return get_static_text(0xFFFD);
  return gi.static_text;
}

void GlyphStore::set_font(QFont font) {
  font_metrics = QFontMetrics(font);
  line_height = font_metrics.height();
  glyph_code_pages.clear();
  x_width = get_width('x');
  GlyphInfo& gi = get_glyph_info('\t');
  gi.width = tab_width*x_width;
}

void GlyphStore::set_tab_width(int tw) {
  tab_width = tw;
  GlyphInfo& gi = get_glyph_info('\t');
  gi.width = tab_width*x_width;
}