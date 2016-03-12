#ifndef SYNTAXIC_CHOICES_HPP
#define SYNTAXIC_CHOICES_HPP

#include <cstdint>
#include <string>
#include <vector>

#define MAX_PATH_SIZE 500

struct Choice {
  std::string name;
  float score;
  int num;

  inline Choice(const std::string& n, int i) : name(n), num(i) {}
};

class ChoiceList {
private:
  int16_t levenshtein_buffer[MAX_PATH_SIZE*MAX_PATH_SIZE];
  int comm1_buf[MAX_PATH_SIZE];
  int comm2_buf[MAX_PATH_SIZE];
  void insert_choice_into_chosen(Choice* ch);

public:
  std::vector<Choice> choices;
  std::vector<Choice*> chosen;
  int currently_chosen;

  float calculate_score(const std::string& entry, const std::string& choice);
  void refilter_choices(const std::string& entry);
  void order_choices();
  void add_choice(const std::string& name, int i);
};

#endif