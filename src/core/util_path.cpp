#include "core/util_path.hpp"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QStandardPaths>
#include <QString>

namespace UtilPath {

static std::string strip_prefix(std::string& path) {
  if (path.find("ssh:") == 0) {
    size_t x = path.find('/');
    if (x == std::string::npos) {
      std::string prefix = path;
      path = "";
      return prefix;
    } else {
      std::string prefix = path.substr(0, x);
      path = path.substr(x);
      return prefix;
    }
  }
  return "";
}

std::string to_absolute(const std::string& path) {
  // SSH already has absolute path.
  if (path.find("ssh:") == 0) return path;

  QDir q_dir(QString::fromStdString(path));
  return q_dir.absolutePath().toStdString();
}

std::string parent_components(const std::string& path) {
  QFileInfo q_fi(QString::fromStdString(path));
  return q_fi.absolutePath().toStdString();
}

std::string last_component(const std::string& path) {
  QDir q_dir(QString::fromStdString(path));
  return q_dir.dirName().toStdString();
}

std::string join_components(const std::string& p1, const std::string& p2) {
  char sep = QDir::separator().toLatin1();
  if (p1.find("ssh:") == 0) sep = '/';

  if (p2.empty()) return p1;

  char last_char = p1[p1.size() - 1];
  char first_char = p2[0];

  bool last = (last_char == '/' || last_char == '\\');
  bool first = (first_char == '/' || first_char == '\\');

  if (first && last) {
    return p1 + p2.substr(1);
  } else if (!first && !last) {
    return p1 + sep + p2;
  }
  return p1 + p2;
}

bool is_existing_file(const std::string& path) {
  return QFileInfo::exists(QString::fromStdString(path));
}

std::string home_path() {
  return QDir::homePath().toStdString();
}

std::string get_extension(const std::string& filename) {
  if (filename.empty()) return "";

  QFileInfo q_file_info(QString::fromStdString(filename));
  std::string extension = q_file_info.completeSuffix().toStdString();
  if (extension.empty()) return extension;
  return "." + extension;
}

std::string get_lowercase_extension(const std::string& filename) {
  if (filename.empty()) return "";

  QFileInfo q_file_info(QString::fromStdString(filename));
  QString q_extension = q_file_info.completeSuffix().toLower();
  std::string extension = q_extension.toStdString();
  if (extension.empty()) return extension;
  return "." + extension;
}

std::string get_basename(const std::string& filename) {
  if (filename.empty()) return "";
  QFileInfo q_file_info(QString::fromStdString(filename));
  return q_file_info.baseName().toStdString();
}

std::vector<std::string> walk(const std::string& dir) {
  std::vector<std::string> results;
  QDirIterator it(QString::fromStdString(dir), QDir::NoDotAndDotDot | QDir::AllEntries, QDirIterator::Subdirectories);
  while (it.hasNext()) {
    results.push_back(it.next().toStdString());
  }
  return results;
}

std::string temp_file() {
  static int temp_file_counter = 0;
  QString temp_dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
  temp_file_counter += 1;
  return join_components(temp_dir.toStdString(), std::string("tmp_syntaxic_") + std::to_string(temp_file_counter));
}

}
