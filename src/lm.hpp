#ifndef SYNTAXIC_LM_HPP
#define SYNTAXIC_LM_HPP

#include <cstdint>
#include <string>

struct LMKey {
  uint8_t b[20];
};

LMKey string_to_key(const std::string& key);
// arr must be 20 bytes long.
void email_to_array(const std::string& email, uint8_t* arr);
bool well_formed(LMKey key);
bool well_formed(const std::string& key);
bool check(LMKey key, const std::string& email);

#endif