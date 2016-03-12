#include "core/util.hpp"
#include "core/util_path.hpp"
#include "core/utf8_util.hpp"
#include "file_browser.hpp"
#include "master.hpp"
#include "master_io_provider.hpp"
#include "process.hpp"
#include "process_impl.hpp"
#include "ssh_io_provider.hpp"
#include "qtgui/dock.hpp"
#include "qtgui/main_window.hpp"

#include <string>
#include <QDir>
#include <QEventLoop>
#include <QMessageBox>
#include <QThread>
#include <QTimer>

namespace LoginEventType {
  enum Type {
    WRITE, WRITE_NO_ECHO, WAIT, MATCH, MATCH_CASE
  };
}

struct LoginEvent {
  LoginEventType::Type type;
  std::string text;
  int num;
};

namespace SSHState {
  enum Type {
    IDLE, LOOPING_UNTIL_MATCH, LOOPING_UNTIL_MATCH_CASE, LOOPING_TIME, CONSUMING_UNTIL_MATCH_CASE
  };
}

class SSHImpl : public ProcessCallback {
private:
  SSHIOProvider* io_provider;

public:

  std::unique_ptr<ProcessImpl> process_impl;
  SSHState::Type state;
  std::string waiting_string;
  std::string consumed;
  std::vector<LoginEvent> login_events;

  SSHImpl(SSHIOProvider* iop);
  double last_ping;

  // Implement ProcessCallback:
  virtual void process_started() override;
  virtual void process_finished(int exit_code) override;
  virtual void process_error(int error) override;
  virtual void process_output(const std::string& text, ProcessOutputType::Type type) override;
  virtual void process_timer() override;

  // Loop control:

  int wait_until_match(const std::string& text, bool case_sensitive);
  int wait_time(int time);
  int consume_until_match(const std::string& text, bool case_sensitive);

  /** Invoke SSHImpl */
  void start(const std::string& cmd_line, const std::string& actions);
};

SSHImpl::SSHImpl(SSHIOProvider* iop) : io_provider(iop), state(SSHState::IDLE) {
}

void SSHImpl::start(const std::string& cmd_line, const std::string& actions) {
  ProgramWithArgs pwa = wrap_with_tty(parse_command_line(cmd_line));
  last_ping = get_timestamp();

  /** Create login events. */
  {
    std::vector<std::string> splitted;
    utf8_string_split(splitted, actions, '\n');
    for (std::string& line : splitted) {
      utf8_rtrim(line, '\r');
      std::vector<std::string> splitted2;
      utf8_string_split_whitespace(splitted2, line, 2);
      if (splitted2.size() == 0) continue;
      if (splitted2.size() != 2) {
        throw std::runtime_error("Invalid line in login actions: " + line);
      }
      LoginEvent login_event;
      if (splitted2[0] == "WRITE") {
        login_event.type = LoginEventType::WRITE;
        login_event.text = splitted2[1];
      } else if (splitted2[0] == "WRITE_NO_ECHO") {
        login_event.type = LoginEventType::WRITE_NO_ECHO;
        login_event.text = splitted2[1];
      } else if (splitted2[0] == "WAIT") {
        login_event.type = LoginEventType::WAIT;
        login_event.num = std::stoi(splitted2[1]);
      } else if (splitted2[0] == "MATCH_CASE") {
        login_event.type = LoginEventType::MATCH_CASE;
        login_event.text = splitted2[1];
      } else if (splitted2[0] == "MATCH") {
        login_event.type = LoginEventType::MATCH;
        login_event.text = splitted2[1];
      } else {
        throw std::runtime_error("Invalid line in login actions: " + line);
      }
      login_events.push_back(login_event);
    }
  }

  io_provider->log_message(pwa.program);
  for (std::string& s: pwa.args) {
    io_provider->log_message(" ");
    io_provider->log_message(s);
  }
  io_provider->log_message("\n");
  process_impl.reset(new ProcessImpl(this, pwa.program, pwa.args, ""));

  for (LoginEvent& login_event : login_events) {
    switch (login_event.type) {
      case LoginEventType::WRITE:
        io_provider->write_to_process(login_event.text);
        io_provider->write_to_process("\n");
        break;
      case LoginEventType::WRITE_NO_ECHO:
        io_provider->write_to_process(login_event.text, false);
        io_provider->write_to_process("\n", false);
        break;
      case LoginEventType::WAIT:
        if (wait_time(login_event.num)) return;
        break;
      case LoginEventType::MATCH_CASE:
        if (wait_until_match(login_event.text, true)) return;
        break;
      case LoginEventType::MATCH:
        if (wait_until_match(login_event.text, false)) return;
        break;
    }
    if (!process_impl->is_running()) return;
  }

  // At this point, we've successfully connected. Open a file browser.
  {
    io_provider->finished_login = true;
    io_provider->check_loop();
    io_provider->update_status_text();
    std::unique_ptr<STree> fp(new FileBrowser(io_provider->prefix + "/"));
    master.add_file_provider(std::move(fp), false);
  }
}

