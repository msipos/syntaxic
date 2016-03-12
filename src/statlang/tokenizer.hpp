#ifndef STATLANG_TOKENIZER_HPP
#define STATLANG_TOKENIZER_HPP

#include "core/common.hpp"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

class TokenizerError : public std::runtime_error {
public:
  TokenizerError(const std::string& s) : std::runtime_error(s) {}
};

class Mode;
class RunningTokenizer;

class Tokenizer {
  friend class RunningTokenizer;
  std::unique_ptr<Mode> root_mode;
  /** True language keywords. */
  std::unordered_set<std::string> keywords;
  /** Other keywords, i.e. builtins, etc. */
  std::unordered_set<std::string> other_keywords;

public:
  Mode* get_mode(int mode_id);

  bool case_insensitive;

  Tokenizer(const char* json_contents, int json_size);
  ~Tokenizer();
};


class RunningTokenizer {
private:
  Tokenizer* tokenizer;
  Mode* current_mode;
  std::vector<int> matches;
  std::vector<Mode*> mode_stack;

  void emit_token(int index, uint8_t mode_id, SLTokenType type, bool is_start, uint8_t extra = 0);

public:
  RunningTokenizer(Tokenizer* t, int start_mode);
  std::vector<Token> tokens;

  void run_tokenizer(const std::string& line);
};

#endif