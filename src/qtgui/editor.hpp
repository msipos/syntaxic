#ifndef QTGUI_EDITOR_HPP
#define QTGUI_EDITOR_HPP

#include "core/line.hpp"
#include "document.hpp"
#include "core/flow_grid.hpp"
#include "qtgui/glyph_store.hpp"
#include "qtgui/qtheme.hpp"

#include <memory>
#include <QBitmap>
#include <QImage>
#include <QScrollArea>
#include <QWidget>

class Editor;
class MainWindow;
class QResizeEvent;
class QTimer;

class MyScrollArea : public QScrollArea {
private:
  Editor* editor;

public:
  MyScrollArea(QWidget* parent, Editor* editor);

  virtual void resizeEvent(QResizeEvent* event);
  void page_up();
  void page_down();
  void get_visible_extent(int& x1, int& x2, int& y1, int& y2);
};

struct FoldImageCache {
  int line;
  QImage image;
};

class Editor : public QWidget {
  Q_OBJECT

private:
  /** Just a reference to the MainWindow to let it know about cursor movements. */
  MainWindow* main_window;

  /** QScrollArea that holds the editor. */
  MyScrollArea* scroll_area;
  void ensure_visible(int x, int y, int size);

  /** Document currently displayed in the widget. */
  Doc* document;

  /** Cache of QImages for folded lines. */
  std::vector<FoldImageCache> fold_images;
  const QImage& draw_fold_image(int row, const Line& line);

  //////////////////////////////////////////////////////////// Cursor blinking

  /** Timer used for blinking. */
  QTimer* q_timer;
  /** Is cursor visible? (Used for blinking.) */
  bool cursor_visible;
  /** Last time it blinked. */
  double last_blink_time;
  /** Reset the blinking of the cursor. */
  void reset_blink();
  /** Is it navigation mode? */
  bool navigation_mode;

  //////////////////////////////////////////////////////////// Cursor position

  /** Mouse is being held. */
  bool mouse_held;
  /** Cursor positions in pixels. */
  int cursor_position_x, cursor_position_y, cursor_size;
  /** Compute cursor pixels. */
  void compute_cursor_pixels();

  // Hooks:

  /** Hook that will trigger whenever the document changes. */
  Hook<Doc*, int> document_hook;
  void document_callback(Doc* doc, int flag);

  /** Hook that triggers on preference update. */
  Hook<> preferences_hook;
  void preferences_callback();

  // Font and rendering:

  /** Current font and its associated metrics. */
  QFont font, bold_font, italic_font;

  /** Glyph store for the current font. */
  GlyphStore glyph_store, glyph_store_bold, glyph_store_italic;

  /** Font for overlays. */
  QFont overlay_font;

  /** Flow grid reflows the text. */
  FlowGrid flow_grid;

  /** Folding line height. */
  int fold_line_height;

  /** Folding alpha. */
  int fold_alpha;

  /** Display ruler? */
  bool ruler_show;

  /** Ruler width. */
  int ruler_width;

  /** Are line numbers visible? */
  bool line_numbers_visible;
  int compute_x_offset();

  /** Highlight cursor line? */
  bool highlight_cursor_line;

  /** Should I word wrap? */
  bool word_wrap;

  /** Should I draw word wrap guides? */
  bool draw_word_wrap_guides;

  /** Internal editor width and height. */
  int editor_width, editor_height;

  /** Current theme. */
  QTheme editor_theme;
  QTheme shell_theme;

  /** Bitmap for the navigation mode cursor. */
  QBitmap q_navmode_bitmap;

  /** Bitmaps for arrows. */
  QBitmap q_up_bitmap, q_down_bitmap;

private slots:
  void slot_timer();

public:
  Editor(QWidget* parent, MainWindow* main_window);

  virtual bool event(QEvent*) override;
  virtual void focusInEvent(QFocusEvent * event) override;
  virtual void paintEvent(QPaintEvent*) override;
  virtual void keyPressEvent(QKeyEvent*) override;
  virtual void keyReleaseEvent(QKeyEvent*) override;
  virtual void mouseDoubleClickEvent(QMouseEvent * event) override;
  virtual void mouseMoveEvent(QMouseEvent * event) override;
  virtual void mousePressEvent(QMouseEvent * event) override;
  /** Do a reflow, compute editor_width and editor_height that will be returned in size hints. */
  void reflow();
  virtual QSize sizeHint() const;
  virtual QSize minimumSizeHint() const;

  int line_height();
  void set_document(Doc* d);
  void set_navigation_mode(bool nm);

  void reload_settings();

  inline MyScrollArea* get_scroll_area() { return scroll_area; }
};

#endif