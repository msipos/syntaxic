#ifndef SYNTAXIC_LMGEN_HPP
#define SYNTAXIC_LMGEN_HPP

#include "lm.hpp"

LMKey lmgen(const std::string& email);

std::string key_to_string(LMKey key);

#endif