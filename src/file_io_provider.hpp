#ifndef SYNTAXIC_FILE_PROVIDER_HPP
#define SYNTAXIC_FILE_PROVIDER_HPP

#include "io_provider.hpp"

class FileIOProvider : public IOProvider {
public:
  virtual bool is_handled(const std::string& abs_path);
  virtual bool parent_dir(const std::string& abs_path, std::string& output);
  virtual std::vector<DirEntry> list_dir(const std::string& abs_path, const std::string& filter);
  virtual unsigned long get_file_size(const std::string& abs_path);
  virtual void touch(const std::string& abs_path, DirEntryType::Type type);
  virtual void rename(const std::string& abs_path_old, const std::string& abs_path_new);
  virtual void remove(const std::string& abs_path);
  virtual std::vector<char> read_file(const std::string& abs_path);
  virtual void write_file_safe(const std::string& abs_path, const char* data, unsigned int size);
};

#endif