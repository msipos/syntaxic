#include "core/word_def.hpp"

#include "core/utf8_util.hpp"

std::vector<WordLoc> split_words(const std::string& text, WordDef wd) {
  std::vector<WordLoc> words;

  std::vector<uint32_t> vec = utf8_string_to_vector(text);
  // This will handle the end.
  vec.push_back(0);

  bool in_word = false;
  unsigned int start_word_utf8 = 0;
  uint32_t p = 0;
  for (unsigned int i = 0; i < vec.size(); i++) {
    uint32_t c = vec[i];
    if (!in_word) {
      if (wd.start_word(c)) {
        in_word = true;
        start_word_utf8 = i;
      }
    } else {
      if (!wd.continue_word(p, c)) {
        in_word = false;
        int start_word = utf8_count(text.c_str(), start_word_utf8);
        int end_word = utf8_count(text.c_str(), i);
        std::string word = text.substr(start_word, end_word - start_word);
        words.push_back({word, start_word, end_word - start_word, int(start_word_utf8), int(i - start_word_utf8)});
      }
    }
    p = c;
  }

  return words;
}
