#ifndef STATLANG_STATLANG_HPP
#define STATLANG_STATLANG_HPP

#include "core/hooks.hpp"
#include "core/rich_text.hpp"
#include "core/text_file.hpp"
#include "statlang/symboldb.hpp"
#include "statlang/tokenizer.hpp"

#include <memory>
#include <string>
#include <unordered_map>

class StatLangError : public std::runtime_error {
public:
  StatLangError(const std::string& s) : std::runtime_error(s) {}
};

class Doc;
class Document;
class LanguageDefs;
class StatLangData;

struct RunningPair {
  int row, col, extra, block_num;
};

/** Each SymbolMetadata is an occurrence of the symbol. */
struct SymbolMetadata {
  int statlang_id;
  SymbolData symbol_data;
  uint8_t token_type; /** StatLangToken type, if any. */
  int block_id;
  int block_depth;
  uint32_t hashes[2];
  float definition_score;

  void debug();
};

class StatLang {
private:
  /** Map from extension to file type. */
  std::unordered_map<std::string, std::string> glob_map;
  /** Map from file type to meta file name. */
  std::unordered_map<std::string, std::string> meta_map;
  /** Map from file type to LanguageDef. */
  std::unordered_map<std::string, std::unique_ptr<LanguageDefs>> language_defs;

  /** Map of ID to internal data. */
  int max_id;
  std::unordered_map<int, std::unique_ptr<StatLangData>> internal_data;
  StatLangData* get_data_for_id(int id);

  Hook<Doc*, int> document_hook;
  void document_callback(Doc*, int);

  std::vector<RunningPair> running_pairs;

  // Helpers:
  LanguageDefs* get_language_def(int id);

public:
  StatLang();
  ~StatLang();

  /** All file types (except Text). */
  std::vector<std::string> available_file_types;

  /** Get matches from symboldb. */
  void query_symboldb(const std::string& prefix, const std::string& whole_word, std::vector<std::string>& matches, std::string& common_prefix);

  /** Initialize. */
  void init(const char* json_contents, int json_size);


  /////// StatLang processing

  /** Add a document to statlang. Return statlang id. */
  int add_document(TextBuffer* tb);

  /** Infer file type from filename. */
  void infer_document_type(int id, const std::string& filename);

  /** Explicitly set file type. */
  void set_document_type(int id, const std::string& type);

  /** Explicitly get file type. */
  std::string get_document_type(int id);

  /** Process a document and populate output variables. Rich_text can be nullptr if you don't desire any output. */
  void process_document(int id, RichText* rich_text);

  /** Highlight a document with temporary highlights. */
  void highlight_document(int id, CursorLocation cursor, RichText* rich_text);

  /** Remove a document from statlang. */
  void remove_document(int id);

  /** Get a single-line comment for a document. */
  bool get_document_comment(int id, std::string& prefix, std::string& postfix);

  /** Get information about this symbol. */
  void get_symbol_metadata_vector(const std::string& symbol, std::vector<SymbolMetadata>& metadata);

  /** Run statlang big analysis. */
  void run_analysis();
};

#endif