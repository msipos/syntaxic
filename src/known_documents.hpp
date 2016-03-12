#ifndef SYNTAXIC_KNOWN_DOCUMENTS_HPP
#define SYNTAXIC_KNOWN_DOCUMENTS_HPP

#include <string>
#include <unordered_set>
#include <vector>

struct KnownDocument {
  std::string abs_path;
  std::string file_name;
};

class KnownDocuments {
private:
  std::unordered_set<std::string> set_paths;

public:
  void add_document(const std::string& abs_path);
  std::vector<KnownDocument> get();
};

#endif