#ifndef SYNTAXIC_CORE_UTIL_HPP
#define SYNTAXIC_CORE_UTIL_HPP

#include <cstring>
#include <cstdint>
#include <string>
#include <vector>

#include "utf8.h"

double get_current_time();

void read_file(std::vector<char>& out, const std::string& p);
void write_file(const std::vector<char>& contents, const std::string& p);
void write_file(const char* contents, uint32_t size, const std::string& p);

/** Return "$SYNTAXIC_ROOT/filename". */
std::string under_root(const std::string& filename);

/** Return "$SYNTAXIC_ROOT/dir/filename". */
std::string under_root2(const std::string& dir, const std::string& filename);

/** Elide on the right to this many characters. */
std::string elide_right(const std::string& path, int num);

/** Return absolute path. */
std::string abs_path(const std::string& p);
/** Get last component of a path. */
std::string last_component(const std::string& p);

/** Return extension, given a filename. */
std::string extract_extension(const std::string& filename);
std::string extract_lowercase_extension(const std::string& filename);

double get_timestamp();

bool is_binary_file(const char* data, int size);


#endif