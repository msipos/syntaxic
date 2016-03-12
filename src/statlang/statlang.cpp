#include "document.hpp"
#include "core/cont_file.hpp"
#include "core/util.hpp"
#include "core/util_glob.hpp"
#include "core/utf8_util.hpp"
#include "core/word_def.hpp"
#include "statlang/statlang.hpp"
#include "statlang/tokenizer.hpp"
#include "syntax_highlight.hpp"

#include <algorithm>
#include "json/json.h"
#include <set>

////////////////////////////////////////////////////////////// LanguageDefs

struct SyntaxPair {
  uint8_t start, end;
};

class LanguageDefs {
public:
  Tokenizer tokenizer;
  std::vector<SyntaxPair> syntax_pairs;
  std::string single_line_comment;

  LanguageDefs(const char* json_contents, int json_size);
};

LanguageDefs::LanguageDefs(const char* json_contents, int json_size)
    : tokenizer(json_contents, json_size) {
  Json::Reader json_reader;
  Json::Value json_root;
  bool parsing_successful = json_reader.parse(json_contents, json_contents+json_size, json_root);

  if (!parsing_successful) {
    throw TokenizerError("Could not parse tokenizer JSON file: "
        + json_reader.getFormattedErrorMessages() + "\n\n" + std::string(json_contents));
  }

  if (json_root.type() != Json::objectValue) {
    throw TokenizerError("Language root must be an object.");
  }

  // Parse syntax
  if (!json_root.isMember("syntax")) return;
  {
    Json::Value syntax_root = json_root["syntax"];
    if (syntax_root.isMember("pairs")) {
      Json::Value pairs_root = syntax_root["pairs"];
      for (unsigned int i = 0; i < pairs_root.size(); i++) {
        Json::Value pair = pairs_root[i];
        uint8_t open = pair["open"].asInt();
        uint8_t close = pair["close"].asInt();
        syntax_pairs.push_back({ open, close });
      }
    }
    if (syntax_root.isMember("single_line_comment")) {
      single_line_comment = syntax_root["single_line_comment"].asString();
    }
  }
}

////////////////////////////////////////////////////////////// StatLangData

struct StatLangToken {
  uint32_t start_row, start_col, end_row, end_col;
  uint8_t token_type;
  uint8_t extra;

  void debug() const {
    printf("%4d:%4d - %4d:%4d: %d, %d\n", start_row, start_col, end_row, end_col, token_type, extra);
  }
};

struct StatLangBlock {
  int token1, token2;
  int parent_block;
  int depth;
};

struct StatLangData {
  int id;
  std::string type;
  TextBuffer* text_buffer;
  std::vector<StatLangToken> tokens;
  std::vector<StatLangBlock> blocks;
  SymbolDatabase symbol_db;

  StatLangData(int _id, TextBuffer* tb) : id(_id), text_buffer(tb) { type = "Text"; }
  StatLangData() : id(0), text_buffer(nullptr) {}

  int token_starting_at(unsigned int row, unsigned int col) {
    // TODO: Switch to binary search
    for (unsigned int i = 0; i < tokens.size(); i++) {
      if (tokens[i].start_row == row) {
        if (tokens[i].start_col == col) return i;
        if (tokens[i].start_col > col) return -1;
      }
      if (tokens[i].start_row > row) return -1;
    }
    return -1;
  }

  int token_ending_at(unsigned int row, unsigned int col) {
    // TODO: Switch to binary search
    for (unsigned int i = 0; i < tokens.size(); i++) {
      if (tokens[i].end_row == row) {
        if (tokens[i].end_col == col) return i;
        if (tokens[i].end_col > col) return -1;
      }
      if (tokens[i].end_row > row) return -1;
    }
    return -1;
  }

  WordDef get_word_def() {
    return WordDef();
  }

  WordDef get_preproc_word_def() {
    WordDef word_def_preproc; word_def_preproc.set_allow_slash(); word_def_preproc.set_allow_dot();
    return word_def_preproc;
  }

