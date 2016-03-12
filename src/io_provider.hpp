#ifndef SYNTAXIC_IO_PROVIDER_HPP
#define SYNTAXIC_IO_PROVIDER_HPP

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace DirEntryType {
  enum Type {
    FILE = 0, DIR, LINK
  };
}

class IOProviderError : public std::runtime_error {
public: inline IOProviderError(const std::string& what) : std::runtime_error(what) {}
};

/** Returned by list_dir. */
struct DirEntry {
  std::string name;
  DirEntryType::Type type;
};

/**
 IOProvider is the interface to the file system.
 */

class IOProvider {
public:
  virtual ~IOProvider() {};


  // Query the filesystem


  /** Queries whether this IOProvider handles this file. If not, then next IOProvider in the line will handle it. */
  virtual bool is_handled(const std::string& abs_path) = 0;
  /** Return parent dir as absolute path in our VFS scheme. Return true if parent dir exists, otherwise false. */
  virtual bool parent_dir(const std::string& abs_path, std::string& output) = 0;
  virtual std::vector<DirEntry> list_dir(const std::string& abs_path, const std::string& filter) = 0;
  virtual unsigned long get_file_size(const std::string& abs_path) = 0;


  // Operate on the filesystem


  virtual void touch(const std::string& abs_path, DirEntryType::Type type) = 0;
  virtual void rename(const std::string& abs_path_old, const std::string& abs_path_new) = 0;
  virtual void remove(const std::string& abs_path) = 0;


  // Read and write


  virtual std::vector<char> read_file(const std::string& abs_path) = 0;
  virtual void write_file_safe(const std::string& abs_path, const char* data, unsigned int size) = 0;
};

#endif