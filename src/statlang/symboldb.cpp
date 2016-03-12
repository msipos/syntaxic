#include "core/utf8_util.hpp"
#include "statlang/symboldb.hpp"

#include <algorithm>

void SymbolDatabase::start_adding() {
  data.clear();
}

void SymbolDatabase::add_symbol(const std::string& str, uint32_t row, uint32_t col) {
  data.push_back({str, row, col, -1});
}

void SymbolDatabase::finish_adding() {
  std::sort(data.begin(), data.end());
}

void SymbolDatabase::debug() {
  for (SymbolData& sd : data) {
    printf("%3d %3d %s\n", sd.row, sd.col, sd.symbol.c_str());
  }
}

void SymbolDatabase::query_by_prefix(const std::string& prefix, std::unordered_map<std::string, int>& matches) {
  SymbolData fake = {prefix, 0, 0, -1};
  std::vector<SymbolData>::iterator iter = std::lower_bound(data.begin(), data.end(), fake);
  while (iter != data.end()) {
    if (is_prefix(prefix, iter->symbol)) {
      if (matches.count(iter->symbol) > 0) {
        matches[iter->symbol]++;
      } else {
        matches[iter->symbol] = 1;
      }
    } else {
      break;
    }
    iter++;
  }
}

void SymbolDatabase::get_symbol(const std::string& symbol, std::vector<SymbolData>& output) {
  SymbolData fake = {symbol, 0, 0, -1};
  std::vector<SymbolData>::iterator iter = std::lower_bound(data.begin(), data.end(), fake);
  while (iter != data.end()) {
    if (iter->symbol != symbol) return;

    output.push_back(*iter);
    iter++;
  }
}