  WordDef get_string_word_def() {
    WordDef word_def_string; word_def_string.set_allow_slash(); word_def_string.set_allow_dot();
    return word_def_string;
  }
};

void SymbolMetadata::debug(){
  printf("%d %4d %2d %d %d %d %f   ", token_type, symbol_data.token, block_id, block_depth, hashes[0], hashes[1], definition_score);
}

////////////////////////////////////////////////////////////// StatLang

StatLang::StatLang() : max_id(1) {}

StatLang::~StatLang() {}

void StatLang::init(const char* json_contents, int json_size) {
  // Hook up to documents
  document_hook = all_docs_hook.add(
      std::bind(&StatLang::document_callback, this, std::placeholders::_1,
      std::placeholders::_2));

  // Parse JSON
  Json::Reader json_reader;
  Json::Value json_root;
  bool parsing_successful = json_reader.parse(json_contents, json_contents+json_size, json_root);

  if (!parsing_successful) {
    throw StatLangError("Could not parse JSON file"
        + json_reader.getFormattedErrorMessages());
  }

  Json::Value stat_lang_root = json_root["stat_lang"];
  Json::Value lang_root = stat_lang_root["languages"];

  for (const std::string& lang_name : lang_root.getMemberNames()) {
    available_file_types.push_back(lang_name);
    Json::Value lang_obj = lang_root[lang_name];
    std::string meta_file = lang_obj["meta_file"].asString();
    Json::Value globs = lang_obj["globs"];

    // Map globs
    for (unsigned int i = 0; i < globs.size(); i++) {
      std::string glob = globs[i].asString();
      glob_map[glob] = lang_name;
    }

    // Map meta file
    meta_map[lang_name] = meta_file;
  }
}

void StatLang::document_callback(Doc* doc, int type) {
  bool big_file = ((doc->get_display_style() & DocFlag::BIG_DOC) != 0);

  if (type & DocEvent::OPENED) {
    // Ugly
    int id = add_document((TextBuffer*) doc->get_text_buffer());
    doc->get_appendage().statlang_id = id;
  }

  if (type & DocEvent::OPENED || type & DocEvent::CHANGED_PATH) {
    int id = doc->get_appendage().statlang_id;
    Document* document = dynamic_cast<Document*>(doc);
    if (document == nullptr) return;
    std::string file_name = utf8_string_lower(document->get_text_file()->get_file_name());
    infer_document_type(id, file_name);
    doc->get_appendage().file_type = get_document_type(id);
    if (!big_file) process_document(id, &(doc->get_appendage().rich_text));
  }

  if (type & DocEvent::EDITED || type & DocEvent::CHANGED_TYPE) {
    int id = doc->get_appendage().statlang_id;
    if (id <= 0) return;
    if (!big_file) process_document(doc->get_appendage().statlang_id, &(doc->get_appendage().rich_text));
  }

  if (type & DocEvent::EDITED || type & DocEvent::CURSOR_MOVED) {
    int id = doc->get_appendage().statlang_id;
    if (id <= 0) return;
    if (!big_file) highlight_document(id, doc->get_cursor(), &(doc->get_appendage().rich_text));
  }

  if (type & DocEvent::CLOSING) {
    int id = doc->get_appendage().statlang_id;
    if (id <= 0) return;
    remove_document(id);
  }
}

StatLangData* StatLang::get_data_for_id(int id) {
  if (internal_data.count(id) == 0) {
    printf("Warning: StatLang: no such id: %d\n", id);
    return nullptr;
  }
  return internal_data[id].get();
}

int StatLang::add_document(TextBuffer* tb) {
  const int id = max_id;
  internal_data[id] = std::unique_ptr<StatLangData>(new StatLangData(id, tb));
  max_id++;
  return id;
}

