#ifndef SYNTAXIC_KEYMAPPER_HPP
#define SYNTAXIC_KEYMAPPER_HPP

#include "core/common.hpp"
#include "doc.hpp"

#include <unordered_map>
#include <vector>

struct ParsedKeySequence {
  bool has_alt, has_ctrl, has_meta, has_shift;
  bool is_char;
  uint32_t code;
  bool valid;
};

class KeyMapper {
private:
  std::unordered_map<int32_t, DocAction::Type> ctrl_shift_map;
  std::unordered_map<int32_t, DocAction::Type> meta_shift_map;
  std::unordered_map<int32_t, DocAction::Type> ctrl_map;
  std::unordered_map<int32_t, DocAction::Type> meta_map;
  std::unordered_map<int32_t, DocAction::Type> alt_map;
  std::unordered_map<int32_t, DocAction::Type> none_map;

public:
  inline KeyMapper() {}
  KeyMapper(const KeyMapper&) = delete;
  KeyMapper& operator=(const KeyMapper&) = delete;

  void add_key(const std::string& key, DocAction::Type action);
  void add_keys(const std::string& keys, DocAction::Type action);
  std::string add_keys_safe(const std::string& keys, DocAction::Type action);

  DocAction::Type map_key(bool is_char, int32_t code, bool shift, bool meta, bool ctrl, bool alt);
  DocAction::Type map_key(KeyPress key_press);

  static bool is_key_valid(const std::string& key);
  static bool are_keys_valid(const std::string& keys);
  static ParsedKeySequence parse_key_sequence(const std::string& key);
  static std::vector<ParsedKeySequence> parse_key_sequences(const std::string& keys);

  void clear();
};

#endif