#include "master.hpp"
#include "process.hpp"
#include "process_impl.hpp"
#include "core/utf8_util.hpp"
#include "qtgui/main_window.hpp"

#include <QEventLoop>
#include <QMessageBox>
#include <QThread>

Process::Process() {
  prompt = "";
}
Process::~Process() {}

bool Process::start_process(const std::vector<std::string>& command_line, const std::string& cwd, bool run_in_shell) {
  std::string program;
  std::vector<std::string> args;

  if (run_in_shell) {
    // TODO: Mac?
    #ifdef CMAKE_WINDOWS
    program = "CMD.EXE";
    args.push_back("/C");
    #else
    program = "bash";
    args.push_back("-c");
    #endif

    std::string remainder;
    for (const std::string& s: command_line) {
      remainder += s;
      remainder += " ";
    }
    args.push_back(remainder);
  } else {
    program = command_line[0];
    for (unsigned int i = 1; i < command_line.size(); i++) {
      args.push_back(command_line[i]);
    }
  }

  pimpl = std::unique_ptr<ProcessImpl>(new ProcessImpl(this, program, args, cwd));
  return true;
}

bool Process::start_shell(const std::string& cwd) {
  process_name = "Shell";
  #ifdef CMAKE_WINDOWS
  ProgramWithArgs pwa = parse_command_line("CMD.EXE");
  #else
  ProgramWithArgs pwa = parse_command_line("bash -i");
  #endif

  pwa = wrap_with_tty(pwa);
  pimpl = std::unique_ptr<ProcessImpl>(new ProcessImpl(this, pwa.program, pwa.args, cwd));
  return true;
}

void Process::process_started() {
  console_output("Process started.\n");
}

void Process::process_finished(int exit_code) {
  console_output("Process finished with exit code " + std::to_string(exit_code) + ".\n");
  if (pimpl->get_event_loop()->isRunning()) pimpl->get_event_loop()->exit();
  call_hook(DocEvent::CHANGED_STATE);
}

void Process::process_error(int error) {
  console_output("Process error code " + std::to_string(error) + ".\n");
  if (pimpl->get_event_loop()->isRunning()) pimpl->get_event_loop()->exit();
  call_hook(DocEvent::CHANGED_STATE);
}

void Process::process_output(const std::string& text, ProcessOutputType::Type /* type */) {
  console_output_ansi(text);
}

void Process::process_timer() {

}

void Process::console_output_ansi(const std::string& contents) {
  std::vector<uint32_t> vec = utf8_string_to_vector(contents);
  std::string tmp;
  uint8_t markup = 0;

  for (unsigned int i = 0; i < vec.size(); i++) {
    uint32_t c = vec[i];

    #ifdef CMAKE_WINDOWS
    if (c == '\r') continue;
    #endif

    if (c == 27) {
      i++;
      if (i >= vec.size()) break;
      c = vec[i];
      if (c == '[') {
        // CSI

        int multiparam = 0;
        int param1 = 0, param2 = 0, param3 = 0;

        i++;
        while (i < vec.size()) {
          c = vec[i];
          if (c == ';') {
            multiparam++;
          } else if (isdigit(c)) {
            if (multiparam == 0) {
              param1 = param1*10 + (c - '0');
            } else if (multiparam == 1) {
              param2 = param2*10 + (c - '0');
            } else if (multiparam == 2) {
              param3 = param3*10 + (c - '0');
            }
          } else if (isalpha(c)) {
            if (!tmp.empty()) {
              console_output(tmp, markup);
              tmp = "";
            }
            if (param1 == 0) markup = 0;
            else {
              if (param1 >= 30 && param1 <= 37) {
                markup = (uint8_t) (param1 - 30);
              }
              if (param2 >= 30 && param2 <= 37) {
                markup = (uint8_t) (param2 - 30);
              }
              if (param3 >= 30 && param3 <= 37) {
                markup = (uint8_t) (param3 - 30);
              }
            }

            break;
          }
          i++;
        }
        continue;

      } else if (c == ']') {
        // OSC

        // Ignore characters until BELL
        i++;
        while (i < vec.size()) {
          c = vec[i];
          // This is bell:
          if (c == 7) break;
          i++;
        }
        continue;
      } else {
        // printf("%d\n", c);
      }
    } else {
      utf8_append(tmp, c);
    }
  }

  if (!tmp.empty()) {
    console_output(tmp, markup);
  }
}

void Process::console_input(const std::string& contents) {
  #ifndef CMAKE_WINDOWS
  console_output(contents + "\n");
  #endif
  pimpl->write(contents.c_str(), contents.size());
  pimpl->write("\n", 1);
}

int Process::get_display_style() const {
  return DocFlag::SHELL_THEME;
}

bool Process::handle_about_to_close() {
  // Double close.
  if (pimpl->get_event_loop()->isRunning()) return false;

  if (pimpl->state() == QProcess::NotRunning) return true;
  else {
    int rv = QMessageBox::warning(dynamic_cast<MainWindow*>(master.get_main_window()), "Process running", "Process in the console is still running, are you sure you wish to terminate it?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (rv == QMessageBox::Yes) {
      pimpl->terminate();
      QThread::msleep(20);
      pimpl->kill();
      QThread::msleep(20);
      pimpl->get_event_loop()->exec();
      return true;
    } else {
      return false;
    }
  }
}

std::string Process::get_short_title() const {
  std::string title = process_name + " ";
  if (pimpl->state() == QProcess::NotRunning) {
    title += " (Done)";
  } else {
    title += " (Running...)";
  }
  return title;
}