void StatLang::infer_document_type(int id, const std::string& filename) {
  if (internal_data.count(id) == 0) {
    printf("Warning: StatLang: no such id: %d\n", id);
    return;
  }
  StatLangData* sld = internal_data[id].get();
  for (auto& pair: glob_map) {
    if (UtilGlob::matches(pair.first, filename)) {
      sld->type = pair.second;
      return;
    }
  }
  sld->type = "Text";
}

void StatLang::set_document_type(int id, const std::string& type) {
  if (internal_data.count(id) == 0) {
    printf("Warning: StatLang: no such id: %d\n", id);
    return;
  }
  StatLangData* sld = internal_data[id].get();
  sld->type = type;
}

std::string StatLang::get_document_type(int id) {
  if (internal_data.count(id) == 0) {
    printf("Warning: StatLang: no such id: %d\n", id);
    return "";
  }
  StatLangData* sld = internal_data[id].get();
  return sld->type;
}

LanguageDefs* StatLang::get_language_def(int id) {
  if (internal_data.count(id) == 0) return nullptr;
  StatLangData* sld = internal_data[id].get();
  const std::string& file_type = sld->type;
  if (file_type == "Text") return nullptr;
  if (language_defs.count(file_type) == 0) {
    if (meta_map.count(file_type) == 0) {
      printf("Warning: no meta for file_type: %s\n", file_type.c_str());
      return nullptr;
    }
    std::string meta_file = meta_map[file_type];
    try {
      std::vector<char> contents;
      read_file(contents, under_root2("languages", meta_file + ".json"));
      language_defs[file_type] = std::unique_ptr<LanguageDefs>(new LanguageDefs(contents.data(), contents.size()));
    } catch (std::exception& e) {
      printf("ERROR: While trying to load language meta file for %s: %s\n", meta_file.c_str(),
          e.what());
      return nullptr;
    }
  }
  return language_defs[file_type].get();
}

