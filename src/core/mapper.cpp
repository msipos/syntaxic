#include "core/mapper.hpp"

#include "utf8.h"

Mapper::Mapper(const std::string& s, int r0) : row0_offset(r0), str(s) {
  // Make indices:
  const char* const start = s.c_str();
  const char* const end = s.c_str() + s.size();
  const char* cur = start;
  for(;;) {
    if (cur == end) break;
    if (*cur == '\n') {
      indices.push_back(cur - start + 1);
    }
    utf8::next(cur, end);
  }  
}

int Mapper::map_row(int index) {
  for (unsigned int i = 0; i < indices.size(); i++) {
    if ((uint32_t) index < indices[i]) return i;
  }
  return indices.size();
}

int Mapper::map_col(int row, int index) {
  int line_index = 0;
  if (row > 0) {
    line_index = indices[row-1];
  }
  const char* const start = str.c_str();
  const char* const end = str.c_str() + str.size();
  const char* cur = start + line_index;
  const char* const target = start + index;
  
  int result = 0;
  if (row == 0) result += row0_offset;
  while (target > cur) {
    utf8::next(cur, end);
    result += 1;
  }
  return result;
}
