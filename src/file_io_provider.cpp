#include "core/util_path.hpp"
#include "file_io_provider.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>

bool FileIOProvider::is_handled(const std::string& /* abs_path */) {
  return true;
}

bool FileIOProvider::parent_dir(const std::string& abs_path, std::string& output) {
  if (abs_path == "") return false;

  QDir q_dir(QString::fromStdString(abs_path));
  if (q_dir.isRoot()) {
    output = "";
    return true;
  }
  q_dir.cdUp();
  output = q_dir.absolutePath().toStdString();
  return true;
}

std::vector<DirEntry> FileIOProvider::list_dir(const std::string& abs_path, const std::string& filter) {
  std::vector<DirEntry> output;

  // Root element: on Windows have drive list here, on Linux and Mac just have root.
  if (abs_path == "") {
    #ifdef CMAKE_WINDOWS
    QList<QFileInfo> list = QDir::drives();
    for (int i = 0; i < list.size(); i++) {
      const QFileInfo& fi = list[i];
      std::string file_name = fi.absolutePath().toStdString();
      if (file_name == "." || file_name == "..") continue;
      output.push_back({file_name, DirEntryType::DIR});
    }
    #else
    output.push_back({"/", DirEntryType::DIR});
    #endif
    return output;
  }

  const QDir q_dir(QString::fromStdString(abs_path));
  if (!q_dir.exists() || !q_dir.isReadable()) {
    throw IOProviderError("Cannot read directory " + abs_path);
  }

  const QString q_filter = QString::fromStdString(filter);
  const bool to_filter = !filter.empty();

  const QList<QFileInfo> list = q_dir.entryInfoList();
  for (int i = 0; i < list.size(); i++) {
    const QFileInfo& fi = list[i];
    std::string file_name = fi.fileName().toStdString();
    if (file_name == "." || file_name == "..") continue;
    if (to_filter && QDir::match(q_filter, fi.fileName())) continue;

    DirEntry entry;
    entry.name = file_name;
    entry.type = DirEntryType::FILE;
    if (fi.isDir()) entry.type  = DirEntryType::DIR;
    if (fi.isSymLink()) entry.type = DirEntryType::LINK;
    output.push_back(entry);
  }
  return output;
}

unsigned long FileIOProvider::get_file_size(const std::string& abs_path) {
  QFileInfo fi(QString::fromStdString(abs_path));
  if (!fi.exists()) throw IOProviderError("File '" + abs_path + "' doesn't exist.");
  return (unsigned long) fi.size();
}

void FileIOProvider::touch(const std::string& abs_path, DirEntryType::Type type) {
  if (type == DirEntryType::FILE) {
    QFile file(QString::fromStdString(abs_path));
    if (!file.open(QIODevice::Append)) {
      throw IOProviderError("Can't create new file '" + abs_path + "': " + file.errorString().toStdString());
    }
    file.close();
  } else {
    std::string parent_path = UtilPath::parent_components(abs_path);
    std::string new_name = UtilPath::last_component(abs_path);
    QDir dir(QString::fromStdString(parent_path));
    if (!dir.mkdir(QString::fromStdString(new_name))) {
      std::string text = "Failed to create directory '";
      text += new_name;
      text += "'.";
      throw IOProviderError(text);
    }
  }
}

void FileIOProvider::rename(const std::string& abs_path_old, const std::string& abs_path_new) {
  QFile file(QString::fromStdString(abs_path_old));
  if (!file.rename(QString::fromStdString(abs_path_new))) {
    throw IOProviderError("Can't rename file '" + abs_path_old + "' to '" + abs_path_new + "'.");
  }
}

void FileIOProvider::remove(const std::string& abs_path) {
  QFileInfo fi(QString::fromStdString(abs_path));
  if (fi.isDir()) {
    QDir dir(QString::fromStdString(abs_path));
    if (!dir.removeRecursively()) {
      throw IOProviderError("Failed to remove dir '" + abs_path + "'.");
    }
  } else {
    QFile file(QString::fromStdString(abs_path));
    if (!file.remove()) {
      throw IOProviderError("Failed to remove file '" + abs_path + "'.");
    }
  }
}

std::vector<char> FileIOProvider::read_file(const std::string& abs_path) {
  std::vector<char> output;
  QFile file(QString::fromStdString(abs_path));
  if (!file.open(QIODevice::ReadOnly)) {
    throw IOProviderError("Can't open file '" + abs_path + "' for reading: "
      + file.errorString().toStdString());
  }
  // Allocate memory for the file.
  output.resize(file.size());

  int64_t num_read = file.read(output.data(), output.size());
  if (num_read != (int64_t) output.size()) {
    throw IOProviderError("I/O error while trying to read '" + abs_path + "': " + file.errorString().toStdString());
  }
  return output;
}

void FileIOProvider::write_file_safe(const std::string& abs_path, const char* data, unsigned int size) {
  // First save the backup
  std::string backup_path = abs_path + ".synbkp";
  const QString q_abs_path = QString::fromStdString(abs_path);
  const QString q_backup_path = q_abs_path + ".synbkp";
  QFileDevice::Permissions permissions;
  bool preserved_permissions = false;
  bool saved_backup = false;
  if (QFile::exists(q_abs_path)) {
    permissions = QFile::permissions(q_abs_path);
    preserved_permissions = true;
    saved_backup = true;
    if (!QFile::rename(q_abs_path, q_backup_path)) {
      throw IOProviderError("Can't save backup '" + backup_path + "'. Aborting save.");
    }
  }

  // Now save the file
  QFile file(q_abs_path);
  if (!file.open(QIODevice::WriteOnly)) {
    if (saved_backup) {
      if (!QFile::rename(q_backup_path, q_abs_path)) {
        throw IOProviderError("Can't save '" + abs_path + "'.  Wasn't able to restore backup.  It is located at '" + backup_path + "'. Error while opening was: " + file.errorString().toStdString());
      }
      throw IOProviderError("Can't save '" + abs_path + "'.  Backup restored.  Error while opening was: " + file.errorString().toStdString());
    } else {
      throw IOProviderError("Can't save '" + abs_path + "': " + file.errorString().toStdString());
    }
  }

  int64_t num_written = file.write(data, (int64_t) size);
  if (num_written != (int64_t) size) {
    if (saved_backup) {
      throw IOProviderError("I/O error while trying to write '" + abs_path + "'.  Backup is at '" + backup_path + "'. Error: " + file.errorString().toStdString());
    } else {
      throw IOProviderError("I/O error while trying to write '" + abs_path + "': " + file.errorString().toStdString());
    }
  }

  file.close();
  if (preserved_permissions) QFile::setPermissions(q_abs_path, permissions);
  if (saved_backup) QFile::remove(q_backup_path);
}