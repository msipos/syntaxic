#ifndef CORE_UTIL_PATH_HPP
#define CORE_UTIL_PATH_HPP

#include <string>
#include <vector>

namespace UtilPath {
  /** Whatever it is, return it in absolute path form. */
  std::string to_absolute(const std::string& path);

  /** Whatever it is, return its parent path. If it was absolute path, the return value is absolute.
   */
  std::string parent_components(const std::string& path);

  /** Break off the last component of the path. */
  std::string last_component(const std::string& path);

  /** Join 2 components intelligently. */
  std::string join_components(const std::string& p1, const std::string& p2);

  /** Is it an existing file? */
  bool is_existing_file(const std::string& path);

  /** Get home path. */
  std::string home_path();

  std::string get_extension(const std::string& filename);
  std::string get_lowercase_extension(const std::string& filename);
  std::string get_basename(const std::string& filename);

  std::vector<std::string> walk(const std::string& dir);

  /** Get temp file. */
  std::string temp_file();
}

#endif