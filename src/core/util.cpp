#include "core/util.hpp"

#include <chrono>
#include <cctype>
#include <stdexcept>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

double get_current_time() {
  auto tp = std::chrono::high_resolution_clock::now().time_since_epoch();
  return std::chrono::duration_cast<std::chrono::milliseconds>(tp).count();
}

bool is_binary_file(const char* data, int size) {
  int num_non_printable = 0;
  for (int i = 0; i < size; i++) {
    char c = data[i];
    if (c < 32) {
      if (c == '\r') continue;
      if (c == '\n') continue;
      if (c == '\t') continue;
      num_non_printable++;
      continue;
    }
    if (c > 126) {
      num_non_printable++;
    }
  }
  float proportion = ((float) num_non_printable) / ((float) size);
  return proportion > 0.1;
}

//path naive_uncomplete(const path p, const path base) {
//    if (p.has_root_path()){
//        if (p.root_path() != base.root_path()) {
//            return p;
//        } else {
//            return naive_uncomplete(p.relative_path(), base.relative_path());
//        }
//    } else {
//        if (base.has_root_path()) {
//            throw std::runtime_error("cannot uncomplete a path relative path from a rooted base");
//        } else {
//            typedef path::const_iterator path_iterator;
//            path_iterator path_it = p.begin();
//            path_iterator base_it = base.begin();
//            while ( path_it != p.end() && base_it != base.end() ) {
//                if (*path_it != *base_it) break;
//                ++path_it; ++base_it;
//            }
//            path result;
//            for (; base_it != base.end(); ++base_it) {
//                result /= "..";
//            }
//            for (; path_it != p.end(); ++path_it) {
//                result /= *path_it;
//            }
//            return result;
//        }
//    }
//}

//void walk_paths(path root, vector<path>& paths, vector<string>& excluded) {
//  boost::filesystem::recursive_directory_iterator itr(root), end;
//  while (itr != end) {
//    const path& p = itr->path();
//    string filename = p.filename().string();
//    if (boost::filesystem::is_directory(p)) {
//      for (string& excl: excluded) {
//        if (filename == excl) {
//          itr.no_push();
//        }
//      }
//      // Ignore . dirs
//      if (filename.find('.') == 0) {
//        itr.no_push();
//      }
//    } else {
//      // Ignore ~ files
//      if (filename.rfind('~') != filename.size() - 1) {
//        if ( !is_binary_file(p)) paths.push_back(naive_uncomplete(p, root));
//      }
//    }
//    itr++;
//  }
//}

void read_file(std::vector<char>& out, const std::string& p) {
  out.clear();
  QFile qfile(QString::fromStdString(p));

  if (!qfile.open(QIODevice::ReadOnly)) {
    throw std::runtime_error("Failed to open file '" + p + "' for reading.");
  }

  for (;;) {
    char c = 0;
    bool result = qfile.getChar(&c);
    if (!result) {
      throw std::runtime_error("Error while reading file '" + p + "'");
    }
    out.push_back(c);
    if (qfile.atEnd()) break;
  }
}

void write_file(const std::vector<char>& contents, const std::string& p) {
  write_file(contents.data(), contents.size(), p);
}

void write_file(const char* contents, uint32_t size, const std::string& p) {
  QFile qfile(QString::fromStdString(p));

  if (!qfile.open(QIODevice::WriteOnly)) {
    throw std::runtime_error("Failed to open file '" + p + "' for writing.");
  }

  int64_t written = qfile.write(contents, size);
  if (written != int64_t(size)) {
    throw std::runtime_error("Error while writing file '" + p + "'");
  }
  qfile.close();
}


std::string under_root(const std::string& filename) {
  // TODO: Double check this with regards to changing current dir.
  QString app_dir = QCoreApplication::applicationDirPath();
  QDir q_dir(app_dir);
  QString file = q_dir.filePath(QString::fromStdString(filename));
  QFileInfo q_file_info(file);
  return q_file_info.absoluteFilePath().toStdString();
}

std::string under_root2(const std::string& dir, const std::string& filename) {
  // TODO: Double check this with regards to changing current dir.
  QString app_dir = QCoreApplication::applicationDirPath();
  QDir q_dir(app_dir);
  QDir q_dir2(q_dir.filePath(QString::fromStdString(dir)));
  QString file = q_dir2.filePath(QString::fromStdString(filename));
  QFileInfo q_file_info(file);
  return q_file_info.absoluteFilePath().toStdString();
}

std::string elide_right(const std::string& path, int num) {
  if (int(path.size()) < num) return path;
  return "..." + path.substr(path.size() - num);
}

std::string abs_path(const std::string& p) {
  QDir q_dir(QString::fromStdString(p));
  return q_dir.absolutePath().toStdString();
}

std::string last_component(const std::string& p) {
  QDir q_dir(QString::fromStdString(p));
  return q_dir.dirName().toStdString();
}

std::string extract_extension(const std::string& filename) {
  if (filename.empty()) return "";

  QFileInfo q_file_info(QString::fromStdString(filename));
  std::string extension = q_file_info.completeSuffix().toStdString();
  if (extension.empty()) return extension;
  return "." + extension;
}

std::string extract_lowercase_extension(const std::string& filename) {
  if (filename.empty()) return "";

  QFileInfo q_file_info(QString::fromStdString(filename));
  QString q_extension = q_file_info.completeSuffix().toLower();
  std::string extension = q_extension.toStdString();
  if (extension.empty()) return extension;
  return "." + extension;
}

double get_timestamp() {
  auto tp = std::chrono::high_resolution_clock::now();
  double d = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count();
  return d / 1000;
}