#include "core/util.hpp"
#include "core/utf8_util.hpp"
#include "process.hpp"
#include "process_impl.hpp"

#include <QByteArray>
#include <QEventLoop>
#include <QTimer>

ProgramWithArgs wrap_with_tty(ProgramWithArgs pwa) {
#ifdef CMAKE_WINDOWS
  return pwa;
#else
  pwa.args.insert(pwa.args.begin(), pwa.program);
  pwa.program = under_root("syntaxic_local_wrapper");
  return pwa;
#endif
}

ProgramWithArgs parse_command_line(const std::string& cmd_line) {
  ProgramWithArgs pwa;

  {
    bool added_program = false;
    std::vector<std::string> splitted1;
    utf8_string_split(splitted1, cmd_line, '"');
    for (unsigned int i = 0; i < splitted1.size(); i++) {
      if (i % 2 == 1) {
        // Between quotes
        if (added_program) pwa.args.push_back(splitted1[i]);
        else {
          pwa.program = splitted1[i];
          added_program = true;
        }
      } else {
        std::string str = utf8_strip(splitted1[i]);
        if (str == "") continue;
        {
          std::vector<std::string> splitted2;
          utf8_string_split_whitespace(splitted2, str);
          for (unsigned int j = 0; j < splitted2.size(); j++) {
            if (added_program) pwa.args.push_back(splitted2[j]);
            else {
              pwa.program = splitted2[j];
              added_program = true;
            }
          }
        }
      }
    }
  }
  return pwa;
}

ProcessImpl::ProcessImpl(ProcessCallback* cb, const std::string& program, const std::vector<std::string>& args, const std::string& cwd) : callback(cb), timed_flush(false) {
  event_loop = new QEventLoop(this);
  timer = new QTimer(this);
  connect(timer, &QTimer::timeout, this, &ProcessImpl::slot_timer);
  timer->start(1000);

  connect(this, &QProcess::started, this, &ProcessImpl::slot_started);
  connect(this, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
      this, &ProcessImpl::slot_finished);
  connect(this, &QProcess::readyReadStandardOutput, this, &ProcessImpl::slot_ready_out);
  connect(this, &QProcess::readyReadStandardError, this, &ProcessImpl::slot_ready_err);
  connect(this, static_cast<void (QProcess::*)(QProcess::ProcessError)>(&QProcess::error),
      this, &ProcessImpl::slot_error);

  QStringList arguments;
  for (const std::string& s : args) {
    arguments << QString::fromStdString(s);
  }
  if (!cwd.empty()) setWorkingDirectory(QString::fromStdString(cwd));
  start(QString::fromStdString(program), arguments);
  running = true;
}

void ProcessImpl::add_line_char(char c) {
  if (ignore_buffer.size() > 0) {
    if (c == '\r' && ignore_buffer[0] == '\n') return;
    if (ignore_buffer[0] == c) {
      ignore_buffer.erase(0, 1);
      return;
    }
  }
  if (c == '\n' && line_buffer.size() > 0 && line_buffer.back() == '\r') line_buffer.pop_back();
  line_buffer.push_back(c);
  if (c == '\n') {
    callback->process_output(line_buffer, ProcessOutputType::STD_OUT);
    line_buffer = "";
    timed_flush = false;
  }
}

void ProcessImpl::slot_timer() {
  callback->process_timer();
  if (!timed_flush) timed_flush = true;
  if (timed_flush) {
    if (!line_buffer.empty()) {
      callback->process_output(line_buffer, ProcessOutputType::STD_OUT);
      line_buffer = "";
      timed_flush = false;
    }
  }
}

void ProcessImpl::slot_started() {
  callback->process_started();
}

void ProcessImpl::slot_finished(int exit_code) {
  callback->process_finished(exit_code);
  running = false;
}

void ProcessImpl::slot_error(QProcess::ProcessError err) {
  callback->process_error(err);
  running = false;
}

void ProcessImpl::slot_ready_out() {
  QByteArray out = readAllStandardOutput();
  std::string text(out.data(), out.size());
  for (unsigned int i = 0; i < text.size(); i++ ){
    add_line_char(text[i]);
  }
}

void ProcessImpl::slot_ready_err() {
  QByteArray out = readAllStandardError();
  std::string text(out.data(), out.size());
  for (unsigned int i = 0; i < text.size(); i++ ){
    add_line_char(text[i]);
  }
}

void ProcessImpl::add_to_ignore_buffer(const std::string& txt) {
  ignore_buffer += txt;
}