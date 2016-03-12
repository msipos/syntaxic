#include "core/rich_text.hpp"
#include "core/utf8_util.hpp"
#include "core/util.hpp"
#include "master.hpp"
#include "qtgui/editor.hpp"
#include "qtgui/main_window.hpp"

#include <cstdlib>
#include <QApplication>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QPaintEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QStaticText>
#include <QTimer>

MyScrollArea::MyScrollArea(QWidget* parent, Editor* e) : QScrollArea(parent), editor(e) {
  setFocusPolicy(Qt::NoFocus);
  setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
  setWidgetResizable(true);
}

void MyScrollArea::resizeEvent(QResizeEvent* event) {
  editor->reflow();
  QScrollArea::resizeEvent(event);
}

void MyScrollArea::page_up() {
  QScrollBar* bar = verticalScrollBar();
  bar->setValue(bar->value() - bar->pageStep());
}

void MyScrollArea::page_down() {
  QScrollBar* bar = verticalScrollBar();
  bar->setValue(bar->value() + bar->pageStep());
}

void MyScrollArea::get_visible_extent(int& x1, int& x2, int& y1, int& y2) {
  QScrollBar* bar = horizontalScrollBar();
  x1 = bar->value();
  x2 = bar->value() + bar->pageStep();
  bar = verticalScrollBar();
  y1 = bar->value();
  y2 = bar->value() + bar->pageStep();
}

Editor::Editor(QWidget* parent, MainWindow* mw) : QWidget(parent), main_window(mw), document(nullptr), cursor_visible(true), last_blink_time(0), navigation_mode(false), glyph_store(font), glyph_store_bold(bold_font), glyph_store_italic(italic_font), fold_line_height(4), fold_alpha(100), ruler_width(100), line_numbers_visible(true), highlight_cursor_line(true), word_wrap(true), draw_word_wrap_guides(true), editor_width(0), editor_height(0) {
  q_timer = new QTimer(this);
  connect(q_timer, &QTimer::timeout, this, &Editor::slot_timer);
  q_timer->start(100);

  scroll_area = new MyScrollArea(parent, this);

  setFocusPolicy(Qt::StrongFocus);

  q_navmode_bitmap = QBitmap(":resources/navmode.png");
  q_up_bitmap = QBitmap(":resources/up.png");
  q_down_bitmap = QBitmap(":resources/down.png");
  preferences_hook = master.pref_manager.change_hook.add(
      std::bind(&Editor::preferences_callback, this));

  scroll_area->setWidget(this);

  reload_settings();
}