void SSHImpl::process_started() {
  io_provider->log_message("Process started\n");
}

void SSHImpl::process_finished(int exit_code) {
  io_provider->log_message("Process finished with exit code ");
  io_provider->log_message(std::to_string(exit_code));
  io_provider->log_message("\n");
  switch(state) {
    case SSHState::IDLE:
      // Ignore.
      break;
    case SSHState::LOOPING_TIME:
    case SSHState::LOOPING_UNTIL_MATCH:
    case SSHState::LOOPING_UNTIL_MATCH_CASE:
    case SSHState::CONSUMING_UNTIL_MATCH_CASE:
      process_impl->get_event_loop()->exit(1);
      break;
  }

  io_provider->set_status_text("Disconnected.");
}

void SSHImpl::process_error(int error) {
  io_provider->log_message(std::string("Process error: ") + std::to_string(error) + "\n");

  switch(state) {
    case SSHState::IDLE:
      // Ignore.
      break;
    case SSHState::LOOPING_TIME:
    case SSHState::LOOPING_UNTIL_MATCH:
    case SSHState::LOOPING_UNTIL_MATCH_CASE:
    case SSHState::CONSUMING_UNTIL_MATCH_CASE:
      process_impl->get_event_loop()->exit(1);
      break;
  }

  io_provider->set_status_text("Disconnected (process error).");
}

void SSHImpl::process_output(const std::string& text, ProcessOutputType::Type /* type */) {
  io_provider->log_message(text);

  switch(state) {
    case SSHState::IDLE:
    case SSHState::LOOPING_TIME:
      // Ignore.
      break;
    case SSHState::LOOPING_UNTIL_MATCH:
      {
        std::string text2 = utf8_string_lower(text);
        if (text2.find(waiting_string) != std::string::npos) {
          state = SSHState::IDLE;
          process_impl->get_event_loop()->exit(0);
        }
      }
      break;
    case SSHState::LOOPING_UNTIL_MATCH_CASE:
      if (text.find(waiting_string) != std::string::npos) {
        state = SSHState::IDLE;
        process_impl->get_event_loop()->exit(0);
      }
      break;
    case SSHState::CONSUMING_UNTIL_MATCH_CASE:
      {
        unsigned long int loc = text.find(waiting_string);
        if (loc == std::string::npos) {
          consumed += text;
        } else {
          consumed += text.substr(0, loc);
          state = SSHState::IDLE;
          process_impl->get_event_loop()->exit(0);
        }
      }
      break;
  }
}

void SSHImpl::process_timer() {
  if (!process_impl->is_running()) return;
  if (process_impl->get_event_loop()->isRunning()) return;

  double ts = get_timestamp();
  if (ts - last_ping > 60) {
    io_provider->write_to_process("echo SYNTAXIC_PING\n");
    last_ping = ts;
    wait_until_match("SYNTAXIC_PING", true);
  }
}

int SSHImpl::wait_until_match(const std::string& text, bool case_sensitive) {
  if (!process_impl->is_running()) return 1;

  if (case_sensitive) state = SSHState::LOOPING_UNTIL_MATCH_CASE;
  else state = SSHState::LOOPING_UNTIL_MATCH;
  waiting_string = text;
  io_provider->update_status_text();
  int rv = process_impl->get_event_loop()->exec();
  io_provider->update_status_text();
  return rv;
}

