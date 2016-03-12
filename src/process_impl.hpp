#ifndef PROCESS_IMPL_HPP
#define PROCESS_IMPL_HPP

#include <string>
#include <vector>
#include <QProcess>

class QEventLoop;
class QTimer;

namespace ProcessOutputType {
  enum Type {
    STD_OUT, STD_ERR
  };
}

class ProcessCallback {
public:
  virtual void process_started() = 0;
  virtual void process_finished(int exit_code) = 0;
  virtual void process_error(int error) = 0;
  virtual void process_output(const std::string& text, ProcessOutputType::Type type) = 0;
  virtual void process_timer() = 0;
};

struct ProgramWithArgs {
  std::string program;
  std::vector<std::string> args;
};

ProgramWithArgs parse_command_line(const std::string& cmd_line);
ProgramWithArgs wrap_with_tty(ProgramWithArgs pwa);

class ProcessImpl : public QProcess {
  Q_OBJECT

private:
  ProcessCallback* callback;
  QEventLoop* event_loop;
  QTimer* timer;
  bool running;

  bool timed_flush;
  std::string line_buffer;
  void add_line_char(char c);

private slots:
  void slot_started();
  void slot_finished(int exit_code);
  void slot_error(QProcess::ProcessError err);
  void slot_ready_out();
  void slot_ready_err();
  void slot_timer();

public:
  ProcessImpl(ProcessCallback* callback, const std::string& program, const std::vector<std::string>& args, const std::string& cwd);

  inline QEventLoop* get_event_loop() { return event_loop; }
  inline bool is_running() { return running; }

  std::string ignore_buffer;
  void add_to_ignore_buffer(const std::string& txt);
};

#endif