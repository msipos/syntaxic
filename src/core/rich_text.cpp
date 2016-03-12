#include "core/text_buffer.hpp"
#include "core/rich_text.hpp"

#include <algorithm>
#include <cstdio>

Highlights::Highlights() : search_index(0), temporary_index(0) {}

void Highlights::process(std::vector<Highlight>& output, const std::vector<Highlight>& highlights,
    int row, int& index, HighlightType type) {
  for (;;) {
    if (index >= int(highlights.size())) {
      return;
    }
    Highlight hl = highlights[index];
    if (hl.row > row) return;
    if (hl.row < row) {
      index++;
      continue;
    }
    hl.type = type;
    output.push_back(hl);
    index++;
  }
}

void Highlights::reset_row_highlights() {
  search_index = 0;
  temporary_index = 0;
}

void Highlights::get_row_highlights(int row) {
  row_highlights.clear();

  process(row_highlights, search, row, search_index, HIGHLIGHT_SEARCH);
  process(row_highlights, temporary, row, temporary_index, HIGHLIGHT_TEMPORARY);
}

void Highlights::sort_temporary_highlights() {
  std::sort(temporary.begin(), temporary.end());
}

bool Highlights::is_char_highlighted(int col) {
  for (Highlight& hl : row_highlights) {
    if (col >= hl.col1 && col < hl.col2) {
      return true;
    }
  }
  return false;
}

bool Highlights::is_char_highlighted_non_temporary(int col) {
  for (Highlight& hl : row_highlights) {
    if (hl.type == HIGHLIGHT_TEMPORARY) continue;
    if (col >= hl.col1 && col < hl.col2) {
      return true;
    }
  }
  return false;
}

int get_row_preference(int row, int col, int delta, const TextBuffer* tb, std::vector<Overlay>& overlays) {
  if (row < 0 || row >= tb->get_num_lines()) return 100000;
  for (Overlay& o: overlays) {
    if (o.drow == row) return 50000;
  }
  const Line& line = tb->get_line(row);
  if (delta < 0) delta *= -1;
  return std::max(((int) line.size() + 3), col) + 2*delta;
}

void RichText::add_overlay(const TextBuffer* tb, int row, int col, OverlayType::Type type, const std::string& text) {
  if (row < 0) { printf("WARNING: Invalid add_annotation call < 0.\n"); return; }
  if (row >= tb->get_num_lines()) {
    printf("WARNING: Invalid add_annotation call >= num_lines.\n"); return;
  }
  int drow = row;
  int best_preference = 100000;

  for (int i = -2; i <= 2; i++) {
    int preference = get_row_preference(row+i, col, i, tb, overlays);
    if (preference < best_preference) {
      drow = row + i;
      best_preference = preference;
    }
  }
  overlays.push_back(Overlay(row, col, type, drow, best_preference, text));
}

void RichText::clear_overlays(OverlayType::Type type) {
  for (int i = overlays.size() - 1; i >= 0; i--) {
    Overlay& o = overlays[i];
    if (o.type == type) {
      overlays.erase(overlays.begin() + i);
    }
  }
}