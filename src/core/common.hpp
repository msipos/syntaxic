#ifndef SYNTAXIC_CORE_COMMON_HPP
#define SYNTAXIC_CORE_COMMON_HPP

#include <cstdint>
#include <cstdio>

/** Character in a line. */
struct Character {
  char32_t c;
  uint8_t markup;

  inline Character() : c(0), markup(0) {}
  inline Character(char32_t c0) : c(c0), markup(0) {}
  inline Character(char32_t c0, uint8_t m) : c(c0), markup(m) {}

  inline bool is_eof() { return c == 0xFFFFFFFF && markup == 255; }
  inline void set_eof() { c = 0xFFFFFFFF; markup = 255; }
};

/** Just an integer coordinate. */
struct Coordinate {
  int x, y;
};

/** Range of lines. Iterate via for(int i = range.start; i < range.end; i++) { ... }.
 Always guarantees start is smaller than end. Also makes it possible to add ranges. */
class Range {
public:
  int start, end;

  inline Range(int s, int e) : start(s), end(e) {
    if (s > e) {
      start = e;
      end = s;
    }
  }

  inline void add(const Range& other) {
    if (other.start < start) start = other.start;
    if (other.end > end) end = other.end;
  }

  inline void add(int i) {
    if (i < start) start = i;
    if (i >= end) end = i+1;
  }
};

enum LineEndings {
  // Is not known.
  UNKNOWN,
  // '\n'
  UNIX,
  // '\r\n'
  WINDOWS,
  // mixed
  MIXED
};

struct CursorLocation {
  int row, col;

  inline CursorLocation() : row(0), col(0) {}
  inline CursorLocation(int r, int c) : row(r), col(c) {}
  inline CursorLocation(const CursorLocation& cl) : row(cl.row), col(cl.col) {}

  inline bool operator>(const CursorLocation& other) const {
    if (row < other.row) return false;
    if (row > other.row) return true;
    return col > other.col;
  }

  inline bool operator>=(const CursorLocation& other) const {
    if (row < other.row) return false;
    if (row > other.row) return true;
    return col >= other.col;
  }

  inline bool operator==(const CursorLocation& other) const {
    return (row == other.row) && (col == other.col);
  }
};

struct SearchResult {
  int row, col, size;

  SearchResult(int r, int c, int s) : row(r), col(c), size(s){}
};

enum SLTokenType {
  // 0
  TOKEN_TYPE_NONE = 0, TOKEN_TYPE_NUMBER, TOKEN_TYPE_COMMENT, TOKEN_TYPE_STRING, TOKEN_TYPE_IDENT,
  // 5
  TOKEN_TYPE_KEYWORD, TOKEN_TYPE_OTHER_KEYWORD, TOKEN_TYPE_PREPROCESSOR, TOKEN_TYPE_OPERATOR,
  // 9
  TOKEN_TYPE_ILLEGAL
};

struct Token {
private:
  // Highest bit is used to encode start/stop information.
  uint8_t type;
public:
  uint8_t mode_id;
  uint8_t extra;
  uint16_t offset;
  inline bool is_start() const { return (type & 128) != 0; }
  inline void set_start(bool start) {
    if (start) type |= 128;
    else type &= 127;
  }
  inline uint8_t get_type() const { return type & 127; }
  inline void set_type(SLTokenType t) {
    type &= 128;
    type |= t;
  }
  inline void debug() {
    if (is_start()) printf("start[");
    else printf("end[");
    printf("tt: %d, offset: %d]", get_type(), offset);
  }
};

enum Key {
  KEY_UNKNOWN=0, KEY_DELETE, KEY_LEFT_ARROW, KEY_RIGHT_ARROW, KEY_PAGE_UP, KEY_PAGE_DOWN,
  KEY_HOME, KEY_END, KEY_UP_ARROW, KEY_DOWN_ARROW
};

struct KeyPress {
  /** Is it a character or a special key? */
  bool is_char, alt, ctrl, meta, shift;
  union {
    uint32_t char_code;
    Key key;
  };

  KeyPress() : is_char(false), alt(false), ctrl(false), meta(false), shift(false) {
    key = KEY_UNKNOWN;
  }

  KeyPress(Key k, bool a, bool c, bool m, bool s) : is_char(false), alt(a), ctrl(c), meta(m), shift(s), key(k) {}
  KeyPress(uint32_t cc, bool a, bool c, bool m, bool s) : is_char(true), alt(a), ctrl(c), meta(m), shift(s), char_code(cc) {}
};

/** Structure that guarantees that row_end >= row_start. If the selection is only on one line, then
 it also guarantees that col_end >= col_start. */
struct SelectionInfo {
  // Is the selection active?  The rest of the fields are valid only if active = true.
  bool active;

  int row_start;
  int row_end;
  int col_start;
  int col_end;

  // If per line action is expected, then lines_ will be used.
  int lines_start;
  int lines_end;

  // Is the cursor at the beginning or the end of the selection?
  bool forwards;
};

struct SearchSettings {
  // Match whole word
  bool word;
  // Case insensitive match
  bool case_insensitive;
};

#endif