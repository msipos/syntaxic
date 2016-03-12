#include "core/utf8_util.hpp"
#include "statlang/tokenizer.hpp"

#include "json/json.h"
#include "myre2.hpp"
#include "utf8.h"

class Mode {
public:
  uint16_t mode_id;

  // Results of parsing the JSON:
  SLTokenType type;
  std::string begin;
  std::string end;
  int end_id;
  std::string illegal;
  int illegal_id;
  int extra;
  bool escape_line_end;

  // Submodes:
  std::vector<std::unique_ptr<Mode>> contains;

  // Regular expressions:
  // We actually need both a RE2::Set and our own vector.
  // TODO: Investigate hacking RE2 to allow this without duplication:
  RE2::Set re_set;
  std::vector<std::unique_ptr<RE2>> regexes;

  inline Mode(uint16_t m) : mode_id(m), type(TOKEN_TYPE_NONE), end_id(-1), illegal_id(-1), extra(0), escape_line_end(false), re_set(RE2::Options(), RE2::ANCHOR_START) {}

  /** Prepare for operation.

    Note on ids:  0 through (contains.size()-1) refer to children's begin REs.
    Other ids are end_id and illegal_id, if any.
    */
  void compile() {
    //// Compile children:

    for (auto& child: contains) {
      child->compile();
    }

    //// Compile self:

    // Compile each childs begin.
    for (auto& child: contains) {
      std::string error;
      int id = re_set.Add(child->begin, &error);
      if (id < 0) {
        throw TokenizerError("Invalid regex: " + error);
      }
      regexes.push_back(std::unique_ptr<RE2>(new RE2(child->begin)));
    }
    // Compile end:
    if (!end.empty()) {
      std::string error;
      end_id = re_set.Add(end, &error);
      if (end_id < 0) {
        throw TokenizerError("Invalid regex for end: " + error);
      }
      regexes.push_back(std::unique_ptr<RE2>(new RE2(end)));
    }
    // Compile illegal:
    if (!illegal.empty()) {
      std::string error;
      illegal_id = re_set.Add(illegal, &error);
      if (illegal_id < 0) {
        throw TokenizerError("Invalid regex for illegal: " + error);
      }
      regexes.push_back(std::unique_ptr<RE2>(new RE2(illegal)));
    }
    re_set.Compile();
  }

  /** Build from JSON. */
  uint16_t from_json(Tokenizer* tokenizer, Json::Value& json_obj) {
    uint16_t next_id = mode_id + 1;
    for (const std::string& member : json_obj.getMemberNames()) {
      if (member == "contains") {
        Json::Value array = json_obj["contains"];
        for (unsigned int i = 0; i < array.size(); i++) {
          std::unique_ptr<Mode> child(new Mode(next_id));
          next_id = child->from_json(tokenizer, array[i]);
          contains.push_back(std::move(child));
        }
      } else if (member == "className") {
        std::string class_name = json_obj["className"].asString();
        if (class_name == "number") {
          type = TOKEN_TYPE_NUMBER;
        } else if (class_name == "comment") {
          type = TOKEN_TYPE_COMMENT;
        } else if (class_name == "string") {
          type = TOKEN_TYPE_STRING;
        } else if (class_name == "ident") {
          type = TOKEN_TYPE_IDENT;
        } else if (class_name == "operator") {
          type = TOKEN_TYPE_OPERATOR;
        } else if (class_name == "preprocessor") {
          type = TOKEN_TYPE_PREPROCESSOR;
        } else if (class_name == "keyword") {
          type = TOKEN_TYPE_KEYWORD;
        } else {
          printf("WARNING: Unknown class_name: %s\n", class_name.c_str());
        }
      } else if (member == "begin") {
        begin = json_obj["begin"].asString();
      } else if (member == "end") {
        end = json_obj["end"].asString();
      } else if (member == "illegal") {
        illegal = json_obj["illegal"].asString();
      } else if (member == "extra") {
        extra = json_obj["extra"].asInt();
      } else if (member == "case_insensitive") {
        tokenizer->case_insensitive = json_obj["case_insensitive"].asBool();
      } else if (member == "escape_line_end") {
        escape_line_end = json_obj["escape_line_end"].asBool();
      } else {
        printf("Warning: uknown: %s\n", member.c_str());
      }
    }
    return next_id;
  }

  Mode* get_mode_by_id(int m) {
    if (mode_id == m) return this;
    for (auto& child: contains) {
      Mode* ptr = child->get_mode_by_id(m);
      if (ptr != nullptr) return ptr;
    }
    return nullptr;
  }
};