int SSHImpl::wait_time(int time) {
  if (!process_impl->is_running()) return 1;

  state = SSHState::LOOPING_TIME;
  QTimer::singleShot(time, [this]() {
    if (state == SSHState::LOOPING_TIME) {
      process_impl->get_event_loop()->exit(0);
      state = SSHState::IDLE;
    }
  } );
  io_provider->update_status_text();
  int rv = process_impl->get_event_loop()->exec();
  io_provider->update_status_text();
  return rv;
}

int SSHImpl::consume_until_match(const std::string& text, bool case_sensitive) {
  if (!process_impl->is_running()) return 1;

  if (!case_sensitive) return 1;
  state = SSHState::CONSUMING_UNTIL_MATCH_CASE;
  waiting_string = text;
  consumed = "";
  io_provider->update_status_text();
  int rv = process_impl->get_event_loop()->exec();
  io_provider->update_status_text();
  return rv;
}

SSHIOProvider::SSHIOProvider(const std::string& n, const std::string& cmd_line, const std::string& actions) : name(n), ssh_dock(nullptr), finished_login(false) {
  prefix = "ssh://" + name;

  ssh_dock = (dynamic_cast<MainWindow*>(master.get_main_window()))->add_new_dock();
  ssh_dock->set_notifier(this);
  set_status_text("Connecting...");

  pimpl.reset(new SSHImpl(this));
  pimpl->start(cmd_line, actions);
}

SSHIOProvider::~SSHIOProvider() {
  if (ssh_dock != nullptr) {
    (dynamic_cast<MainWindow*>(master.get_main_window()))->remove_dock(ssh_dock);
    ssh_dock = nullptr;
  }
  if (pimpl->process_impl->is_running()) {
    pimpl->process_impl->terminate();
    QThread::msleep(200);
    pimpl->process_impl->kill();
    QThread::msleep(200);
    pimpl->process_impl->waitForFinished(1000);
  }
}

void SSHIOProvider::check_loop() {
  if (pimpl->process_impl->get_event_loop()->isRunning()) {
    throw IOProviderError("Already processing.");
  }
}

void SSHIOProvider::update_status_text() {
  if (pimpl->process_impl->is_running()) {
    if (pimpl->state == SSHState::IDLE) {
      if (finished_login) set_status_text("Idle.");
      else set_status_text("Login stuck...");
    } else {
      if (finished_login) set_status_text("Working...");
      else set_status_text("Logging in...");
    }
  } else {
    set_status_text("Disconnected.");
  }
}

void SSHIOProvider::set_status_text(const std::string& txt) {
  if (ssh_dock) ssh_dock->set_text(prefix + ": " + txt);
}

bool SSHIOProvider::is_handled(const std::string& abs_path) {
  if (!pimpl->process_impl->is_running()) return false;
  if (is_prefix(prefix, abs_path)) {
    return true;
  }
  return false;
}

void SSHIOProvider::log_message(const std::string& text) {
  log += text;
  if (log.size() > 5000) {
    log = log.substr(log.size() - 1000);
  }
}

void SSHIOProvider::write_to_process(const std::string& text, bool expect_echo) {
  log_message(text);
  // plink.exe prints out every input when using TTY. For now TTY is disabled on Win and expect_echo is always false..
  if (expect_echo) pimpl->process_impl->add_to_ignore_buffer(text);
  pimpl->process_impl->write(text.c_str(), text.size());
}

static std::string check_consumed(const std::string& command, const std::string& consumed) {
  if (consumed.size() < 3) throw IOProviderError("Failed command '" + command + "': no output");
  char c3 = consumed[consumed.size() - 1];
  char c2 = consumed[consumed.size() - 2];
  char c1 = consumed[consumed.size() - 3];
  if (c3 != '_' || c2 != 'K' || c1 != 'O') {
    throw IOProviderError("Failed command '" + command + "':\n\n " + consumed.substr(0, consumed.size() - 3));
  }
  return consumed.substr(0, consumed.size() - 3);
}

static bool check_consumed_is_ok(const std::string& command, const std::string& consumed) {
  if (consumed.size() < 3) throw IOProviderError("Failed command '" + command + "': no output");
  char c3 = consumed[consumed.size() - 1];
  char c2 = consumed[consumed.size() - 2];
  char c1 = consumed[consumed.size() - 3];
  if (c1 == 'O' && c2 == 'K' && c3 == '_') return true;
  if (c1 == 'N' && c2 == 'O' && c3 == '_') return false;
  throw IOProviderError("Failed command '" + command + "':\n\n " + consumed.substr(0, consumed.size() - 3));
}

