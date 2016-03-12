#include "lmgen.hpp"

int main(int argc, char** argv) {
  if (argc != 2) return 1;
  LMKey key = lmgen(argv[1]);
  std::string skey = key_to_string(key);
  printf("%s\n", skey.c_str());
}