#ifndef SYNTAXIC_CORE_WORD_DEF_HPP
#define SYNTAXIC_CORE_WORD_DEF_HPP

#include <cctype>
#include <cstdint>
#include <string>
#include <vector>

// TODO: Add '-' optionally for words and !? for endings
class WordDef {
private:
  bool allow_slash, allow_dot;

public:
  inline WordDef() : allow_slash(false), allow_dot(false) {}

  inline void set_allow_slash() { allow_slash = true; }
  inline void set_allow_dot() { allow_dot = true; }

  inline bool start_word(uint32_t c) {
    if (isalpha(c) || c == '_' || c == '$') return true;
    if (allow_slash && (c == '/' || c == '\\')) return true;
    if (allow_dot && c == '.') return true;
    return false;
  }

  inline bool continue_word(uint32_t /* p */, uint32_t c) {
    if (isalnum(c) || c == '_' || c == '$') return true;
    if (allow_slash && (c == '/' || c == '\\')) return true;
    if (allow_dot && c == '.') return true;
    return false;
  }

  inline bool backtrack_word(uint32_t c) {
    if (isalnum(c) || c == '_' || c == '$') return true;
    if (allow_slash && (c == '/' || c == '\\')) return true;
    if (allow_dot && c == '.') return true;
    return false;
  }
};

struct WordLoc {
  std::string word;
  int start;
  int size;
  int start_utf8;
  int size_utf8;
};

std::vector<WordLoc> split_words(const std::string& text, WordDef wd);

#endif