bool SSHIOProvider::parent_dir(const std::string& abs_path, std::string& output) {
  std::string path = abs_path.substr(prefix.size());
  if (path == "/") {
    output = "";
    return true;
  }
  if (path.empty()) return false;
  output = prefix + UtilPath::parent_components(path);
  return true;
}

std::vector<DirEntry> SSHIOProvider::list_dir(const std::string& abs_path, const std::string& filter) {
  check_loop();

  const QString q_filter = QString::fromStdString(filter);
  const bool to_filter = !filter.empty();

  std::vector<DirEntry> vec;

  if (abs_path == "") {
    vec.push_back(DirEntry( { prefix + "/", DirEntryType::DIR } ));
    return vec;
  }
  std::string path = abs_path.substr(prefix.size());
  std::string cmd = "ls -F1 '" + path + "' && echo OK_SYNTAXIC_TERMINATOR || echo NO_SYNTAXIC_TERMINATOR\n";
  write_to_process(cmd);
  pimpl->consume_until_match("SYNTAXIC_TERMINATOR", true);

  // Parse output of ls.
  {
    std::vector<std::string> splitted;
    std::string consumed = check_consumed(cmd, pimpl->consumed);
    utf8_string_split(splitted, consumed, '\n');
    for (unsigned int i = 0; i < splitted.size(); i++) {
      DirEntryType::Type type = DirEntryType::FILE;
      std::string& name = splitted[i];
      if (name.empty()) continue;
      char last_char = name[name.size()-1];
      if (last_char == '/') {
        type = DirEntryType::DIR;
        name.erase(name.end() - 1);
      } else if (last_char == '@') {
        type = DirEntryType::LINK;
        name.erase(name.end() - 1);
      } else if (last_char == '*' || last_char == '=' || last_char == '>' || last_char == '|') {
        name.erase(name.end() - 1);
      }
      if (to_filter && QDir::match(q_filter, QString::fromStdString(name))) continue;
      vec.push_back(DirEntry( {name, type} ));
    }
  }
  return vec;
}

unsigned long SSHIOProvider::get_file_size(const std::string& abs_path) {
  check_loop();

  std::string path = abs_path.substr(prefix.size());
  std::string cmd = "wc -c '" + path + "' && echo OK_SYNTAXIC_TERMINATOR || echo NO_SYNTAXIC_TERMINATOR\n";

  write_to_process(cmd);
  pimpl->consume_until_match("SYNTAXIC_TERMINATOR", true);

  // Parse output of wc.
  {
    std::vector<std::string> splitted;
    std::string consumed = check_consumed(cmd, pimpl->consumed);
    utf8_string_split_whitespace(splitted, consumed);
    if (splitted.size() == 0) throw IOProviderError("Could not get size of " + abs_path);
    try {
      return std::stoi(splitted[0]);
    } catch (...) {
      throw IOProviderError("Invalid output of wc -c");
    }
  }
}

void SSHIOProvider::touch(const std::string& abs_path, DirEntryType::Type type) {
  check_loop();

  std::string path = abs_path.substr(prefix.size());

  std::string cmd;
  if (type == DirEntryType::FILE) {
    cmd = "touch '" + path + "' && echo OK_SYNTAXIC_TERMINATOR || echo NO_SYNTAXIC_TERMINATOR\n";
  } else {
    cmd = "mkdir '" + path + "' && echo OK_SYNTAXIC_TERMINATOR || echo NO_SYNTAXIC_TERMINATOR\n";
  }
  write_to_process(cmd);
  pimpl->consume_until_match("SYNTAXIC_TERMINATOR", true);
  check_consumed(cmd, pimpl->consumed);
}

void SSHIOProvider::rename(const std::string& abs_path_old, const std::string& abs_path_new) {
  check_loop();
  std::string path_old = abs_path_old.substr(prefix.size());
  std::string path_new = abs_path_new.substr(prefix.size());

  std::string cmd = "mv '" + path_old + "' '" + path_new + "' && echo OK_SYNTAXIC_TERMINATOR || echo NO_SYNTAXIC_TERMINATOR\n";
  write_to_process(cmd);
  pimpl->consume_until_match("SYNTAXIC_TERMINATOR", true);
  check_consumed(cmd, pimpl->consumed);
}

