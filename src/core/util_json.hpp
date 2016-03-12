#ifndef CORE_UTIL_JSON_HPP
#define CORE_UTIL_JSON_HPP

#include "json/json.h"
#include <stdexcept>

inline Json::Value parse_json(const char* contents, uint32_t size) {
  Json::Reader json_reader;
  Json::Value json_root;
  bool parsing_successful = json_reader.parse(contents, contents+size, json_root);
  if (!parsing_successful) {
    throw std::runtime_error("Could not parse JSON file: "
        + json_reader.getFormattedErrorMessages());
  }
  return json_root;
}

#endif