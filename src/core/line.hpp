#ifndef SYNTAXIC_LINE_HPP
#define SYNTAXIC_LINE_HPP

#include "core/common.hpp"

#include <cstring>
#include <string>
#include <vector>

/** Extra data for each line that is not touched by Line. */
struct LineAppendage {
  std::vector<Token> tokens;

  // Is line folded?
  bool folded;

  inline LineAppendage(): folded(false) {}
};

struct Indentation {
  int num_tabs, num_spaces;
};

class Line {
private:
  std::vector<Character> contents;
  void check_index(int i) const;

  LineAppendage line_appendage;
public:
  Line();
  Line(const std::string& s);
  Line(const char* buf, int line_len);

  // Copy:

  Line(const Line&) = delete;
  Line& operator=(const Line&) = delete;

  // Move:
  Line(const Line&& other) {
	  contents = std::move(other.contents);
	  line_appendage = other.line_appendage;
  }
  Line& operator=(const Line&& other) {
	  contents = std::move(other.contents);
	  line_appendage = other.line_appendage;
	  return *this;
  }

  // Copy from another line:
  void from_line(const Line&);

  // Access

  inline size_t size() const { return contents.size(); }
  inline LineAppendage& appendage() { return line_appendage; }
  inline const LineAppendage& appendage() const { return line_appendage; }
  inline Character& get_char(int index) { return contents.at(index); }
  inline const Character& get_char(int index) const { return contents.at(index); }
  /** Get the start of the line, i.e. skip all whitespace at the start of the line. In case of
   all whitespace line, this is the ending. */
  int get_start() const;
  /** Get the end of the line, i.e. skip all whitespace at the end. In case of all whitespace line,
   this is the ending. */
  int get_end() const;
  /** Get contents as a UTF8 string. */
  std::string to_string() const;
  std::string to_string(int index0, int index1) const;
  /** Is line purely whitespace? */
  bool is_whitespace() const;
  /** Number of real, i.e. non-whitespace, characters in a line. */
  int num_real_chars() const;
  /** Does line consist purely of non-word characters? Usually these are punctuation lines. */
  bool is_non_word() const;

  // Editing

  void append(char32_t c, uint8_t markup=0);
  void append(const std::string& other, uint8_t markup=0);
  void append(const char* s, uint8_t markup=0);
  void append(const char* s, int sz, uint8_t markup=0);
  void insert(int index, char32_t c, uint8_t markup=0);
  void insert(int index, const std::string& other, uint8_t markup=0);
  void insert(int index, const char* s, uint8_t markup=0);
  void insert(int index, const char* s, int sz, uint8_t markup=0);
  void remove(int index);
  void remove(int index0, int index1);

  /** Delete line after index. */
  void trim(int index);
  /** Trim whitespace at the end of the line. */
  void trim_whitespace();
  /** Trim '\r' off the end (if any). */
  void trim_r();
  Indentation get_indentation() const;
  inline void optimize_size() { contents.shrink_to_fit(); }

  /** Search. If word == true, then the results must be whole words. */
  void search(const std::string& term, std::vector<SearchResult>& results, int line_num, bool word, bool case_insensitive=false) const;
  void search_char(int ch, std::vector<int>& results) const;
  void replace(const std::string& term, std::string& replacement, int col);
};

#endif