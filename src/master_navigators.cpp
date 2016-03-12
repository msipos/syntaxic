#include "master_navigators.hpp"

#include "core/util_path.hpp"
#include "core/utf8_util.hpp"
#include "core/word_def.hpp"
#include "master.hpp"

#include <cctype>

MasterNavigators::MasterNavigators() {
  master_navigators = this;
}

MasterNavigators::~MasterNavigators() {
  master_navigators = nullptr;
}

void MasterNavigators::jump(const std::string& line, int col) {
  std::string parsed_word;
  std::vector<NavigationDestination> destinations = get_destinations(line, col, parsed_word);

  if (destinations.empty()) {
    if (parsed_word.empty()) {
      master.feedback("No where to jump", "Click on a valid word.");
    } else {
      master.feedback("No where to jump", std::string("Could not parse '") + parsed_word + "' as valid destination.");
    }
    return;
  }

  int row = destinations[0].row;
  col = destinations[0].col;
  if (row >= 0) row--;
  if (col >= 0) col--;
  master.open_document(destinations[0].path.c_str(), nullptr, row, col);
}

std::vector<NavigationDestination> MasterNavigators::get_destinations(const std::string& line, int col, std::string& parsed_word) {
  std::vector<NavigationDestination> rv;

  std::vector<uint32_t> vec = utf8_string_to_vector(line);
  if (line.size() == 0) return rv;

  WordDef wd; wd.set_allow_dot(); wd.set_allow_slash();
  std::vector<WordLoc> words = split_words(line, wd);

  for (WordLoc& wl : words) {
    if (wl.start_utf8 <= col && wl.start_utf8 + wl.size_utf8 >= col) {
      std::string matched_word = wl.word;
      std::string before_word = line.substr(0, wl.start);
      std::string after_word = line.substr(wl.start + wl.size);
      process_destination(rv, before_word, matched_word, after_word);
      parsed_word = matched_word;
      break;
    }
  }

  return rv;
}

static void get_numbers(const std::string& str, int& row, int& col) {
  std::vector<uint32_t> vec = utf8_string_to_vector(str);

  int word = 0;
  bool in_word = false;
  unsigned int max_size = str.size();
  if (max_size > 8) max_size = 8;
  for (unsigned int i = 0; i < max_size; i++) {
    if (isdigit(vec[i])) {
      if (!in_word) {
        word++;
        in_word = true;
        if (word == 1) row = 0;
        if (word == 2) col = 0;
      }
      if (word == 1) {
        row = row*10 + vec[i] - '0';
      } else if (word == 2) {
        col = col*10 + vec[i] - '0';
      }

    } else {
      in_word = false;
    }
  }
}

void MasterNavigators::process_destination(std::vector<NavigationDestination>& output, const std::string& /* before_word */, const std::string& word, const std::string& after_word) {
  if (word.empty()) return;

  int row = -1, col = -1;
  get_numbers(after_word, row, col);

  // First, is it an existing file?
  if (UtilPath::is_existing_file(word)) {
    output.push_back({3, word, row, col});
  }

  // Second, is the filename a known document?
  std::vector<KnownDocument> known_docs;
  master.get_known_documents(known_docs);
  std::string filename = UtilPath::last_component(word);
  for (KnownDocument& kd : known_docs) {
    if (filename == kd.file_name) {
      output.push_back({2, kd.abs_path, row, col});
    }
  }

  // Third, is the basename a known document.
  std::string basename = UtilPath::get_basename(filename);
  for (KnownDocument& kd : known_docs) {
    if (basename == UtilPath::get_basename(kd.file_name)) {
      output.push_back({1, kd.abs_path, row, col});
    }
  }
}

MasterNavigators* master_navigators;
