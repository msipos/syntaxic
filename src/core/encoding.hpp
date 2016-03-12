#ifndef SYNTAXIC_CORE_ENCODING_HPP
#define SYNTAXIC_CORE_ENCODING_HPP

#include <stdexcept>
#include <string>
#include <vector>

struct EncodingPair {
  const char* name;
  const char* extended;
};

extern EncodingPair ENCODINGS[];
extern int NUM_ENCODINGS;

std::vector<char> utf8_to_encoding(const char* codec_name, const std::string& str);
std::string encoding_to_utf8(const char* codec_name, const char* buf, unsigned int sz);
bool encoding_possible(const char* codec_name, const std::string& str);
const char* codec_extended_to_name(const char* extended);

class EncodingError : public std::runtime_error {
public:  EncodingError(const std::string& what) : std::runtime_error(what) {}
};

#endif