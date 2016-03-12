#ifndef SYNTAXIC_CORE_CONT_FILE_HPP
#define SYNTAXIC_CORE_CONT_FILE_HPP

#include "core/common.hpp"

#include <string>

class TextFile;

class ContFile {
private:
  TextFile* text_file;
  CursorLocation cursor;

public:
  ContFile(TextFile* tf, CursorLocation cur) : text_file(tf), cursor(cur) {}

  inline void go_to(CursorLocation cl) { cursor = cl; }
  inline CursorLocation get_cursor() const { return cursor; }

  /** Only returns non-nullptr if there is an end of token here. */
  const Token* token_immediately_left();
  /** Only returns non-nullptr if there is a start of token here. */
  const Token* token_immediately_right();
  /** Returns a token if we are within it. */
  const Token* token_within();

  /** Get token text.  Token is assumed to be on this line. */
  std::string get_token_text(const Token* tok);

  /** Move left until next start of token. Return this token.  If nullptr, then start of file was
  reached. */
  const Token* move_token_left();
  /** Move right until next start of token. Return this token. If nullptr, then end of file was
  reached. */
  const Token* move_token_right();

  Character move_left();
  Character move_right();
};

#endif