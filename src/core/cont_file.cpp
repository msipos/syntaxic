#include "core/cont_file.hpp"
#include "core/text_file.hpp"

const Token* ContFile::token_immediately_left(){
  const Line& l = text_file->get_line(cursor.row);
  const std::vector<Token>& tokens = l.appendage().tokens;
  for (unsigned int i = 0; i < tokens.size(); i++) {
    const Token& tok = tokens[i];
    const int offset = tok.offset;
    if (offset < cursor.col) continue;

    if (offset == cursor.col) {
      if (!tok.is_start()) {
        if (i > 0) return &tokens[i-1];
        else return &tok;
      }
    } else return nullptr;
  }
  return nullptr;
}

const Token* ContFile::token_immediately_right(){
  const Line& l = text_file->get_line(cursor.row);
  const std::vector<Token>& tokens = l.appendage().tokens;
  for (const Token& tok : tokens) {
    const int offset = tok.offset;
    if (offset < cursor.col) continue;

    if (offset == cursor.col) {
      if (tok.is_start()) {
        return &tok;
      }
    } else return nullptr;
  }
  return nullptr;
}

const Token* ContFile::token_within() {
  const Line& l = text_file->get_line(cursor.row);
  const std::vector<Token>& tokens = l.appendage().tokens;
  for (int i = 0; i < int(tokens.size()) - 1; i++) {
    const Token& tok = tokens[i];
    const Token& tok2 = tokens[i+1];
    const int offset1 = tok.offset, offset2 = tok2.offset;
    if (offset1 < cursor.col && offset2 > cursor.col && tok.is_start()) {
      return &tok;
    }
    if (offset1 >= cursor.col) return nullptr;
  }
  return nullptr;
}

std::string ContFile::get_token_text(const Token* tok) {
  const Line& l = text_file->get_line(cursor.row);
  const std::vector<Token>& tokens = l.appendage().tokens;
  for (int i = 0; i < int(tokens.size()) - 1; i++) {
    const Token* t = &(tokens[i]);
    if (t == tok) {
      const Token* t2 = &(tokens[i+1]);
      return l.to_string(t->offset, t2->offset);
    }
  }
  return "";
}

const Token* ContFile::move_token_left(){
  for (int l = cursor.row; l >= 0; l--) {
    const Line& line = text_file->get_line(l);
    int start_col = line.size();
    if (l == cursor.row) start_col = cursor.col;
    const std::vector<Token>& tokens = line.appendage().tokens;

    for (int t = tokens.size() - 1; t >= 0; t--) {
      const Token& token = tokens[t];
      if (!token.is_start()) continue;
      if (token.offset < start_col) {
        // Found it
        cursor.row = l;
        cursor.col = token.offset;
        return &token;
      }
    }
  }
  return nullptr;
}

const Token* ContFile::move_token_right(){
  for (int l = cursor.row; l < text_file->get_num_lines(); l++) {
    const Line& line = text_file->get_line(l);
    int start_col = -1;
    if (l == cursor.row) start_col = cursor.col;
    const std::vector<Token>& tokens = line.appendage().tokens;

    for (unsigned int t = 0; t < tokens.size(); t++) {
      const Token& token = tokens[t];
      if (!token.is_start()) continue;
      if (token.offset >= start_col) {
        // Found it
        cursor.row = l;
        cursor.col = token.offset + 1;
        return &token;
      }
    }
  }
  return nullptr;
}

Character ContFile::move_left(){
  if (text_file->can_move_left(cursor)) {
    cursor = text_file->move_left(cursor);
    return text_file->at_cursor(cursor);
  }
  Character ch_eof;
  ch_eof.set_eof();
  return ch_eof;
}

Character ContFile::move_right(){
  if (text_file->can_move_right(cursor)) {
    Character ch = text_file->at_cursor(cursor);
    cursor = text_file->move_right(cursor);
    return ch;
  }
  Character ch_eof;
  ch_eof.set_eof();
  return ch_eof;
}