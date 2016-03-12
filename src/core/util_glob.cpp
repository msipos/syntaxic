#include "core/util_glob.hpp"
#include "core/utf8_util.hpp"

namespace UtilGlob {

bool matches(const std::string& glob, const std::string& str) {
  std::vector<uint32_t> vglob = utf8_string_to_vector(glob);

  unsigned int star_loc = vglob.size();
  for (unsigned int i = 0; i < vglob.size(); i++) {
    if (vglob[i] == '*') {
      star_loc = i;
      break;
    }
  }

  // No star
  if (star_loc == vglob.size()) return glob == str;

  int after_star = vglob.size() - star_loc - 1;
  std::vector<uint32_t> vstr = utf8_string_to_vector(str);
  if (vstr.size() < star_loc + after_star) return false;

  // Match stuff before the star
  for (unsigned int i = 0; i < star_loc; i++) {
    if (vstr[i] != vglob[i]) return false;
  }

  // Match stuff after the star
  for (int i = 0; i < after_star; i++) {
    if (vstr[vstr.size() - i - 1] != vglob[vglob.size() - i - 1]) return false;
  }
  return true;
}

}