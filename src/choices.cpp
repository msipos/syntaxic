#include "choices.hpp"

#include <algorithm>

using std::min;
using std::max;

int16_t longest_common_substring(const std::string& str1, const std::string& str2, int* curr, int* prev)
{
  if (str1.empty() || str2.empty()) return 0;
  if (str1.size() >= MAX_PATH_SIZE || str2.size() >= MAX_PATH_SIZE) return 0;

  int *swap = nullptr;
  int maxSubstr = 0;

  for(unsigned int i = 0; i<str1.size(); ++i) {
    for(unsigned int j = 0; j<str2.size(); ++j) {
      if(str1[i] != str2[j]) {
        curr[j] = 0;
      } else {
        if(i == 0 || j == 0) {
          curr[j] = 1;
        } else {
          curr[j] = 1 + prev[j-1];
        }
        maxSubstr = max(maxSubstr, curr[j]);
      }
    }
    swap=curr;
    curr=prev;
    prev=swap;
  }
  return maxSubstr;
}

int16_t levenshtein_distance(const std::string& s, const std::string& t, int16_t* buffer) {
  int n = s.size(), m = t.size();
  if (n >= MAX_PATH_SIZE || m >= MAX_PATH_SIZE) return 1000;
  int16_t* d = buffer;
  m++;
  n++;
  for (int i = 0; i < n; i++) d[i] = i;
  for (int i = 0; i < m; i++) d[i * n] = i;

  for (int i = 1; i < n; i++) {
    for (int j = 1; j < m; j++) {
      // Insertions/deletions cost 1, substitution 3.
      const int a = d[(j - 1) * n + i] + 1;
      const int b = d[j * n + i - 1] + 1;
      const int c = d[(j - 1) * n + i - 1] + ((s[i-1] != t[j-1]) ? 3 : 0);
      d[j * n + i] = (min(a,(min(b,c))));
    }
  }
  return d[n * m - 1];
}

float ChoiceList::calculate_score(const std::string& entry, const std::string& choice) {
  float score = levenshtein_distance(entry, choice, levenshtein_buffer)
              / (static_cast<float> (choice.size()));
  score = score - (score*0.1*longest_common_substring(entry, choice, comm1_buf, comm2_buf));
  return score;
}

#define MAX_NUM_CHOICES 30

void ChoiceList::insert_choice_into_chosen(Choice* ch) {
  if (chosen.size() == 0) {
    chosen.push_back(ch);
    return;
  } else {
    bool found = false;
    for (int i = int(chosen.size()) - 1; i >= 0; i--) {
      if (chosen[i]->score < ch->score) {
        chosen.insert(chosen.begin() + i + 1, ch);
        found = true;
        break;
      }
    }
    if (!found) chosen.insert(chosen.begin(), ch);
  }
  if (chosen.size() > MAX_NUM_CHOICES) {
    chosen.pop_back();
  }
}

void ChoiceList::refilter_choices(const std::string& entry) {
  chosen.clear();
  for (Choice& c : choices) {
    c.score = calculate_score(entry, c.name);
    insert_choice_into_chosen(&c);
  }
}

void ChoiceList::add_choice(const std::string& name, int i) {
  choices.push_back(Choice(name, i));
}

void ChoiceList::order_choices() {
  int i = 1;
  chosen.clear();
  for (Choice& c : choices) {
    chosen.push_back(&c);
    i++;
    if (i > MAX_NUM_CHOICES) return;
  }
}