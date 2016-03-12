#include "master_io_provider.hpp"
#include "file_io_provider.hpp"
#include "ssh_io_provider.hpp"

MasterIOProvider* master_io_provider = nullptr;

IOProvider* MasterIOProvider::get_handling_provider(const std::string& abs_path) {
  for (auto& provider : providers) {
    if (provider->is_handled(abs_path)) {
      return provider.get();
    }
  }
  return nullptr;
}

MasterIOProvider::MasterIOProvider() {
  providers.push_back(std::unique_ptr<IOProvider>(new FileIOProvider));
  master_io_provider = this;
}

MasterIOProvider::~MasterIOProvider() {
  master_io_provider = nullptr;
}

bool MasterIOProvider::is_handled(const std::string& /* abs_path */) {
  /** Does not matter. */
  return true;
}

bool MasterIOProvider::parent_dir(const std::string& abs_path, std::string& output) {
  if (abs_path == "") return false;

  IOProvider* iop = get_handling_provider(abs_path);
  if (iop) return iop->parent_dir(abs_path, output);
  return false;
}

std::vector<DirEntry> MasterIOProvider::list_dir(const std::string& abs_path, const std::string& filter) {
  if (abs_path == "") {
    std::vector<DirEntry> entries;
    for (auto& provider : providers) {
      std::vector<DirEntry> dir_entries = provider->list_dir("", filter);
      for (DirEntry& de : dir_entries) {
        entries.push_back(de);
      }
    }
    return entries;
  }

  IOProvider* iop = get_handling_provider(abs_path);
  if (iop) return iop->list_dir(abs_path, filter);
  return std::vector<DirEntry>();
}

unsigned long MasterIOProvider::get_file_size(const std::string& abs_path) {
  IOProvider* iop = get_handling_provider(abs_path);
  if (iop) return iop->get_file_size(abs_path);
  return 0;
}

void MasterIOProvider::touch(const std::string& abs_path, DirEntryType::Type type) {
  IOProvider* iop = get_handling_provider(abs_path);
  if (iop) iop->touch(abs_path, type);
}

void MasterIOProvider::rename(const std::string& abs_path_old, const std::string& abs_path_new) {
  IOProvider* iop = get_handling_provider(abs_path_old);
  if (iop) iop->rename(abs_path_old, abs_path_new);
}

void MasterIOProvider::remove(const std::string& abs_path) {
  IOProvider* iop = get_handling_provider(abs_path);
  if (iop) iop->remove(abs_path);
}

std::vector<char> MasterIOProvider::read_file(const std::string& abs_path) {
  IOProvider* iop = get_handling_provider(abs_path);
  if (iop) return iop->read_file(abs_path);
  return std::vector<char>();
}

void MasterIOProvider::write_file_safe(const std::string& abs_path, const char* data, unsigned int size) {
  IOProvider* iop = get_handling_provider(abs_path);
  if (iop) iop->write_file_safe(abs_path, data, size);
}

void MasterIOProvider::add_ssh(const std::string& name, const std::string& cmd_line, const std::string& actions) {
  providers.insert(providers.begin(), std::unique_ptr<IOProvider>(new SSHIOProvider(name, cmd_line, actions)));
}

void MasterIOProvider::remove_io_provider(IOProvider* iop) {
  for (unsigned int i = 0; i < providers.size(); i++) {
    if (providers[i].get() == iop) {
      providers.erase(providers.begin() + i);
      return;
    }
  }
}

void MasterIOProvider::clear_io_providers() {
  providers.clear();
}