Tokenizer::Tokenizer(const char* json_contents, int json_size) : case_insensitive(false) {
  Json::Reader json_reader;
  Json::Value json_root;
  bool parsing_successful = json_reader.parse(json_contents, json_contents+json_size, json_root);

  if (!parsing_successful) {
    throw TokenizerError("Could not parse tokenizer JSON file: "
        + json_reader.getFormattedErrorMessages() + "\n\n" + std::string(json_contents));
  }

  if (json_root.type() != Json::objectValue) {
    throw TokenizerError("Tokenizer root must be an object.");
  }

  // Parse metadata
  {
    Json::Value meta_root = json_root["meta"];
    for (std::string& member: meta_root.getMemberNames()) {
      std::vector<std::string> parts;
      utf8_string_split(parts, meta_root[member].asString(), ' ');
      for (std::string& str: parts) {
        if (member == "keyword") {
          keywords.insert(str);
        } else {
          other_keywords.insert(str);
        }
      }
    }
  }

  root_mode = std::unique_ptr<Mode>(new Mode(0));
  root_mode->from_json(this, json_root["root"]);
  root_mode->compile();
}

Mode* Tokenizer::get_mode(int mode_id) {
  return root_mode->get_mode_by_id(mode_id);
}

Tokenizer::~Tokenizer() {}

RunningTokenizer::RunningTokenizer(Tokenizer* t, int start_mode) : tokenizer(t){
  current_mode = tokenizer->get_mode(start_mode);
}

void RunningTokenizer::emit_token(int index, uint8_t mode_id, SLTokenType type, bool is_start,
    uint8_t extra) {
  if (type == TOKEN_TYPE_NONE) {
    /** Ignore these! */
    return;
  }

  Token token;
  token.offset = index;
  token.mode_id = mode_id;
  token.set_type(type);
  token.set_start(is_start);
  token.extra = extra;
  tokens.push_back(token);
}

void RunningTokenizer::run_tokenizer(const std::string& str) {
  const char* start = str.c_str(); // Raw to start of token.
  const char* cur = str.c_str(); // Raw to end of token.
  const char* const end = str.c_str() + str.size(); // Raw to end of line.
  re2::StringPiece cur_sp(cur);

  // This is for UTF8 indexing
  const char* ucur = str.c_str(); // Steps over UTF8 characters.
  int uidx = 0; // UTF8 index of end of token.

  for (;;) {
    if (cur > end) return;
    RE2::Set& set = current_mode->re_set;

    matches.clear();
    const bool rv = set.Match(cur, &matches);
    if (rv) {
      int first_match = matches.front();

      RE2* regex = current_mode->regexes[first_match].get();
      RE2::Consume(&cur_sp, *regex);
      {
        const int amount_consumed = cur_sp.data() - cur;
        start = cur;
        cur += amount_consumed;
      }

      const int uidx_start = uidx; // UTF8 index of start of token.
      // Forward uidx.
      while (ucur < cur) {
        utf8::next(ucur, end);
        uidx++;
      }

      if (first_match == current_mode->end_id) {
        // This was an end. Therefore, emit an end token.
        emit_token(uidx, current_mode->mode_id, current_mode->type, false);

        current_mode = mode_stack.back();
        mode_stack.pop_back();
      } else if (first_match == current_mode->illegal_id) {
        if (!mode_stack.empty()) {
          emit_token(uidx_start, current_mode->mode_id, current_mode->type, false);
          current_mode = mode_stack.back();
          mode_stack.pop_back();
        }
        emit_token(uidx_start, current_mode->mode_id, TOKEN_TYPE_ILLEGAL, true);
        emit_token(uidx, current_mode->mode_id, TOKEN_TYPE_ILLEGAL, false);
      } else {
        Mode* child_mode = current_mode->contains[first_match].get();
        if (!child_mode->end.empty()) {
          mode_stack.push_back(current_mode);
          current_mode = child_mode;
          emit_token(uidx_start, current_mode->mode_id, current_mode->type, true,
              current_mode->extra);
        } else {
          // This token signifies that the line end should be escaped, hence don't allow the end token to trigger and instead return now.
          if (child_mode->escape_line_end) {
            return;
          }

          // This could be an identifier
          SLTokenType token_type = child_mode->type;
          if (token_type == TOKEN_TYPE_IDENT) {
            // This is the string for the identifier.
            std::string ident(start, cur - start);
            if (tokenizer->case_insensitive) ident = utf8_string_lower(ident);
            if (tokenizer->keywords.count(ident) > 0) {
              token_type = TOKEN_TYPE_KEYWORD;
            } else if (tokenizer->other_keywords.count(ident) > 0) {
              token_type = TOKEN_TYPE_OTHER_KEYWORD;
            }
          }
          emit_token(uidx_start, child_mode->mode_id, token_type, true, child_mode->extra);
          emit_token(uidx, child_mode->mode_id, token_type, false);
        }
      }
    } else {
      if (ucur == end) return;
      utf8::next(ucur, end);
      uidx++;
      int amount = ucur - cur;
      cur += amount;
      cur_sp.remove_prefix(amount);
    }
  }
}