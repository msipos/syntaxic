#ifndef SYNTAXIC_STATLANG_SYMBOLDB_HPP
#define SYNTAXIC_STATLANG_SYMBOLDB_HPP

#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct SymbolData {
  std::string symbol;
  uint32_t row, col;
  int32_t token;

  inline bool operator<(const SymbolData& other) const {
    return symbol < other.symbol;
  }
};

struct Match {
  std::string str;
  int num_occurences;
};

class SymbolDatabase {
private:
  std::vector<SymbolData> data;

public:
  /** Reset symbol db to its empty state. */
  void start_adding();

  /** Add a symbol. */
  void add_symbol(const std::string& str, uint32_t row, uint32_t col);

  /** Finish adding symbols.  Sort the symbol table. */
  void finish_adding();

  void debug();

  /** Naive query by prefix. */
  void query_by_prefix(const std::string& prefix, std::unordered_map<std::string, int>& matches);

  /** Return symbol data. */
  const std::vector<SymbolData>& get_data() const { return data; }
  std::vector<SymbolData>& get_data() { return data; }

  /** Get all occurences of symbol. */
  void get_symbol(const std::string& symbol, std::vector<SymbolData>& output);
};

#endif