#include "core/line.hpp"
#include "core/utf8_util.hpp"

#include <stdexcept>
#include "utf8.h"

Line::Line(const char* buf, int line_len) {
  append(buf, line_len);
}

Line::Line(const std::string& s) {
  append(s);
}

Line::Line() {
}

void Line::from_line(const Line& other) {
  contents = other.contents;
  line_appendage = other.line_appendage;
}

void Line::check_index(int i) const {
  if (i < 0 || i > (int) size()) throw std::out_of_range("check index");
}

#define CHECK(i)   check_index(i)

void Line::append(char32_t ch, uint8_t markup) {
  contents.push_back(Character(ch, markup));
}

void Line::append(const std::string& other, uint8_t markup) {
  append(other.c_str(), other.size(), markup);
}

void Line::append(const char* s, uint8_t markup) {
  append(s, strlen(s), markup);
}

void Line::append(const char* s, int sz, uint8_t markup) {
  const char* end = s + sz;
  while (s != end) {
    uint32_t c = utf8::next(s, end);
    contents.push_back(Character(c, markup));
  }
}

void Line::insert(int index, char32_t ch, uint8_t markup) {
  CHECK(index);
  contents.insert(contents.begin() + index, Character(ch, markup));
}

void Line::insert(int index, const std::string& other, uint8_t markup) {
  insert(index, other.c_str(), other.size(), markup);
}

void Line::insert(int index, const char* s, uint8_t markup) {
  insert(index, s, strlen(s), markup);
}

void Line::insert(int index, const char* s, int sz, uint8_t markup) {
  CHECK(index);
  const char* end = s + sz;
  while (s != end) {
    uint32_t c = utf8::next(s, end);
    contents.insert(contents.begin() + index, Character(c, markup));
    index++;
  }
}

void Line::remove(int index) {
  CHECK(index);
  contents.erase(contents.begin() + index);
}

void Line::remove(int index0, int index1) {
  CHECK(index0);
  CHECK(index1);
  contents.erase(contents.begin() + index0, contents.begin() + index1);
}

void Line::trim(int index) {
  CHECK(index);
  contents.erase(contents.begin() + index, contents.end());
}

int Line::get_start() const {
  for (unsigned int i = 0; i < contents.size(); i++) {
    const Character& c = contents[i];
    if (c.c != ' ' && c.c != '\t') {
      return i;
    }
  }
  return contents.size();
}

int Line::get_end() const {
  for (int i = contents.size() - 1; i >= 0; i--) {
    const Character& c = contents[i];
    if (c.c != ' ' && c.c != '\t') {
      return i+1;
    }
  }
  return contents.size();
}

std::string Line::to_string(int index0, int index1) const {
  CHECK(index0); CHECK(index1);
  std::string s;
  for (int i = index0; i < index1; i++) {
    uint32_t c = contents[i].c;
    utf8::append(c, std::back_inserter(s));
  }
  return s;
}

std::string Line::to_string() const {
  return to_string(0, size());
}

bool Line::is_whitespace() const {
  for (unsigned int i = 0; i < contents.size(); i++) {
    const Character& cc = contents[i];
    if (cc.c != ' ' && cc.c != '\t') return false;
  }
  return true;
}

int Line::num_real_chars() const {
  int n = 0;
  for (unsigned int i = 0; i < contents.size(); i++) {
    const Character& cc = contents[i];
    if (cc.c != ' ' && cc.c != '\t') n++;
  }
  return n;
}

bool Line::is_non_word() const {
  for (unsigned int i = 0; i < contents.size(); i++) {
    const Character& cc = contents[i];
    if (isalnum(cc.c) || cc.c == '_') return false;
  }
  return true;
}

void Line::trim_whitespace() {
  int end = get_end();
  if (is_whitespace()) end = 0;
  trim(end);
}

void Line::trim_r() {
  if (!contents.empty() && contents.back().c == '\r') {
    contents.pop_back();
  }
}

Indentation Line::get_indentation() const {
  Indentation ind = {0, 0};
  for (unsigned int i = 0; i < contents.size(); i++) {
    const Character& cc = contents[i];
    if (cc.c == ' ') ind.num_spaces++;
    else if (cc.c == '\t') ind.num_tabs++;
    else return ind;
  }
  return ind;
}

void Line::search(const std::string& term, std::vector<SearchResult>& results, int line_num,
    bool word, bool case_insensitive) const {
  if (term.empty()) return;
  const int term_size = utf8_size(term);
  const char* cterm = term.c_str();
  const char* cterm_cur = cterm;
  const char* const cterm_end = cterm + strlen(cterm);
  int found_so_far = 0;

  for (unsigned int i = 0; i < contents.size(); i++) {
    const Character& cc = contents[i];

    uint32_t term_c = utf8::next(cterm_cur, cterm_end);
    uint32_t c = cc.c;

    if (case_insensitive) {
      term_c = tolower(term_c);
      c = tolower(c);
    }

    if (term_c == c) {
      found_so_far++;
      if (cterm_cur == cterm_end) {
        // Found it!
        int found_it_at = i - term_size + 1;
        cterm_cur = cterm;

        if (word) {
          // Check character to the left
          if (found_it_at > 0) {
            uint32_t lc = contents[found_it_at-1].c;
            if (isalnum(lc) || lc == '_') continue;
          }

          // Check character to the right
          if (i + 1 < contents.size()) {
            uint32_t rc = contents[i+1].c;
            if (isalnum(rc) || rc == '_') continue;
          }
        }

        results.push_back(SearchResult(line_num, found_it_at, term_size));
        found_so_far = 0;
      }
    } else {
      i -= (found_so_far);
      found_so_far = 0;
      cterm_cur = cterm;
    }
  }
}

void Line::search_char(int ch, std::vector<int>& results) const {
  for (unsigned int i = 0; i < contents.size(); i++) {
    int mych = contents[i].c;
    if (mych == ch) results.push_back(i);
  }
}

void Line::replace(const std::string& term, std::string& replacement, int col) {
  int size = utf8_size(term);
  remove(col, col+size);
  insert(col, replacement);
}