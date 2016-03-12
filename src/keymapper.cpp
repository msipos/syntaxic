#include "core/utf8_util.hpp"
#include "keymapper.hpp"

std::string KeyMapper::add_keys_safe(const std::string& keys, DocAction::Type action) {
  std::string output;
  try {
    add_keys(keys, action);
  } catch (std::exception& e) {
    output += e.what();
    output += "\n";
  }
  return output;
}

void KeyMapper::add_keys(const std::string& keys, DocAction::Type action) {
  std::vector<std::string> splitted;
  utf8_string_split(splitted, keys, '|');
  for (std::string s : splitted) {
    s = utf8_strip(s);
    if (!s.empty()) add_key(s, action);
  }
}

#define CTRL_NAME "ctrl"
#define META_NAME "meta"
#ifdef CMAKE_MACOSX
#define CTRL_NAME "cmd"
#define META_NAME "ctrl"
#endif

void KeyMapper::add_key(const std::string& key, DocAction::Type action) {
  ParsedKeySequence psk = parse_key_sequence(key);
  if (!psk.valid) throw std::runtime_error("Invalid key: '" + key + "'");

  if (!psk.is_char) {
    psk.code *= -1;
  }

  if (psk.has_shift && psk.has_meta) {
    meta_shift_map[psk.code] = action; return;
  }
  if (psk.has_shift && psk.has_ctrl) {
    ctrl_shift_map[psk.code] = action; return;
  }
  if (psk.has_meta) {
    meta_map[psk.code] = action; return;
  }
  if (psk.has_ctrl) {
    ctrl_map[psk.code] = action; return;
  }
  if (psk.has_alt) {
    alt_map[psk.code] = action; return;
  }
  none_map[psk.code] = action;
}

DocAction::Type KeyMapper::map_key(KeyPress key_press) {
  if (key_press.is_char) {
    return map_key(key_press.is_char, key_press.char_code, key_press.shift, key_press.meta, key_press.ctrl, key_press.alt);
  } else {
    return map_key(key_press.is_char, key_press.key, key_press.shift, key_press.meta, key_press.ctrl, key_press.alt);
  }
}

DocAction::Type KeyMapper::map_key(bool is_char, int32_t code, bool shift, bool meta, bool ctrl, bool alt) {
  if (!is_char) code *= -1;
  if (meta && shift) {
    if (meta_shift_map.count(code) > 0) {
      return meta_shift_map[code];
    }
  }
  if (meta) {
    if (meta_map.count(code) > 0) {
      return meta_map[code];
    }
  }
  if (ctrl && shift) {
    if (ctrl_shift_map.count(code) > 0) {
      return ctrl_shift_map[code];
    }
  }
  if (ctrl) {
    if (ctrl_map.count(code) > 0) {
      return ctrl_map[code];
    }
  }
  if (alt) {
    if (alt_map.count(code) > 0) {
      return alt_map[code];
    }
  }
  if (none_map.count(code) > 0) {
    return none_map[code];
  }
  return DocAction::NONE;
}

bool KeyMapper::are_keys_valid(const std::string& keys) {
  std::vector<std::string> splitted;
  utf8_string_split(splitted, keys, '|');
  for (const std::string& s : splitted) {
    std::string stripped = utf8_strip(s);
    if (!is_key_valid(s)) return false;
  }
  return true;
}

bool KeyMapper::is_key_valid(const std::string& key) {
  ParsedKeySequence psk = parse_key_sequence(key);
  return psk.valid;
}

ParsedKeySequence KeyMapper::parse_key_sequence(const std::string& key) {
  ParsedKeySequence psk;
  psk.has_alt = false; psk.has_ctrl = false; psk.has_meta = false; psk.has_shift = false;
  psk.is_char = true;
  psk.code = 0;
  psk.valid = true;

  std::string skey = utf8_string_lower(key);
  std::vector<std::string> splitted;
  utf8_string_split(splitted, skey, '-');

  for (std::string s : splitted) {
    if (s == CTRL_NAME) {
      if (psk.has_ctrl) { psk.valid = false; return psk; }
      psk.has_ctrl = true;
      continue;
    }
    if (s == META_NAME) {
      if (psk.has_meta) { psk.valid = false; return psk; }
      psk.has_meta = true;
      continue;
    }
    if (s == "shift") {
      if (psk.has_shift) { psk.valid = false; return psk; }
      psk.has_shift = true;
      continue;
    }
    if (s == "alt") {
      if (psk.has_alt) { psk.valid = false; return psk; }
      psk.has_alt = true;
      continue;
    }
    if (s == "left") { psk.is_char = false; psk.code = KEY_LEFT_ARROW; continue; }
    if (s == "right") { psk.is_char = false; psk.code = KEY_RIGHT_ARROW; continue; }
    if (s == "up") { psk.is_char = false; psk.code = KEY_UP_ARROW; continue; }
    if (s == "down") { psk.is_char = false; psk.code = KEY_DOWN_ARROW; continue; }
    if (s == "pageup") { psk.is_char = false; psk.code = KEY_PAGE_UP; continue; }
    if (s == "pagedown") { psk.is_char = false; psk.code = KEY_PAGE_DOWN; continue; }
    if (s == "home") { psk.is_char = false; psk.code = KEY_HOME; continue; }
    if (s == "end") { psk.is_char = false; psk.code = KEY_END; continue; }
    if (s == "delete") { psk.is_char = false; psk.code = KEY_DELETE; continue; }
    if (s == "tab") { psk.code = '\t'; continue; }
    if (s == "backspace") { psk.code = '\b'; continue; }
    if (s == "space") { psk.code = ' '; continue; }
    if (s.size() == 1) { psk.code = s[0]; continue; }
    psk.valid = false; return psk;
  }

  if (psk.code == 0) psk.valid = false;

  return psk;
}

std::vector<ParsedKeySequence> KeyMapper::parse_key_sequences(const std::string& keys) {
  std::vector<ParsedKeySequence> output;
  std::vector<std::string> splitted;
  utf8_string_split(splitted, keys, '|');
  for (const std::string& s : splitted) {
    std::string stripped = utf8_strip(s);
    output.push_back(parse_key_sequence(stripped));
  }
  return output;
}

void KeyMapper::clear() {
  meta_shift_map.clear();
  meta_map.clear();
  ctrl_shift_map.clear();
  ctrl_map.clear();
  alt_map.clear();
  none_map.clear();
}