void StatLang::process_document(int id, RichText* rich_text) {
  if (internal_data.count(id) == 0) {
    printf("Warning: StatLang: no such id: %d\n", id);
    return;
  }

  StatLangData* sld = internal_data[id].get();
  LanguageDefs* lang_def = get_language_def(id);
  if (lang_def == nullptr) return;
  TextBuffer* text_buffer = sld->text_buffer;
  WordDef word_def = sld->get_word_def();
  WordDef word_def_preproc = sld->get_preproc_word_def();
  WordDef word_def_string = sld->get_string_word_def();

  // STEP 1: Run a tokenizer and create token stream. In the process also run through symbols (words)

  {
    Tokenizer* tokenizer = &(lang_def->tokenizer);
    RunningTokenizer rtok(tokenizer, 0);
    // Current syntax highlighting
    int sh = 0;
    StatLangToken current_token; // This is conserved across lines
    current_token.token_type = TOKEN_TYPE_NONE;
    sld->tokens.clear();
    sld->symbol_db.start_adding();
    for (int row = 0; row < text_buffer->get_num_lines(); row++) {

      // Convert line into a string and run it in the tokenizer.
      Line& line = text_buffer->get_line(row);
      std::string str = line.to_string();
      rtok.run_tokenizer(str);

      // Color the line
      {
        unsigned int token_index = 0;
        for (unsigned int j = 0; j <= line.size(); j++) {
          while(token_index < rtok.tokens.size()) {
            const Token& token = rtok.tokens[token_index];
            if (token.offset > j) break;
            if (token.is_start()) {
              // For the time being, this is identity.
              switch(token.get_type()) {
                case TOKEN_TYPE_NUMBER: sh = SH_NUMBER; break;
                case TOKEN_TYPE_COMMENT: sh = SH_COMMENT; break;
                case TOKEN_TYPE_STRING: sh = SH_STRING; break;
                case TOKEN_TYPE_IDENT : sh = SH_IDENT ; break;
                case TOKEN_TYPE_KEYWORD : sh = SH_KEYWORD ; break;
                case TOKEN_TYPE_OTHER_KEYWORD : sh = SH_OTHER_KEYWORD ; break;
                case TOKEN_TYPE_PREPROCESSOR : sh = SH_PREPROCESSOR ; break;
                case TOKEN_TYPE_OPERATOR : sh = SH_OPERATOR ; break;
                case TOKEN_TYPE_NONE : sh = SH_NONE ; break;
                case TOKEN_TYPE_ILLEGAL: sh = SH_ERROR ; break;
              }
            } else {
              sh = 0;
            }
            token_index++;
          }
          if (j < line.size()) line.get_char(j).markup = sh;
        }
      }

      // Go through the words
      {
        int in_word = 0;
        uint32_t p = 0;
        int token_type = current_token.token_type;
        unsigned int token_index = 0;
        for (unsigned int j = 0; j < line.size(); j++) {
          const uint32_t c = line.get_char(j).c;
          WordDef* wd = &word_def;
          if (token_type == TOKEN_TYPE_PREPROCESSOR) wd = &word_def_preproc;
          else if (token_type == TOKEN_TYPE_STRING) wd = &word_def_string;

          if (in_word > 0) {
            if (!wd->continue_word(p, c)) {
              // We have a word.
              const int word_start = j - in_word;

              if (token_type != TOKEN_TYPE_KEYWORD) {
                const std::string word = line.to_string(word_start, j);
                sld->symbol_db.add_symbol(word, row, word_start);
              }
              in_word = 0;
            } else {
              in_word++;
            }
          } else {
            if (wd->start_word(c)) {
              in_word = 1;
            }
          }
          p = c;

          // Figure out token_type.
          while(token_index < rtok.tokens.size()) {
            const Token& token = rtok.tokens[token_index];
            if (token.offset > j) break;
            if (token.is_start()) token_type = token.get_type();
            else token_type = TOKEN_TYPE_NONE;
            token_index++;
          }
        }

        // Last word of the line.
        if (in_word > 0) {
          const int word_start = line.size() - in_word;

          if (token_type != TOKEN_TYPE_KEYWORD) {
            const std::string word = line.to_string(word_start, line.size());
            sld->symbol_db.add_symbol(word, row, word_start);
          }
        }
      }

      // Convert rtok tokens into statlang tokens and save them all
      for (Token& tok : rtok.tokens) {
        if (tok.is_start()) {
          current_token.start_row = row;
          current_token.start_col = tok.offset;
          current_token.token_type = tok.get_type();
          current_token.extra = tok.extra;
        } else {
          current_token.end_row = row;
          current_token.end_col = tok.offset;
          sld->tokens.push_back(current_token);
        }
      }
      rtok.tokens.clear();
    } // end for each row
  }

  // STEP 1.5: Match tokens to symbols.

  if (sld->symbol_db.get_data().size() > 0 && sld->tokens.size() > 0) {
    std::vector<SymbolData>& data = sld->symbol_db.get_data();
    unsigned int token_index = 0;
    unsigned int symbol_index = 0;
    for (;;) {
      SymbolData& sd = data[symbol_index];
      const CursorLocation cl_sd(sd.row, sd.col);
      if (symbol_index >= data.size()) break;
      if (token_index < sld->tokens.size() - 1) {
        // Token can go further
        const StatLangToken& tok = sld->tokens[token_index+1];
        const CursorLocation cl_tok(tok.start_row, tok.start_col);
        if (cl_tok > cl_sd) {
          // Gone too far.  This is it.
          sd.token = token_index;
        } else {
          // Try to go even further.
          token_index++;
          continue;
        }
      } else {
        // Last token, just keep going
        sd.token = token_index;
      }
      symbol_index++;
    }

    sld->symbol_db.finish_adding();
  }

  // STEP 2: Run through the lines and do token matching.  Expose errors in the overlays.  Build blocks.

  {
    if (rich_text) rich_text->clear_overlays(OverlayType::STATLANG_ERROR);
    running_pairs.clear();
    sld->blocks.clear();

    for (unsigned int i = 0; i < sld->tokens.size(); i++) {
      StatLangToken& token = sld->tokens[i];
      uint8_t type = token.token_type;
      uint8_t extra = token.extra;

      // Flag illegal tokens
      if (type == TOKEN_TYPE_ILLEGAL) {
        if (rich_text) rich_text->add_overlay(text_buffer, token.start_row, token.start_col, OverlayType::STATLANG_ERROR, "Invalid token.");
      }

      // Flag mismatches
      if (extra > 0) {
        for (SyntaxPair& sp : lang_def->syntax_pairs) {
          if (sp.start == extra) {
            // This is an opening bracket
            int parent_block = -1;
            if (!running_pairs.empty()) parent_block = running_pairs.back().block_num;
            sld->blocks.push_back( { int(i), -1, parent_block, 0 });
            running_pairs.push_back( { (int) token.start_row, (int) token.start_col, sp.start, int(sld->blocks.size() - 1)});
            break;
          } else if (sp.end == extra) {
            // This is a closing bracket
            if (running_pairs.empty()) {
              if (rich_text) rich_text->add_overlay(text_buffer, token.start_row, token.start_col, OverlayType::STATLANG_ERROR, "No matching block to close.");
              break;
            }

            if (running_pairs.back().extra != sp.start) {
              RunningPair rp = running_pairs.back();

              // Now run backwards on the stack and unwind it until we hit the matching open
              // token, but only if there is one.

              bool any_matching_tokens = false;
              for (RunningPair& rp: running_pairs) {
                if (rp.extra == sp.start) {
                  any_matching_tokens = true;
                  break;
                }
              }

              if (any_matching_tokens) {
                for (int j = running_pairs.size() - 1; j >= 0; j--) {
                  RunningPair rp = running_pairs[j];
                  running_pairs.erase(running_pairs.begin() + j);
                  if (rp.extra == sp.start) break;
                }
                if (rich_text) rich_text->add_overlay(text_buffer, rp.row, rp.col, OverlayType::STATLANG_ERROR, "Close token not matching.");
              } else {
                if (rich_text) rich_text->add_overlay(text_buffer, token.start_row, token.start_col, OverlayType::STATLANG_ERROR, "Open token not matching.");
              }

              break;
            } else {
              sld->blocks[running_pairs.back().block_num].token2 = i;
              running_pairs.pop_back();
            }
          }
        }
      }
    }

    if (!running_pairs.empty()) {
      for (RunningPair rp : running_pairs) {
        if (rich_text) rich_text->add_overlay(text_buffer, rp.row, rp.col, OverlayType::STATLANG_ERROR, "Not closed.");
      }
    }
  }

  // STEP 3: Go through blocks and calculate their depth

  {
    for (unsigned int i = 0; i < sld->blocks.size(); i++) {
      int depth = 1;
      int cur = i;
      for (;;) {
        int parent = sld->blocks[cur].parent_block;
        if (parent < 0) break;
        // Something's wrong:
        if (parent == cur) break;
        cur = parent;
        depth++;
      }
      sld->blocks[i].depth = depth;
    }
  }
}

