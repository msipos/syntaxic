#include "core/util.hpp"
#include "known_documents.hpp"

void KnownDocuments::add_document(const std::string& abs_path) {
  if (set_paths.count(abs_path) > 0) return;
  set_paths.insert(abs_path);
}

std::vector<KnownDocument> KnownDocuments::get() {
  std::vector<KnownDocument> vec;
  for (const std::string& p: set_paths) {
    vec.push_back({ p, last_component(p) });
  }

  return vec;
}