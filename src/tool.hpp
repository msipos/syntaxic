#ifndef SYNTAXIC_TOOL_HPP
#define SYNTAXIC_TOOL_HPP

#include <memory>
#include <string>

class Document;
class MyProcess;

class SynTool {
private:
  std::string command;
  bool wants_temp_buffer;
  Document* document;
  std::unique_ptr<MyProcess> process;
  int escape_counter;
  std::string cwd;

public:
  SynTool(const std::string& command, bool wants_temp_buffer, Document* doc, const std::string&
      cwd);
  ~SynTool();

  inline bool uses_temp_buffer() { return wants_temp_buffer; }

  void start();

  // Callbacks from MyProcess or Document.
  void cb_started();
  void cb_output(const std::string& text, int type);
  /** User pressed escape. */
  void cb_escape();
  /** Error happened in the process. */
  void cb_error(int error);
  /** User pressed a key or provided input. */
  void cb_input(const std::string& text);
  void cb_finished(int exit_code);
};

#endif