static void highlight_parens(StatLangData* sld, LanguageDefs* lang_def, RichText* rich_text, int token_num) {
  if (token_num < 0) return;

  int extra_push = sld->tokens[token_num].extra;
  // Figure out the match.
  int extra_pop = -1, direction;
  for (const SyntaxPair& sp : lang_def->syntax_pairs) {
    if (extra_push == sp.start) {
      extra_pop = sp.end;
      direction = 1;
      break;
    } else if (extra_push == sp.end) {
      extra_pop = sp.start;
      direction = -1;
      break;
    }
  }
  if (extra_pop == -1) return;

  int stack = 1;
  for (;;) {
    const StatLangToken* slt;
    if (direction > 0) {
      token_num++;
      if (token_num < int(sld->tokens.size())) slt = &(sld->tokens[token_num]);
      else return;
    } else {
      token_num--;
      if (token_num >= 0) slt = &(sld->tokens[token_num]);
      else return;
    }

    int extra = slt->extra;
    if (extra == extra_push) {
      stack++;
    } else if (extra == extra_pop) {
      stack--;
      if (stack == 0) {
        Highlight hl(slt->start_row, slt->start_col, slt->end_col);
        hl.type = HIGHLIGHT_TEMPORARY;
        rich_text->highlights.temporary.push_back(hl);
        break;
      }
    }
  }
}