void SSHIOProvider::remove(const std::string& abs_path) {
  check_loop();
  std::string path = abs_path.substr(prefix.size());

  std::string cmd = "rm -r '" + path + "' && echo OK_SYNTAXIC_TERMINATOR || echo NO_SYNTAXIC_TERMINATOR\n";
  write_to_process(cmd);
  pimpl->consume_until_match("SYNTAXIC_TERMINATOR", true);
  check_consumed(cmd, pimpl->consumed);
}

std::vector<char> SSHIOProvider::read_file(const std::string& abs_path) {
  check_loop();

  std::string path = abs_path.substr(prefix.size());
  std::string cmd = "cat '" + path + "' && echo OK_SYNTAXIC_TERMINATOR || echo NO_SYNTAXIC_TERMINATOR\n";
  write_to_process(cmd);
  pimpl->consume_until_match("SYNTAXIC_TERMINATOR", true);
  std::string consumed = check_consumed(cmd, pimpl->consumed);
  std::vector<char> vec(consumed.begin(), consumed.end());
  return vec;
}

void SSHIOProvider::write_file_safe(const std::string& abs_path, const char* data, unsigned int size) {
  check_loop();

  std::string path = abs_path.substr(prefix.size());

  // First check if file exists already.
  bool saved_backup = false;
  std::string cmd = "[ -f '" + path + "' ] && echo OK_SYNTAXIC_TERMINATOR || echo NO_SYNTAXIC_TERMINATOR\n";

  write_to_process(cmd);
  pimpl->consume_until_match("SYNTAXIC_TERMINATOR", true);
  bool exists = check_consumed_is_ok(cmd, pimpl->consumed);
  if (exists) {
    cmd = "mv '" + path + "' '" + path + ".synbkp' && echo OK_SYNTAXIC_TERMINATOR || echo NO_SYNTAXIC_TERMINATOR\n";
    write_to_process(cmd);
    pimpl->consume_until_match("SYNTAXIC_TERMINATOR", true);
    bool success = check_consumed_is_ok(cmd, pimpl->consumed);
    if (!success) {
      throw IOProviderError("Could not save backup file '" + abs_path + "':\n\n" + pimpl->consumed);
    }
    saved_backup = true;
  }

  cmd = "( cat <<SYNTAXIC_EOF | head -c -1 > '" + path + "' ) && echo OK_SYNTAXIC_TERMINATOR || echo NO_SYNTAXIC_TERMINATOR\n";

  write_to_process(cmd);
  pimpl->process_impl->write(data, size);
  write_to_process("\nSYNTAXIC_EOF\n");
  pimpl->consume_until_match("SYNTAXIC_TERMINATOR", true);
  if (!check_consumed_is_ok(cmd, pimpl->consumed)) {
    throw IOProviderError("Could not save file '" + abs_path + "':\n\n" + pimpl->consumed);
  }
  if (saved_backup) {
    cmd = "rm '" + path + ".synbkp' && echo OK_SYNTAXIC_TERMINATOR || echo NO_SYNTAXIC_TERMINATOR\n";
    write_to_process(cmd);
    pimpl->consume_until_match("SYNTAXIC_TERMINATOR", true);
    if (!check_consumed_is_ok(cmd, pimpl->consumed)) {
      throw IOProviderError("Could not delete backup file '" + abs_path + ".synbkp':\n\n" + pimpl->consumed);
    }
  }
}

std::vector<std::string> SSHIOProvider::get_dock_actions() {
  std::vector<std::string> vec;
  vec.push_back("View log");
  if (!pimpl->process_impl->is_running()) {
    vec.push_back("Remove");
  } else {
    vec.push_back("Disconnect");
  }
  return vec;
}

void SSHIOProvider::dock_action(const std::string& action) {
  if (action == "View log") {
    master.open_temp_read_only_document("SSH Log", log);
  } else if (action == "Disconnect") {
    pimpl->process_impl->terminate();
    QThread::msleep(200);
    pimpl->process_impl->kill();
    QThread::msleep(200);
    pimpl->process_impl->waitForFinished(1000);
  }

  if (action == "Disconnect" || action == "Remove") {
    master_io_provider->remove_io_provider(this);
  }
}