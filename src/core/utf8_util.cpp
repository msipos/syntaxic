#include "core/utf8_util.hpp"

#include <algorithm>
#include "utf8.h"

bool utf8_check(const char* data, unsigned int len) {
  const char* end = data + len;
  while (data < end) {
    try {
      utf8::next(data, end);
    } catch (utf8::exception&) {
      return false;
    }
  }
  return true;
}

std::string utf8_convert_best(const char* data, unsigned int len) {
  std::string out;

  const char* end = data + len;
  const char* prev = data;

  while (data < end) {
    try {
      uint32_t cp = utf8::next(data, end);
      while (prev < data) {
        out += *prev;
        prev++;
      }
    } catch (utf8::exception&) {
      char c = *prev;
      utf8_append(out, c);
      prev++;
      data = prev;
    }
  }

  return out;
}

std::string utf8_to_string(uint32_t cp) {
  std::string out;
  char piece[5] = {0, 0, 0, 0, 0};
  utf8::append(cp, piece);
  out += piece;
  return out;
}

void utf8_rtrim(std::string& str, char c) {
  for (int i = str.size() - 1; i >= 0; i--) {
    if (str[i] != c) {
      str.erase(str.begin() + i + 1, str.end());
	  return;
    }
  }
}

std::string utf8_strip(const std::string& str) {
  std::vector<uint32_t> codes = utf8_string_to_vector(str);
  std::vector<uint32_t> outvec;

  int start = codes.size(), end = 0;
  for (unsigned int i = 0; i < codes.size(); i++) {
    if (codes[i] != ' ' && codes[i] != '\t' && codes[i] != '\n') {
      start = int(i);
      break;
    }
  }
  for (int i = codes.size()-1; i >= 0; i--) {
    if (codes[i] != ' ' && codes[i] != '\t' && codes[i] != '\n') {
      end = i + 1;
      break;
    }
  }

  for (int i = start; i < end; i++) {
    outvec.push_back(codes[i]);
  }
  return utf8_vector_to_string(outvec);
}

std::string utf8_string_lower(const std::string& input) {
  std::string out;
  for (uint32_t ch : UTF8Wrapper(input)) {
    if (ch < 256) {
      ch = tolower(ch);
    }
    utf8_append(out, ch);
  }
  return out;
}

std::string utf8_common_prefix(const std::string& s1, const std::string& s2) {
  std::string cp;
  int max = std::min(utf8_size(s1), utf8_size(s2));
  const char* c1 = s1.c_str();
  const char* c2 = s2.c_str();
  for (int i = 0; i < max; i++) {
    uint32_t u1 = utf8_get(c1);
    uint32_t u2 = utf8_get(c2);
    if (u1 != u2) break;
    utf8_append(cp, u1);
    c1 += utf8_count(c1, 1);
    c2 += utf8_count(c2, 1);
  }
  return cp;
}

void utf8_string_split(std::vector<std::string>& output, const std::string& input, uint32_t split_char) {
  int i = 0;
  int last_start = 0;
  for (const uint32_t ch : UTF8Wrapper(input)) {
    if (ch == split_char) {
      const int start = utf8_count(input.c_str(), last_start);
      const int end = utf8_count(input.c_str(), i);
      output.push_back(input.substr(start, end-start));
      last_start = i + 1;
    }
    i += 1;
  }
  const int start = utf8_count(input.c_str(), last_start);
  output.push_back(input.substr(start));
}

void utf8_string_split_whitespace(std::vector<std::string>& output, const std::string& input, int max_split) {
  int i = 0;
  int last_start = 0;
  for (const uint32_t ch : UTF8Wrapper(input)) {
    if (ch == ' ' || ch == '\t' || ch == '\n') {
      const int start = utf8_count(input.c_str(), last_start);
      const int end = utf8_count(input.c_str(), i);

      if (max_split > 0 && int(output.size()) == max_split - 1) {
        std::string last = input.substr(start);
        if (!last.empty()) output.push_back(last);
        return;
      }


      if (start != end) output.push_back(input.substr(start, end-start));
      last_start = i + 1;
    }
    i += 1;
  }
  const int start = utf8_count(input.c_str(), last_start);
  std::string last = input.substr(start);
  if (!last.empty()) output.push_back(last);
}

int utf8_count(const char* str, int count) {
  const char* cur = str;
  const char* end = cur + strlen(cur);
  int index = 0;
  for (;;) {
    if (index == count) break;
    if (cur == end) throw std::out_of_range("utf8_count invalid count");
    utf8::next(cur, end);
    index++;
  }
  return cur - str;
}

uint32_t utf8_get(const char* str) {
  const char* cur = str;
  const char* end = cur + strlen(cur);
  return utf8::next(cur, end);
}

int utf8_size(const char* cur) {
  const char* end = cur + strlen(cur);
  int size = 0;
  while (cur != end) {
    utf8::next(cur, end);
    size++;
  }
  return size;
}

int utf8_size(const std::string& str) {
  return utf8_size(str.c_str());
}

void utf8_insert(std::string& str, int index, const std::string& piece) {
  int offset = utf8_count(str.c_str(), index);
  str.insert(offset, piece);
}

void utf8_append(std::string& a, uint32_t c) {
  utf8::append(c, std::back_inserter(a));
}

void utf8_insert(std::string& str, int index, uint32_t c) {
  int offset = utf8_count(str.c_str(), index);
  char piece[5] = {0, 0, 0, 0, 0};
  utf8::append(c, piece);
  str.insert(offset, piece);
}

void utf8_remove(std::string& str, int index) {
  const char* end = str.c_str() + str.size();
  int temp = 0;
  const char* cur = str.c_str();
  for (;;) {
    if (temp == index) break;
    if (cur == end) throw std::out_of_range("utf8_remove invalid index");
    utf8::next(cur, end);
    temp++;
  }
  const char* next = cur;
  if (next == end) throw std::out_of_range("utf8_remove invalid index");
  utf8::next(next, end);
  str.erase(cur - str.c_str(), next - cur);
}

bool UTF8Iterator::operator!=(const UTF8Iterator& other) const {
  return ptr != other.ptr;
}

uint32_t UTF8Iterator::operator*() const {
  return utf8::peek_next(ptr, ptr_end);
}

void UTF8Iterator::operator++() {
  utf8::next(ptr, ptr_end);
}

UTF8Iterator UTF8Wrapper::begin() const {
  const char* end = str + size;
  return UTF8Iterator(str, end);
}

UTF8Iterator UTF8Wrapper::end() const {
  const char* end = str + size;
  return UTF8Iterator(end, end);
}

std::vector<uint32_t> utf8_string_to_vector(const std::string& str) {
  std::vector<uint32_t> vec;
  utf8_append_string_to_vector(str, vec);
  return vec;
}

void utf8_append_string_to_vector(const std::string& str, std::vector<uint32_t>& vec) {
  utf8::utf8to32(str.c_str(), str.c_str() + str.size(), std::back_inserter(vec));
}

std::string utf8_vector_to_string(const std::vector<uint32_t>& vec) {
  std::string str;
  utf8_append_vector_to_string(vec, str);
  return str;
}

void utf8_append_vector_to_string(const std::vector<uint32_t>& vec, std::string& str) {
  utf8::utf32to8(vec.begin(), vec.end(), std::back_inserter(str));
}

bool is_prefix(const std::string& prefix, const std::string& str) {
  if (str.size() < prefix.size()) return false;
  auto res = std::mismatch(prefix.begin(), prefix.end(), str.begin());
  return res.first == prefix.end();
}