void StatLang::highlight_document(int id, CursorLocation cursor, RichText* rich_text) {
  if (internal_data.count(id) == 0) {
    printf("Warning: StatLang: no such id: %d\n", id);
    return;
  }

  StatLangData* sld = internal_data[id].get();
  rich_text->highlights.temporary.clear();

  // Do word highlighting.

  std::string word = sld->text_buffer->get_word(cursor, sld->get_word_def());
  if (!word.empty()) {
    std::vector<SearchResult> results;
    sld->text_buffer->search_word(word, results);
    // Highlight the results.
    for (const SearchResult& sr: results) {
      rich_text->highlights.temporary.push_back(Highlight(sr.row, sr.col, sr.col + sr.size));
    }
  }

  // Do paren highlighting.
  {
    LanguageDefs* lang_def = get_language_def(id);
    if (lang_def != nullptr) {
      highlight_parens(sld, lang_def, rich_text, sld->token_starting_at(cursor.row, cursor.col));
      highlight_parens(sld, lang_def, rich_text, sld->token_ending_at(cursor.row, cursor.col));
    }
  }

  rich_text->highlights.sort_temporary_highlights();
}

void StatLang::remove_document(int id) {
  internal_data.erase(id);
}

/// OTHER

bool StatLang::get_document_comment(int id, std::string& prefix, std::string& postfix) {
  LanguageDefs* lang_def = get_language_def(id);
  if (lang_def == nullptr) return false;
  prefix = lang_def->single_line_comment;
  return true;
}

void StatLang::query_symboldb(const std::string& prefix, const std::string& whole_word, std::vector<std::string>& matches, std::string& common_prefix) {

  std::unordered_map<std::string, int> match_map;

  for (auto& pair: internal_data) {
    SymbolDatabase& sdb = pair.second->symbol_db;
    sdb.query_by_prefix(prefix, match_map);
  }

  // Get rid of 1 instance of whole word.
  if (match_map.count(whole_word) > 0) {
    match_map[whole_word] -= 1;
    if (match_map[whole_word] == 0) match_map.erase(whole_word);
  }

  for (auto& pair: match_map) {
    matches.push_back(pair.first);
  }
  std::sort(matches.begin(), matches.end());
  if (matches.empty()) return;
  common_prefix = matches[0];
  for (std::string& match: matches) {
    common_prefix = utf8_common_prefix(common_prefix, match);
  }
}

static uint32_t get_token_hash(StatLangData* sld, int token_num) {
  uint32_t h = 0;
  uint8_t* hb = (uint8_t*) &h;
  if (token_num >= 0 && token_num < int(sld->tokens.size())) {
    StatLangToken& token = sld->tokens[token_num];
    hb[0] = token.token_type;
    hb[1] = (uint8_t) (int(token.end_col) - int(token.start_col));
    hb[2] = (uint8_t) (sld->text_buffer->get_line(token.start_row).get_char(token.start_col).c);
    if (hb[1] > 1) {
      hb[3] = (uint8_t) (sld->text_buffer->get_line(token.start_row).get_char(token.start_col+1).c);
    }
  }
  return h;
}

#define HASH_PENALTY 0.1
#define BLOCK_PENALTY 0.98
#define MAX_HASH_PENALTY 10

