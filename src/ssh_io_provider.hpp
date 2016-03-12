#ifndef SYNTAXIC_SSH_IO_PROVIDER_HPP
#define SYNTAXIC_SSH_IO_PROVIDER_HPP

#include "io_provider.hpp"
#include "qtgui/dock.hpp"

#include <memory>
#include <string>

class SSHImpl;

class SSHIOProvider : public IOProvider, public DockNotifier {
friend class SSHImpl;

private:
  std::unique_ptr<SSHImpl> pimpl;
  std::string name;
  std::string prefix;
  Dock* ssh_dock;
  void update_status_text();
  void set_status_text(const std::string& txt);

  std::string log;
  void log_message(const std::string& text);
  /** Write this text to the process (and to the log). */
  void write_to_process(const std::string& text, bool expect_echo=false);
  bool finished_login;

  void check_loop();

public:
  SSHIOProvider(const std::string& name, const std::string& cmd_line, const std::string& actions);
  virtual ~SSHIOProvider();

  /** Implement IOProvider: */
  virtual bool is_handled(const std::string& abs_path);
  virtual bool parent_dir(const std::string& abs_path, std::string& output);
  virtual std::vector<DirEntry> list_dir(const std::string& abs_path, const std::string& filter);
  virtual unsigned long get_file_size(const std::string& abs_path);
  virtual void touch(const std::string& abs_path, DirEntryType::Type type);
  virtual void rename(const std::string& abs_path_old, const std::string& abs_path_new);
  virtual void remove(const std::string& abs_path);
  virtual std::vector<char> read_file(const std::string& abs_path);
  virtual void write_file_safe(const std::string& abs_path, const char* data, unsigned int size);

  /** Implement DockNotifier. */
  virtual std::vector<std::string> get_dock_actions();
  virtual void dock_action(const std::string& action);
};

#endif