const QImage& Editor::draw_fold_image(int row, const Line& line) {
  int width = flow_grid.map_to_x(row, line.size());

  for (const FoldImageCache& c: fold_images) {
    if (c.line == row) return c.image;
  }

  QImage image(width, glyph_store.get_line_height()-4, QImage::Format_ARGB32_Premultiplied);
  image.fill(0x00000000);
  {
    QPainter painter(&image);

    for (unsigned int i = 0; i < line.size(); i++) {
      const Character character = line.get_char(i);
      const int char_code = character.c;
      const int x = flow_grid.map_to_x(row, i);
      const int y = -2;
      QColor color = editor_theme.text_colors[character.markup % 16];
      color.setAlpha(fold_alpha);
      const int fh = editor_theme.text_fonts[character.markup % 16];
      painter.setPen(color);

      if (fh == 1) {
        painter.setFont(bold_font);
        painter.drawStaticText(x, y, glyph_store_bold.get_static_text(char_code));
      } else if (fh == 2) {
        painter.setFont(italic_font);
        painter.drawStaticText(x, y, glyph_store_italic.get_static_text(char_code));
      } else {
        painter.setFont(font);
        painter.drawStaticText(x, y, glyph_store.get_static_text(char_code));
      }
    }
  }
  QImage rv = image.scaled(width, fold_line_height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
  fold_images.push_back({row, rv});
  return fold_images.back().image;
}

void Editor::preferences_callback() {
  reload_settings();
}

void Editor::reload_settings() {
  font = master.pref_manager.get_font("appearance.editor_font");
  int tab_width = master.pref_manager.get_int("indentation.width");
  ruler_show = master.pref_manager.get_bool("ruler.show");
  ruler_width = master.pref_manager.get_int("ruler.width");
  line_numbers_visible = master.pref_manager.get_bool("appearance.show_line_numbers");
  highlight_cursor_line = master.pref_manager.get_bool("appearance.highlight_cursor_line");
  word_wrap = master.pref_manager.get_bool("wrapping.wrap");
  draw_word_wrap_guides = master.pref_manager.get_bool("wrapping.draw_guides");
  {
    std::string theme_name = master.pref_manager.get_string("theme.editor_theme");
    try {
      editor_theme = QTheme(master.theme_engine.get_theme(theme_name));
    } catch (...) {
      printf("WARNING: Failed to load theme %s\n", theme_name.c_str());
      editor_theme = master.theme_engine.get_default_theme();
    }
  }
  {
    std::string theme_name = master.pref_manager.get_string("theme.shell_theme");
    try {
      shell_theme = QTheme(master.theme_engine.get_theme(theme_name));
    } catch (...) {
      printf("WARNING: Failed to load theme %s\n", theme_name.c_str());
      shell_theme = master.theme_engine.get_default_theme();
    }
  }

  glyph_store.set_font(font);
  glyph_store.set_tab_width(tab_width);

  bold_font = font;
  bold_font.setBold(true);
  glyph_store_bold.set_font(bold_font);
  glyph_store.set_tab_width(tab_width);

  italic_font = font;
  italic_font.setItalic(true);
  glyph_store_bold.set_font(italic_font);
  glyph_store.set_tab_width(tab_width);

  fold_line_height = master.pref_manager.get_int("folding.line_height");
  fold_alpha = int(master.pref_manager.get_int("folding.alpha")*255.0 / 100.0);

  flow_grid.x_width = glyph_store.get_x_width();
  flow_grid.tab_width = tab_width * flow_grid.x_width;
  flow_grid.line_height = glyph_store.get_line_height();
  flow_grid.folded_line_height = fold_line_height;
  flow_grid.word_wrap_indent = master.pref_manager.get_int("wrapping.indent");

  // This will refresh a lot of things.
  set_document(document);
  update();
}

QSize Editor::sizeHint() const { return QSize(editor_width, editor_height); }
QSize Editor::minimumSizeHint() const { return sizeHint(); }
void Editor::ensure_visible(int x, int y, int size) {
  scroll_area->ensureVisible(x, y, 1, size);

  // Map to window coordinates.
  QPoint pt = mapToGlobal(QPoint(x, y));
  main_window->editor_cursor_moved(pt.x(), pt.y(), size);
}

void Editor::reflow() {
  if (document == nullptr) {
    editor_width = 300;
    editor_height = 300;
    return;
  }

  // May be used later
  const int old_scroll_y = cursor_position_y - scroll_area->verticalScrollBar()->value();

  flow_grid.text_buffer = document->get_text_buffer();
  flow_grid.x_offset = compute_x_offset();
  if (word_wrap) {
    flow_grid.word_wrap_width = scroll_area->viewport()->size().width();
  } else {
    flow_grid.word_wrap_width = -1;
  }
  flow_grid.fast_reflow = (document->get_display_style() & DocFlag::BIG_DOC);
  flow_grid.reflow();
  editor_width = flow_grid.output_width;
  editor_height = flow_grid.output_height;

  // Notify Qt that size has changed
  updateGeometry();
  adjustSize();

  // Recompute cursor pixel on every reflow.
  compute_cursor_pixels();

  // This reflow has happened outside of focus.  This means we should restore cursor position to its rightful place.
  if (!hasFocus()) {
    scroll_area->verticalScrollBar()->setValue(cursor_position_y - old_scroll_y);
  }
}

void Editor::set_document(Doc* d) {
  document = d;
  reflow();

  fold_images.clear();
  if (d == nullptr) {
    unsetCursor();
    return;
  }

  // Deal with cursor
  reset_blink();

  // Add document hook
  document_hook = d->get_doc_hook()->add(
      std::bind(&Editor::document_callback, this, std::placeholders::_1,
      std::placeholders::_2));

  // Update cursor to IBeam.
  setCursor(Qt::IBeamCursor);
}

void Editor::set_navigation_mode(bool nm) {
  navigation_mode = nm;
  update();
}

void Editor::document_callback(Doc* /* doc */, int flag) {
  if (flag & DocEvent::EDITED) {
    fold_images.clear();
    reflow();
    updateGeometry();
    adjustSize();
    update();
  }

  if (flag & DocEvent::PAGE_UP) {
    scroll_area->page_up();
  } else if (flag & DocEvent::PAGE_DOWN) {
    scroll_area->page_down();
  }

  if (flag & DocEvent::FOLDED) {
    reflow();
    update();
    updateGeometry();
    adjustSize();
  }

  if (flag & DocEvent::CURSOR_MOVED) {
    compute_cursor_pixels();
    update();
  }

  if (flag & DocEvent::CENTRALIZE) {
    ensure_visible(cursor_position_x, cursor_position_y + cursor_size/2, cursor_size*15);
  }
}

void Editor::compute_cursor_pixels() {
  cursor_size = glyph_store.get_line_height();
  if (document == nullptr) {
    cursor_position_x = 0;
    cursor_position_y = 0;
  } else {
    CursorLocation cl = document->get_cursor();
    int x0 = compute_x_offset();
    flow_grid.x_offset = x0;
    cursor_position_x = flow_grid.map_to_x(cl.row, cl.col);
    cursor_position_y = flow_grid.map_to_y(cl.row, cl.col);

    document->get_appendage().cursor_x = cursor_position_x;
    document->get_appendage().cursor_y = cursor_position_y;
  }
  ensure_visible(cursor_position_x, cursor_position_y + cursor_size/2, cursor_size/2);
}

#define LINE_NUMBER_SPACE 4

int Editor::compute_x_offset() {
  if (document == nullptr) return 0;
  int max_line = document->get_text_buffer()->get_num_lines() + 1;
  int x0 = 0;
  if (line_numbers_visible && (document->get_display_style() & DocFlag::SHELL_THEME) == 0) {
    int num_digits = 0;
    while (max_line > 0) {
      max_line /= 10; num_digits++;
    }
    const int width9 = glyph_store.get_width('9');
    x0 = 2 + LINE_NUMBER_SPACE + width9*num_digits;
  }
  return x0;
}

void Editor::reset_blink() {
  last_blink_time = 0.0;
  slot_timer();
}

void Editor::slot_timer() {
  double timestamp = get_timestamp();
  const double period = navigation_mode ? 0.5 : 2;
  const bool last_cursor_visible = cursor_visible;
  if (!hasFocus()) {
    // Disable blinking when not in focus.
    cursor_visible = true;
  } else {
    if (timestamp - last_blink_time > period) {
      last_blink_time = timestamp;
      cursor_visible = true;
    } else if (timestamp - last_blink_time > period / 2.0) {
      cursor_visible = false;
    }
  }
  if (cursor_visible != last_cursor_visible) {
    update(cursor_position_x-4, cursor_position_y, 8, cursor_size);
  }
}

int Editor::line_height() { return glyph_store.get_line_height(); }

// Make sure Tab and BackTab (shift+tab) keys are processed in keyPressEvent as they are stolen by Qt.
bool Editor::event(QEvent* e) {
  if (e && e->type() == QEvent::KeyPress) {
    QKeyEvent * keyEvent = dynamic_cast<QKeyEvent*>(e);
    if (keyEvent && (keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab)) {
      keyPressEvent(keyEvent);
      return true;
    }
  }
  return QWidget::event(e);
}

void Editor::keyReleaseEvent(QKeyEvent* event) {
  event->accept();
  int key = event->key();
  if (key == Qt::Key_Control) {
    setCursor(Qt::IBeamCursor);
    setMouseTracking(false);
  }
}

void Editor::keyPressEvent(QKeyEvent* event) {
  event->accept();

  if (document == nullptr) return;

  // Set VisualPayload for this document
  {
    QScrollBar* bar = scroll_area->verticalScrollBar();
    const int page_size = bar->pageStep() / glyph_store.get_line_height();
    document->set_visual_payload(VisualPayload(page_size, &flow_grid, navigation_mode));
  }

  reset_blink();

  int key = event->key();

  if (key == Qt::Key_Control) {
    setCursor(Qt::PointingHandCursor);
    setMouseTracking(true);
    return;
  }

  Qt::KeyboardModifiers km = event->modifiers();
  bool alt = km & Qt::AltModifier;
  bool ctrl = km & Qt::ControlModifier;
  bool meta = km & Qt::MetaModifier;
  bool shift = km & Qt::ShiftModifier;

  if (navigation_mode && !ctrl && !alt) {
    switch (key) {
      case Qt::Key_O:
        main_window->slot_document_prev();
        return;
      case Qt::Key_P:
        main_window->slot_document_next();
        return;
    }
  }

  // Some keys have to be handled here, for the shortcuts:
  if (ctrl || alt || meta) {
    if (key >= Qt::Key_A && key <= Qt::Key_Z) {
      document->handle_raw_char(KeyPress('a' + key - Qt::Key_A, alt, ctrl, meta, shift));
      return;
    }
  }

  Key our_key = KEY_UNKNOWN;
  switch (key) {
    case Qt::Key_Right: our_key = KEY_RIGHT_ARROW; break;
    case Qt::Key_Up: our_key = KEY_UP_ARROW; break;
    case Qt::Key_Left: our_key = KEY_LEFT_ARROW; break;
    case Qt::Key_Down: our_key = KEY_DOWN_ARROW; break;
    case Qt::Key_Home: our_key = KEY_HOME; break;
    case Qt::Key_End: our_key = KEY_END; break;
    case Qt::Key_PageUp: our_key = KEY_PAGE_UP; break;
    case Qt::Key_PageDown: our_key = KEY_PAGE_DOWN; break;
    case Qt::Key_Delete: our_key = KEY_DELETE; break;
  }
  if (our_key != KEY_UNKNOWN) {
    document->handle_raw_char(KeyPress(our_key, alt, ctrl, meta, shift));
    return;
  }

  KeyPress kp;
  switch (key) {
    case Qt::Key_Tab: kp = KeyPress('\t', alt, ctrl, meta, shift); break;
    case Qt::Key_Backtab: kp = KeyPress('\t', alt, ctrl, meta, true); break;
    case Qt::Key_Backspace: kp = KeyPress(8, alt, ctrl, meta, shift); break;
    case Qt::Key_Escape:
      document->handle_escape();
      return;
    default:
      {
        QString text = event->text();
        std::string stext = text.toStdString();
        if (!stext.empty()) {
          uint32_t char_code = utf8_get(stext.c_str());
          if (char_code <= 26 && ctrl) {
            char_code = 'a' + char_code - 1;
          }
          kp = KeyPress(char_code, alt, ctrl, meta, shift);
        }
      }
      break;
  }
  document->handle_raw_char(kp);
}

void Editor::mouseDoubleClickEvent(QMouseEvent* event){
  if (document == nullptr) return;

  if (event->buttons() & Qt::LeftButton) {
    CursorLocation cl = flow_grid.unmap(event->x(), event->y());
    reset_blink();
    document->handle_mouse(cl.row, cl.col, false, false, true);
  }
}

void Editor::mouseMoveEvent(QMouseEvent* event){
  event->accept();

  if (!(event->modifiers() & Qt::ControlModifier)) {
    setCursor(Qt::IBeamCursor);
    setMouseTracking(false);
  }

  if (event->buttons() & Qt::LeftButton) {
    if (document == nullptr) return;
    CursorLocation cl = flow_grid.unmap(event->x(), event->y());
    reset_blink();
    document->handle_mouse(cl.row, cl.col, true, false);
  }
}

void Editor::mousePressEvent(QMouseEvent* event){
  if (document == nullptr) return;
  event->accept();

  if (event->button() == Qt::RightButton) {
    main_window->editor_context(event->globalPos());
    return;
  }

  CursorLocation cl = flow_grid.unmap(event->x(), event->y());
  reset_blink();
  document->handle_mouse(cl.row, cl.col, QGuiApplication::keyboardModifiers() & Qt::ShiftModifier, QGuiApplication::keyboardModifiers() & Qt::ControlModifier);
}

void Editor::focusInEvent(QFocusEvent* event) {
  main_window->set_current_doc(document);
  QWidget::focusInEvent(event);
}

/** For rendering of highlights and selections. */
void render_box(QPainter* painter, FlowGrid* flow_grid, int row, int col1, int col2, QColor color) {
  const RowInfo ri = flow_grid->get_row_info(row);
  const int line_height = ri.height / ri.num_effective_rows;
  if (col2 < 0) col2 = ri.length;
  for (int col = col1; col < col2; col++) {
    const FlowGridElement fge = flow_grid->map_to_element(row, col);
    painter->fillRect(fge.coordinate.x, fge.coordinate.y, fge.width, line_height, color);
  }
  // Nothing was rendered, instead, render a slim 1px border
  if (col1 >= col2) {
    const FlowGridElement fge = flow_grid->map_to_element(row, col1);
    painter->fillRect(fge.coordinate.x, fge.coordinate.y, 1, line_height, color);
  }
}

void Editor::paintEvent(QPaintEvent* event) {
  QPainter painter(this);
  painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

  QRect update_rect = event->rect();
  if (document == nullptr) {
    painter.fillRect(update_rect, editor_theme.bg_color);
    painter.setPen(Qt::white);
    painter.drawText(rect(), Qt::AlignCenter, "Create a new document or open an existing one.");
    main_window->editor_redrawn();
    return;
  }

  // TODO: Cleanup

  const TextBuffer* tf = document->get_text_buffer();
  const int num_lines = tf->get_num_lines();
  const CursorLocation cl = document->get_cursor();
  const SelectionInfo si = document->get_selection();
  const bool selection_active = si.active;
  const int line_height = glyph_store.get_line_height();
  const bool display_style_shell = (document->get_display_style() & DocFlag::SHELL_THEME) != 0;
  Highlights* highlights = &(document->get_appendage().rich_text.highlights);
  const bool suppress_temporary_highlights = document->get_appendage().rich_text.suppress_temporary_highlights;
  const bool has_focus = hasFocus();
  QTheme& theme = (display_style_shell) ? shell_theme : editor_theme;
  painter.fillRect(update_rect, theme.bg_color);

  // Starting and ending rows
  int start_paint_row = flow_grid.unmap_row(update_rect.top()) - 1;
  if (start_paint_row < 0) start_paint_row = 0;

  int end_paint_row = flow_grid.unmap_row(update_rect.bottom()) + 1;
  if (end_paint_row >= num_lines) end_paint_row = num_lines-1;


  // Line numbers
  const int x0 = compute_x_offset();
  if (line_numbers_visible && !display_style_shell) {
    painter.setFont(bold_font);
    painter.setPen(theme.gutter_text_color);
    painter.fillRect(0, update_rect.top(), x0 - LINE_NUMBER_SPACE/2, update_rect.height(),
        theme.gutter_bg_color);
    const int line_x = x0 - LINE_NUMBER_SPACE - glyph_store.get_width('9') / 2;
    for (int i = start_paint_row; i <= end_paint_row; i++) {
      int y = flow_grid.map_to_y(i, 0);

      const Line& l = tf->get_line(i);
      if (l.appendage().folded) {
        painter.drawLine(line_x, y, line_x, y+fold_line_height);
      } else {
        // Use our glyph store:
        int row_num = i+1;

        int x = x0 - LINE_NUMBER_SPACE;
        while (row_num > 0) {
          int digit = row_num % 10;
          const int width = glyph_store.get_width('0' + digit);
          x -= width;
          painter.drawStaticText(x, y, glyph_store_bold.get_static_text('0' + digit));
          row_num /= 10;
        }
      }
    }
  }

  // Ruler:
  if (ruler_show && !display_style_shell) {
    int ruler_px = x0 + ruler_width*glyph_store.get_x_width();
    painter.fillRect(ruler_px, update_rect.top(), update_rect.width(), update_rect.height(),
        theme.ruler_color);
  }

  // Draw the highlight line
  if (highlight_cursor_line && has_focus) {
    const RowInfo ri = flow_grid.get_row_info(cl.row);
    const int y = ri.y;
    const int y2 = y + ri.height;
    if (y <= update_rect.bottom() && y2 >= update_rect.top()) {
      painter.fillRect(x0, y, update_rect.right(), y2 - y, theme.cursor_highlight_bg_color);
    }
  }

  // Is the document wide?
  bool wide_document = false;
  {
    int needed_x = flow_grid.output_width + 20;
    int size_x = update_rect.width();
    if (needed_x > size_x) wide_document = true;
  }

  // Print out the text
  highlights->reset_row_highlights();
  painter.setFont(font);
  for (int row = start_paint_row; row <= end_paint_row; row++) {
    const Line& line = tf->get_line(row);
    const bool folded = line.appendage().folded;

    const RowInfo ri = flow_grid.get_row_info(row);
    const int row_y = ri.y;
    const int row_y2 = row_y + ri.height;
    const int h = ri.height;

    // Render wrap guides.
    if (draw_word_wrap_guides && ri.num_effective_rows > 1) {
      QColor color = theme.text_colors[0];
      QPen wrap_guide_pen = QPen(Qt::DashLine);
      wrap_guide_pen.setColor(color);
      painter.setPen(wrap_guide_pen);
      for (int i = 0; i < ri.num_effective_rows; i++) {
        EffRowInfo eri = flow_grid.get_effective_row_info(row, i);
        const int lm = flow_grid.get_effective_row_left_margin(eri) - 1;
        const int rm = flow_grid.get_effective_row_right_margin(eri);
        if (i > 0 && lm >= x0) painter.drawLine(lm, eri.y, lm, eri.y+eri.height);
        if (i != ri.num_effective_rows - 1) painter.drawLine(rm, eri.y, rm, eri.y+eri.height);
      }
    }

    // Render highlights
    highlights->get_row_highlights(row);
    for (Highlight& hl : highlights->row_highlights) {
      if (hl.type == HIGHLIGHT_TEMPORARY && suppress_temporary_highlights) continue;
      render_box(&painter, &flow_grid, row, hl.col1, hl.col2, theme.highlight_bg_color);
    }

    // Render selection
    if (selection_active) {
      if (row == si.row_start && row == si.row_end) {
        render_box(&painter, &flow_grid, row, si.col_start, si.col_end, theme.selected_bg_color);
      } else if (row == si.row_start) {
        render_box(&painter, &flow_grid, row, si.col_start, -1, theme.selected_bg_color);
      } else if (row == si.row_end) {
        render_box(&painter, &flow_grid, row, 0, si.col_end, theme.selected_bg_color);
      } else if (row > si.row_start && row < si.row_end) {
        render_box(&painter, &flow_grid, row, 0, -1, theme.selected_bg_color);
      }
    }
    const bool possibly_selected = selection_active && row >= si.row_start && row <= si.row_end;
    const bool possibly_highlighted = !(highlights->row_highlights.empty());

    if (!folded) {
      // Finally, print text
      for (unsigned int col = 0; col < line.size(); col++) {
        const Coordinate coord = flow_grid.map_to_coordinate(row, col);
        const int x = coord.x;
        const int y = coord.y;
        if (wide_document) {
          if (x > update_rect.right()) continue;
          if (x + 20 < update_rect.left()) continue;
        }

        const Character character = line.get_char(col);
        const int char_code = character.c;
        if (char_code == ' ' || char_code == '\t') continue;

        QColor color = theme.text_colors[character.markup % 16];
        if (possibly_selected) {
          bool selected_char = true;
          if (row == si.row_start && (int) col < si.col_start) selected_char = false;
          if (row == si.row_end && (int) col >= si.col_end) selected_char = false;
          if (selected_char) color = theme.selected_text_colors[character.markup % 16];
        }
        if (possibly_highlighted) {
          if (suppress_temporary_highlights) {
            if (highlights->is_char_highlighted_non_temporary(col)) color = theme.highlight_text_color;
          } else {
            if (highlights->is_char_highlighted(col)) color = theme.highlight_text_color;
          }
        }

        painter.setPen(color);

        const int fh = theme.text_fonts[character.markup % 16];
        if (fh == 1) {
          painter.setFont(bold_font);
          painter.drawStaticText(x, y, glyph_store_bold.get_static_text(char_code));
        } else if (fh == 2) {
          painter.setFont(italic_font);
          painter.drawStaticText(x, y, glyph_store_italic.get_static_text(char_code));
        } else {
          painter.setFont(font);
          painter.drawStaticText(x, y, glyph_store.get_static_text(char_code));
        }
      }
    } else {
      if (!line.is_whitespace() && fold_line_height > 0) {
        QImage im = draw_fold_image(row, line);
        painter.drawImage(0, row_y, im);
      }
    }
  }

  // Render overlays
  QPen overlay_line_pen = QPen(Qt::DashLine);
  overlay_line_pen.setColor(theme.overlay_line_color);
  painter.setFont(overlay_font);

  for (const Overlay &overlay : document->get_appendage().rich_text.overlays) {
    // Check the overlay.
    const int drow = overlay.drow, dcol = overlay.dcol;
    if (drow < 0 || drow >= tf->get_num_lines()) {
      printf("WARNING: invalid overlay.\n");
      continue;
    }
    const int row = overlay.row, col = overlay.col;
    if (row < 0 || row >= tf->get_num_lines()) {
      printf("WARNING: invalid overlay.\n");
      continue;
    }

    // Draw the overlay.
    const int x = flow_grid.map_to_x(row, col);
    const int y = flow_grid.map_to_y(row, col) + glyph_store.get_line_height();
    const int dx1 = flow_grid.map_to_x(drow, dcol);
    const int dy1 = flow_grid.map_to_y(drow, dcol);
    const int dy2 = dy1 + glyph_store.get_line_height();

    if (y > update_rect.bottom() && dy1 > update_rect.bottom()) continue;
    if (y < update_rect.top() && dy2 < update_rect.top()) continue;

    // Little circle
    painter.setPen(theme.overlay_line_color);
    painter.drawArc(x-3, y-3, 6, 6, 0, 16*360);

    // Line from circle to text
    painter.setPen(overlay_line_pen);
    if (y == dy2) {
      painter.drawLine(x+3, y, dx1, y);
    } else if (y < dy2) {
      painter.drawLine(x, y+3, x, dy2);
      painter.drawLine(x, dy2, dx1, dy2);
    } else {
      painter.drawLine(x, y-3, x, dy2);
      painter.drawLine(x, dy2, dx1, dy2);
    }

    // Text
    const QRect bound(dx1, dy1 - 2, 1000, dy2 - dy1 + 4);
    QRect out_rect;
    painter.setPen(theme.overlay_text_color);
    painter.drawText(bound, Qt::AlignVCenter, QString::fromStdString(overlay.text),
        &out_rect);
    painter.setPen(overlay_line_pen);
    painter.drawLine(dx1, dy2, dx1 + out_rect.width(), dy2);
  }

  // Display cursor
  if (cursor_visible) {
    if (!navigation_mode) {
      painter.fillRect(cursor_position_x, cursor_position_y, 1, cursor_size, theme.cursor_color);
    } else {
      QRect cursor_rect(cursor_position_x - 4, cursor_position_y, 8, cursor_size);
      painter.setPen(theme.cursor_color);
      painter.drawPixmap(cursor_rect, q_navmode_bitmap);
    }
  }

  // Draw boxes for extras up and down
  {
//    int ye1, ye2, xe1, xe2;
//    scroll_area->get_visible_extent(xe1, xe2, ye1, ye2);
//
//    const int rowe1 = flow_grid.unmap_row(ye1);
//    const int rowe2 = flow_grid.unmap_row(ye2);
//
//    {
//      QRect rect(xe2 - 50, ye1 + 5, 44, 20);
//      painter.setPen(theme.highlight_bg_color);
//      painter.drawText(rect,  Qt::AlignRight | Qt::AlignTop, "Foo");
//
//    }
  }

  main_window->editor_redrawn();
}