struct SortMetadataByDefinitionScore {
  bool operator()( const SymbolMetadata& left, const SymbolMetadata& right ) const { return left.definition_score < right.definition_score; }
};

void StatLang::get_symbol_metadata_vector(const std::string& symbol, std::vector<SymbolMetadata>& metadata) {
  for (auto& pair: internal_data) {
    StatLangData* sld = pair.second.get();
    SymbolDatabase& sdb = sld->symbol_db;

    std::vector<SymbolData> symbol_data;
    sdb.get_symbol(symbol, symbol_data);
    for (auto& sd: symbol_data) {
      SymbolMetadata sm;
      sm.statlang_id = pair.first;
      sm.symbol_data = sd;
      sm.token_type = 0;
      sm.block_id = -1;
      sm.block_depth = 0;
      if (sd.token >= 0 && sd.token < int(sld->tokens.size())) {
        sm.token_type = sld->tokens[sd.token].token_type;
        for (unsigned int i = 0; i < sld->blocks.size(); i++) {
          if (sld->blocks[i].token1 <= sd.token && sld->blocks[i].token2 >= sd.token) {
            sm.block_id = i;
            sm.block_depth = sld->blocks[i].depth;
          }
        }
      }
      sm.hashes[0] = get_token_hash(sld, sd.token-1);
      sm.hashes[1] = get_token_hash(sld, sd.token+1);
      sm.definition_score = -sm.block_depth*BLOCK_PENALTY;
      metadata.push_back(sm);
    }
  }

  // Calculate definition_score
  {
    std::unordered_map<uint32_t, int> hash_map;
    for (SymbolMetadata& sm: metadata) {
      uint32_t hash = 0;
      for (int hash_num = 0; hash_num < 2; hash_num++) {
        hash ^= sm.hashes[hash_num];
      }
      if (hash_map.count(hash) > 0) {
        hash_map[hash]++;
      } else {
        hash_map[hash] = 1;
      }
    }
    for (SymbolMetadata& sm: metadata) {
      uint32_t hash = 0;
      for (int hash_num = 0; hash_num < 2; hash_num++) {
        hash ^= sm.hashes[hash_num];
      }
      int penalty = hash_map[hash];
      if (penalty > MAX_HASH_PENALTY) penalty = MAX_HASH_PENALTY;
      if (hash_map.count(hash) > 0) sm.definition_score -= HASH_PENALTY*penalty;
    }
    hash_map.clear();
  }

  std::sort(metadata.begin(), metadata.end(), SortMetadataByDefinitionScore());
}

static void debug_context(StatLangData* sld, int row) {
  if (sld == nullptr) return;
  printf("  (%04d)  %s\n", row, sld->text_buffer->get_line(row).to_string().c_str());
}

void StatLang::run_analysis() {
  // Get all symbols
  std::set<std::string> all_symbols;

  for (auto& pair: internal_data) {
    SymbolDatabase& sdb = pair.second->symbol_db;
    const std::vector<SymbolData>& data = sdb.get_data();
    for (const SymbolData& sd : data) {
      all_symbols.insert(sd.symbol);
    }
  }

  for (const std::string& s: all_symbols) {
    printf("%s\n", s.c_str());
    std::vector<SymbolMetadata> metadata;
    get_symbol_metadata_vector(s, metadata);
    for (auto& md: metadata) {
      md.debug();
      debug_context(get_data_for_id(md.statlang_id), md.symbol_data.row);
    }
  }

//  for (auto& pair: internal_data) {
//    const std::vector<StatLangBlock>& blocks = pair.second->blocks;
//    const std::vector<StatLangToken>& tokens = pair.second->tokens;
//    for (auto& b : blocks) {
//
//      printf("%d %d %d\n", b.token1, b.token2, b.parent_block);
//      tokens[b.token1].debug();
//      if (b.token2 > 0) {
//        tokens[b.token2].debug();
//      }
//    }
//  }
}