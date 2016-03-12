#include "core/flow_grid.hpp"
#include "core/text_buffer.hpp"

#include <algorithm>
#include <cctype>

FlowGrid::FlowGrid() : text_buffer(nullptr), x_width(1), tab_width(4), line_height(10), folded_line_height(2), word_wrap_width(-1), word_wrap_indent(4), x_offset(0), fast_reflow(false), output_width(10), output_height(10) {}

int FlowGrid::get_line_indent(const Line& line) const {
  int indent = 0;
  for (unsigned int j = 0; j < line.size(); j++) {
    const uint32_t ch = line.get_char(j).c;
    if (ch == '\t') indent += tab_width;
    else if (ch == ' ') indent += x_width;
    else break;
  }
  return indent;
}

int FlowGrid::get_word_width(const Line& line, int start_col) const {
  int width = 0;
  {
    const uint32_t ch = line.get_char(start_col).c;
    if (ch == '\t') return tab_width;
    if (isalnum(ch) || ch == '_') {
      width += x_width;
    } else {
      return x_width;
    }
  }
  for (unsigned int j = start_col+1; j < line.size(); j++) {
    const uint32_t ch = line.get_char(j).c;
    if (isalnum(ch) || ch == '_') {
      width += x_width;
    } else break;
  }
  return width;
}

/** Note, word_wrap_width is in pixels, and so are all the widths below. */
void FlowGrid::reflow() {
  rows.clear();
  elements.clear();
  output_width = 0;

  const bool wrap = word_wrap_width > 40;
  int current_y = 0;
  for (int i = 0; i < text_buffer->get_num_lines(); i++) {
    const Line& line = text_buffer->get_line(i);
    int current_x = x_offset;

    int line_indent = 0;
    if (wrap && !fast_reflow) {
      line_indent = get_line_indent(line) + word_wrap_indent*x_width;
      // Disable line indentation when too narrow.
      if (line_indent > word_wrap_width - 12*x_width) line_indent = 0;
    }

    // Number of effective rows for this row.
    int num_effective_rows = 1;

    FlowGridRow fgr;
    fgr.index = int(elements.size());
    fgr.start_y = current_y;
    fgr.row_length = int(line.size());
    // Total row height must be computed at the end.
    const int char_height = line.appendage().folded ? folded_line_height : line_height;

    for (unsigned int j = 0; j < line.size(); j++) {
      const uint32_t ch = line.get_char(j).c;
      const int char_width = (ch == '\t') ? tab_width : x_width;

      if (wrap) {
        int word_width;
        if (fast_reflow) word_width = 1;
        else word_width = get_word_width(line, j);

        if (word_width > word_wrap_width) {
          // This is a really, _really_ long word.
          if (current_x + char_width > word_wrap_width) {
            current_x = x_offset + line_indent;
            num_effective_rows += 1;
            current_y += char_height;
          }
        } else if (current_x + word_width > word_wrap_width) {
          current_x = x_offset + line_indent;
          num_effective_rows += 1;
          current_y += char_height;
        }
      }

      FlowGridElement fge;
      fge.coordinate.x = current_x;
      fge.coordinate.y = current_y;
      fge.width = (uint8_t) char_width;
      elements.push_back(fge);

      current_x += char_width;
    }
    if (current_x > output_width) output_width = current_x;
    current_y += char_height;

    fgr.row_height = num_effective_rows*char_height;
    fgr.num_effective_rows = num_effective_rows;
    rows.push_back(fgr);
  }
  if (wrap) output_width = word_wrap_width;
  output_height = current_y;
}

RowInfo FlowGrid::get_row_info(int row) const {
  if (row < 0 || rows.empty()) {
    printf("Warning: row < 0 in get_row_info.\n");
    return { 0, row*line_height, line_height, 1, 0 };
  }
  if (row >= int(rows.size())) {
    printf("Warning: row too big in get_row_info.\n");
    const int last = rows.back().start_y + rows.back().row_height;
    return { int(elements.size()), last + (row - int(rows.size()))*line_height, line_height, 1, 0 };
  }

  const FlowGridRow& fgr = rows[row];
  return { fgr.index, fgr.start_y, fgr.row_height, fgr.num_effective_rows, fgr.row_length };
}

