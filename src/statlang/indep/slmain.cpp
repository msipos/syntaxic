#include "core/util.hpp"
#include "core/util_path.hpp"
#include "statlang/statlang.hpp"
#include "master_io_provider.hpp"

#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <QCoreApplication>
#include <vector>

int main(int argc, char** argv) {
  QCoreApplication qca(argc, argv);
  MasterIOProvider miop;

  if (argc > 1) {
    StatLang stat_lang;

    {
      // Load statlang meta.
      std::vector<char> file_contents;
      read_file(file_contents, under_root("syntaxic_meta.json"));
      stat_lang.init(file_contents.data(), file_contents.size());
    }

    std::vector<std::string> paths = UtilPath::walk(argv[1]);
    std::vector<std::unique_ptr<TextFile>> files;

    for (auto& path: paths) {
      fprintf(stderr, "processing %s...\n", path.c_str());

      try {
        std::unique_ptr<TextFile> tf(new TextFile(master_io_provider));
        tf->change_path(path);
        tf->load();
        int sl_id = stat_lang.add_document(tf.get());
        stat_lang.infer_document_type(sl_id, tf->get_file_name());
        stat_lang.process_document(sl_id, nullptr);
        files.push_back(std::move(tf));
      } catch (std::exception& e) {
        fprintf(stderr, "error: %s\n", e.what());
      }
    }

    stat_lang.run_analysis();
  }
}