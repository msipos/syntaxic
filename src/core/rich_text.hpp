#ifndef SYNTAXIC_CORE_HIGHLIGHTS_HPP
#define SYNTAXIC_CORE_HIGHLIGHTS_HPP

#include <string>
#include <vector>

class TextBuffer;

enum HighlightType {
  HIGHLIGHT_UNKNOWN,
  HIGHLIGHT_TEMPORARY,
  HIGHLIGHT_SEARCH
};

struct Highlight {
  HighlightType type;
  int row, col1, col2;

  Highlight(int r, int c1, int c2) : type(HIGHLIGHT_UNKNOWN), row(r), col1(c1), col2(c2) {}

  inline bool operator< (const Highlight& rhs) const {
    if (row == rhs.row) {
      return col1 < rhs.col1;
    }
    return row < rhs.row;
  }
};

class Highlights {
  int search_index, temporary_index;

  void process(std::vector<Highlight>& output, const std::vector<Highlight>& highlights, int row, int& index, HighlightType type);

public:
  Highlights();

  std::vector<Highlight> search;
  std::vector<Highlight> temporary;

  // Temporary highlights
  std::vector<Highlight> row_highlights;

  void reset_row_highlights();
  void get_row_highlights(int row);
  void sort_temporary_highlights();
  /** Look in row highlights and tell me if this column is highlighted. */
  bool is_char_highlighted(int col);
  /** Look in row highlights and tell me if this column is highlighted, but temporary highlight doesn't count. */
  bool is_char_highlighted_non_temporary(int col);
};

namespace OverlayType {
  enum Type {
    NOTICE, ERROR, STATLANG_ERROR
  };
}

struct Overlay {
  int row, col;
  int drow, dcol;
  OverlayType::Type type;
  std::string text;

  Overlay(int r, int c, OverlayType::Type t, const std::string& tx) : row(r), col(c), drow(0), dcol(0), type(t), text(tx) {};
  Overlay(int r, int c, OverlayType::Type t, int dr, int dc, const std::string& tx) : row(r), col(c), drow(dr), dcol(dc), type(t), text(tx) {};
};

/** Additions to plain TextBuffer.  Overlays and highlights for the time being. */
class RichText {
public:
  Highlights highlights;

  /** Suppress temporary highlights?  This is used during incremental search. */
  bool suppress_temporary_highlights;

  /** Sequence of overlays. */
  std::vector<Overlay> overlays;
  void add_overlay(const TextBuffer* tb, int row, int col, OverlayType::Type type, const std::string& text);
  void clear_overlays(OverlayType::Type type);

  inline RichText() : suppress_temporary_highlights(false) {}
};


#endif