FlowGridElement FlowGrid::map_to_element(int row, int col) const {
  if (row < 0 || rows.empty()) {
    printf("Warning: row < 0 in map_to_coordinate.\n");
    return { x_offset + col*x_width, row*line_height, uint8_t(x_width) };
  }
  if (row >= int(rows.size())) {
    printf("Warning: row too big in map_to_coordinate.\n");
    const int last = rows.back().start_y + rows.back().row_height;
    return { x_offset + col*x_width, last + (row - int(rows.size()))*line_height, uint8_t(x_width) };
  }

  const FlowGridRow& fgr = rows[row];
  if (col < 0 || fgr.row_length == 0) {
    return { x_offset + col*x_width, fgr.start_y, uint8_t(x_width) };
  }
  if (col >= fgr.row_length) {
    const FlowGridElement& last_fge = elements[fgr.index + fgr.row_length-1];
    return { last_fge.coordinate.x + last_fge.width + (col - fgr.row_length)*x_width, last_fge.coordinate.y, uint8_t(x_width) };
  }
  const FlowGridElement& fge = elements[fgr.index + col];
  return fge;
}

Coordinate FlowGrid::map_to_coordinate(int row, int col) const {
  const FlowGridElement fge = map_to_element(row, col);
  return fge.coordinate;
}

int FlowGrid::map_to_x(int row, int col) const {
  return map_to_coordinate(row, col).x;
}

int FlowGrid::map_to_y(int row, int col) const {
  return map_to_coordinate(row, col).y;
}

struct FlowGridElementComparator {
  bool operator() (const FlowGridElement& left, const FlowGridElement& right) {
    return left.coordinate.y < right.coordinate.y;
  }
};

int FlowGrid::get_row_index_of_effective_row(int x, int row, int eff_row) const {
  const EffRowInfo eri = get_effective_row_info(row, eff_row);
  for (int index = eri.first_index; index < eri.last_index; index++) {
    if (elements[eri.row_index + index].coordinate.x >= x) {
      return index;
    }
  }
  return eri.last_index;
}

int FlowGrid::get_effective_row(int row, int col) const {
  if (col <= 0) return 0;
  const RowInfo ri = get_row_info(row);
  if (col >= ri.length) return ri.num_effective_rows - 1;
  const int ty = elements[ri.index + col].coordinate.y;
  const int irh = ri.height / ri.num_effective_rows;
  return (ty - ri.y) / irh;
}

EffRowInfo FlowGrid::get_effective_row_info(int row, int eff_row) const {
  const RowInfo ri = get_row_info(row);
  EffRowInfo eri;

  if (eff_row < 0) {
    printf("Warning: eff_row < 0\n");
    eri = {0, 0, 0, 0, 0};
    return eri;
  } else if (eff_row >= ri.num_effective_rows) {
    printf("Warning: eff_row >= num_ers\n");
    eri = {0, 0, 0, 0, 0};
    return eri;
  }

  // Individual row height:
  const int irh = ri.height / ri.num_effective_rows;
  eri.row_index = ri.index;
  eri.height = irh;
  eri.y = ri.y + irh*eff_row;
  const int start_index = rows[row].index;
  const int end_index = start_index + rows[row].row_length;
  const FlowGridElement dummy_fge = { {0, eri.y}, 0 };
  eri.first_index = std::lower_bound(elements.begin() + start_index, elements.begin() + end_index, dummy_fge, FlowGridElementComparator()) - elements.begin() - eri.row_index;
  eri.last_index = std::upper_bound(elements.begin() + start_index, elements.begin() + end_index, dummy_fge, FlowGridElementComparator()) - elements.begin() - eri.row_index;

  return eri;
}

int FlowGrid::get_effective_row_left_margin(const EffRowInfo& eri) const {
  return elements[eri.row_index + eri.first_index].coordinate.x;
}

int FlowGrid::get_effective_row_right_margin(const EffRowInfo& eri) const {
  return elements[eri.row_index + eri.last_index-1].coordinate.x + elements[eri.last_index-1].width;
}

int FlowGrid::unmap_row(int y) const {
  for (int row = 0; row < int(rows.size()); row++) {
    const FlowGridRow& fgr = rows[row];
    if (fgr.start_y + fgr.row_height >= y) return row;
  }
  return rows.size() - 1;
}

CursorLocation FlowGrid::unmap(int x, int y) const {
  if (rows.empty()) return CursorLocation(0, 0);

  int row = unmap_row(y);

  const FlowGridRow& fgr = rows[row];
  const int row_height = fgr.row_height / fgr.num_effective_rows;
  for (int col = 0; col < fgr.row_length; col++) {
    const FlowGridElement& fge = elements[fgr.index + col];
    if (x < fge.coordinate.x + x_width/2) {
      if (y <= fge.coordinate.y + row_height) {
        return CursorLocation(row, col);
      }
    }
  }

  return CursorLocation(row, fgr.row_length);
}