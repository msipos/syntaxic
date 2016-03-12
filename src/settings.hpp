#ifndef SYNTAXIC_SETTINGS_HPP
#define SYNTAXIC_SETTINGS_HPP

#include "stree.hpp"

#include <QFont>
#include <string>
#include <vector>

struct SSHConnectionInfo {
  std::string name;
  std::string cmd_line;
  std::string login_events;
};

class Settings {
private:
  std::vector<std::string> invokes;
  bool cache_loaded;

public:
  Settings();

  // License

  std::string get_lm_email();
  void set_lm_email(const std::string& str);
  std::string get_lm_key();
  void set_lm_key(const std::string& str);

  // Transitory

  void get_invokes(std::vector<std::string>& out);
  void add_invoke(const std::string& cmd);

  // Saved sidebar state
  std::vector<STreeSave> get_sidebar_state();
  void set_sidebar_state(const std::vector<STreeSave>& state);
  std::string get_current_path();
  void set_current_path(const std::string& current_path);

  // SSH Connections
  std::vector<SSHConnectionInfo> get_ssh_connections();
  void save_ssh_connections(std::vector<SSHConnectionInfo>& connections);

  // First time on
  bool get_first_time();
  void set_first_time();

  void save_settings();
};

#endif