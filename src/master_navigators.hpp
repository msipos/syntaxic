#ifndef SYNTAXIC_MASTER_NAVIGATORS_HPP
#define SYNTAXIC_MASTER_NAVIGATORS_HPP

#include <string>
#include <vector>

struct NavigationDestination {
  int confidence;
  std::string path;
  int row, col;
};

class MasterNavigators {
public:
  MasterNavigators();
  ~MasterNavigators();
  void jump(const std::string& line, int col);
  std::vector<NavigationDestination> get_destinations(const std::string& line, int col, std::string& parsed_word);
  void process_destination(std::vector<NavigationDestination>& output, const std::string& before_word, const std::string& word, const std::string& after_word);
};

extern MasterNavigators* master_navigators;

#endif