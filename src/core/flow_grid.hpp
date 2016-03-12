#ifndef SYNTAXIC_CORE_FLOW_GRID_HPP
#define SYNTAXIC_CORE_FLOW_GRID_HPP

#include "core/common.hpp"
#include "core/line.hpp"

#include <cstdint>
#include <vector>

class TextBuffer;

struct FlowGridRow {
  int index;
  int start_y;
  int row_height;
  int row_length;
  int num_effective_rows;
};

struct FlowGridElement {
  Coordinate coordinate;
  uint8_t width;
};

struct RowInfo {
  // Index of first element
  int index;
  // Top, in pixels
  int y;
  // Total height in pixels
  int height;
  // Number of effective rows.
  int num_effective_rows;
  // Length in characters
  int length;
};

struct EffRowInfo {
  int y;
  int height;
  // Index of the whole row.
  int row_index;
  // First index of this effective row (from start of row).
  int first_index;
  // Last index of this effective row (exclusive).
  int last_index;
};

class FlowGrid {
private:
  std::vector<FlowGridRow> rows;
  std::vector<FlowGridElement> elements;

  int get_line_indent(const Line& line) const;
  int get_word_width(const Line& line, int start_col) const;

public:
  FlowGrid();


  //////////   Inputs for flowing


  /** Text file to flow. */
  const TextBuffer* text_buffer;

  /** Width of an x, in pixels. */
  int x_width;

  /** Width of a tab in pixels. */
  int tab_width;

  /** Height of a normal line. */
  int line_height;

  /** Height of a folded line. */
  int folded_line_height;

  /** Word wrap? -1 if not and maximum width if yes. */
  int word_wrap_width;

  /** If word-wrap is enabled, indent this much (in spaces). */
  int word_wrap_indent;

  /** X-offset.  Add this to every x. */
  int x_offset;

  /** Hint for big files, use a fast reflow algo. */
  bool fast_reflow;

  /** Once your inputs are set up, you can call reflow() and read outputs. */
  void reflow();


  //////////   Outputs for flowing


  /** Maximum width of a line. */
  int output_width;

  /** Height of the whole grid. */
  int output_height;

  /** Get element (coordinate + width) for a row+col pair. */
  FlowGridElement map_to_element(int row, int col) const;
  /** Get a coordinate for a row+col pair. */
  Coordinate map_to_coordinate(int row, int col) const;
  int map_to_x(int row, int col) const;
  int map_to_y(int row, int col) const;
  RowInfo get_row_info(int row) const;
  /** Get row index in an effective row that is closest to x. */
  int get_row_index_of_effective_row(int x, int row, int eff_row) const;
  /** Given a column index, what effective row are you in? */
  int get_effective_row(int row, int col) const;
  EffRowInfo get_effective_row_info(int row, int eff_row) const;
  /** For a row's effective row, get the last X coordinate (inclusive). */
  int get_effective_row_right_margin(const EffRowInfo& eri) const;
  /** For a row's effective row, get the first X coordinate (inclusive). */
  int get_effective_row_left_margin(const EffRowInfo& eri) const;

  /** Map from coordinate to the cursor location. */
  int unmap_row(int y) const;
  CursorLocation unmap(int x, int y) const;
};

#endif