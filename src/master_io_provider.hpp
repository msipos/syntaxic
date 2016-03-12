#ifndef SYNTAXIC_MASTER_IO_PROVIDER_HPP
#define SYNTAXIC_MASTER_IO_PROVIDER_HPP

#include "io_provider.hpp"

#include <memory>

class MasterIOProvider : public IOProvider {
private:
  std::vector<std::unique_ptr<IOProvider>> providers;
  IOProvider* get_handling_provider(const std::string& abs_path);

public:
  MasterIOProvider();
  virtual ~MasterIOProvider();

  virtual bool is_handled(const std::string& abs_path);
  virtual bool parent_dir(const std::string& abs_path, std::string& output);
  virtual std::vector<DirEntry> list_dir(const std::string& abs_path, const std::string& filter);
  virtual unsigned long get_file_size(const std::string& abs_path);
  virtual void touch(const std::string& abs_path, DirEntryType::Type type);
  virtual void rename(const std::string& abs_path_old, const std::string& abs_path_new);
  virtual void remove(const std::string& abs_path);
  virtual std::vector<char> read_file(const std::string& abs_path);
  virtual void write_file_safe(const std::string& abs_path, const char* data, unsigned int size);

  // Other

  void add_ssh(const std::string& name, const std::string& cmd_line, const std::string& actions);
  void remove_io_provider(IOProvider* iop);
  void clear_io_providers();
};

extern MasterIOProvider* master_io_provider;

#endif