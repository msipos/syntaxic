#ifndef SYNTAXIC_PROCESS_HPP
#define SYNTAXIC_PROCESS_HPP

#include "console.hpp"
#include "process_impl.hpp"

#include <memory>
#include <string>
#include <vector>

class ProcessImpl;

class Process : public Console, public ProcessCallback {
private:
  std::unique_ptr<ProcessImpl> pimpl;

  void console_output_ansi(const std::string& contents);
  std::string process_name;

public:
  Process();
  ~Process();

  bool start_process(const std::vector<std::string>& command_line, const std::string& cwd, bool run_in_shell);
  bool start_shell(const std::string& cwd);

  // Implement ProcessCallback

  virtual void process_started() override;
  virtual void process_finished(int exit_code) override;
  virtual void process_error(int error) override;
  virtual void process_output(const std::string& text, ProcessOutputType::Type type) override;
  virtual void process_timer();

  virtual void console_input(const std::string& contents) override;

  // Implement Doc

  virtual int get_display_style() const;
  virtual std::string get_short_title() const;
  virtual bool handle_about_to_close();
};

#endif