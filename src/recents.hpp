#ifndef SYNTAXIC_RECENTS_HPP
#define SYNTAXIC_RECENTS_HPP

#include "core/hooks.hpp"

#include <string>
#include <vector>

class Recents {
private:
  std::string key;
  int max_size;
  std::vector<std::string> recents;

public:
  Recents(std::string k, int max_size);

  inline const std::vector<std::string>& get_recents() const { return recents; }

  HookSource<> recents_changed_hook;

  void add(const std::string& str);
};

#endif