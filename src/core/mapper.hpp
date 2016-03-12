#ifndef CORE_MAPPER_HPP
#define CORE_MAPPER_HPP

#include <cstdint>
#include <string>
#include <vector>

/** A mapper object is instantiated on an std::string.  It provides a way for mapping between
index in a string, to (row, col) pair. 

NOTE: Stores a reference to the string. */

class Mapper {
private:
  std::vector<uint32_t> indices;
  uint32_t row0_offset;
  const std::string& str;

public:
  Mapper(const std::string& str, int row0_offset = 0);
  
  int map_row(int index);
  int map_col(int row, int index);
  inline void map(int index, int* row, int* col) {
    *row = map_row(index);
    *col = map_col(*row, index);
  }
};

#endif
