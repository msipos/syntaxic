#ifndef SYNTAXIC_CORE_UTF8_UTIL_HPP
#define SYNTAXIC_CORE_UTF8_UTIL_HPP

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

// Convert the data trying to parse it to UTF8, but swallow errors in parsing.
std::string utf8_convert_best(const char* data, unsigned int len);

/** Return whether the string is correctly UTF-8 encoded. */
bool utf8_check(const char* data, unsigned int len);

// Convert a unicode code point to a UTF8 std string.
std::string utf8_to_string(uint32_t cp);

// Trim this character off the right side of the string.
void utf8_rtrim(std::string& str, char c);

/** Remove whitespace from the ends of this string. */
std::string utf8_strip(const std::string& str);

/** Lowercase string. */
std::string utf8_string_lower(const std::string& input);

/** String split. */
void utf8_string_split(std::vector<std::string>& output, const std::string& input, uint32_t c);
void utf8_string_split_whitespace(std::vector<std::string>& output, const std::string& input, int max_split=-1);

/** Is prefix a prefix of str? */
bool is_prefix(const std::string& prefix, const std::string& str);
/** Return common prefix. */
std::string utf8_common_prefix(const std::string& s1, const std::string& s2);

/** Fast forward count steps in a UTF8 string. */
int utf8_count(const char* s, int count);
/** Get a UTF8 character from the string. */
uint32_t utf8_get(const char* s);
/** Get length of a utf8 string. */
int utf8_size(const char* s);
/** Get length of a utf8 string. */
int utf8_size(const std::string& s);
/** Append a unicode character to UTF8 string. */
void utf8_append(std::string& a, uint32_t c);
/** Count from beginning and insert a string at index. */
void utf8_insert(std::string& a, int index, const std::string& b);
/** Count from beginning and insert a unicode character at index. */
void utf8_insert(std::string& a, int index, uint32_t c);
/** Count from beginning and remove a unicode character at index. */
void utf8_remove(std::string& a, int index);

// Iterate (read-only) over the contents of a UTF8 std::string.

class UTF8Iterator {
private:
  const char* ptr;
  const char* ptr_end;
public:
  explicit UTF8Iterator(const char* s, const char* end) : ptr(s), ptr_end(end) {}

  bool operator!=(const UTF8Iterator&) const;
  uint32_t operator*() const;
  void operator++();
};

class UTF8Wrapper {
private:
  const char* str;
  int size;

public:
  explicit UTF8Wrapper(const std::string& s) : str(s.c_str()), size(s.size()) {}
  explicit UTF8Wrapper(const char* s) : str(s), size(strlen(s)) {}
  explicit UTF8Wrapper(const char* s, int start, int end) {
    const int offset1 = utf8_count(s, start);
    const int offset2 = utf8_count(s, end);
    str = s + offset1;
    size = offset2 - offset1;
  }
  UTF8Iterator begin() const;
  UTF8Iterator end() const;
};

/** Convert an std::string into a vector<uint32_t>. */
std::vector<uint32_t> utf8_string_to_vector(const std::string& s);
void utf8_append_string_to_vector(const std::string& s, std::vector<uint32_t>& vec);

/** Convert a vector<uint32_t> into a UTF8 std::string. */
std::string utf8_vector_to_string(const std::vector<uint32_t>& vec);
void utf8_append_vector_to_string(const std::vector<uint32_t>& vec, std::string& s);

#endif