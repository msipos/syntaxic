#include "document.hpp"
#include "master.hpp"
#include "my_process.hpp"
#include "tool.hpp"

SynTool::SynTool(const std::string& c, bool w, Document* d, const std::string& wd) : command(c),
    wants_temp_buffer(w), document(d), process(nullptr), escape_counter(0), cwd(wd) {}

SynTool::~SynTool() {}

void SynTool::start() {
  process = std::unique_ptr<MyProcess>(new MyProcess(this));

  // TODO: Mac?
  QStringList arguments;

  #ifdef CMAKE_WINDOWS
  QString program = "CMD.EXE";
  arguments << "/C";
  #else
  QString program = "bash";
  arguments << "-c";
  #endif
  arguments << QString::fromStdString(command);

  if (document) {
    std::string text = "Invoking '";
    text += command;
    text += "'...\n";
    document->handle_console(text);
//    document->handle_virtual_state(1);
    document->set_owning_tool(this);
  }
  process->setWorkingDirectory(QString::fromStdString(cwd));
  process->start(program, arguments);
}

void SynTool::cb_started() {
  /* Ignore for now. */
}

void SynTool::cb_output(const std::string& text, int /* type */) {
  // TODO: Do something with type.
  if (document) {
    document->handle_console(text);
  }
}

void SynTool::cb_escape() {
  escape_counter++;
  if (escape_counter == 1) {
    process->closeWriteChannel();
    process->terminate();
  } else {
    process->kill();
  }
}

void SynTool::cb_input(const std::string& text) {
  process->write(text.c_str(), text.size());
}

void SynTool::cb_error(int error) {
  std::string text = "Error ";
  text += std::to_string(error);
  text += " occured in running subprocess.";
  master.feedback("Error in subprocess", text);
}

void SynTool::cb_finished(int exit_code) {
  if (document) {
    std::string text = "Process finished with exit code ";
    text += std::to_string(exit_code);
    text += ".\n";
    document->handle_console(text);

//    if (exit_code != 0) {
//      document->handle_virtual_state(2);
//    } else {
//      document->handle_virtual_state(0);
//    }
    document->set_owning_tool(nullptr);
  }
  master.tool_finished(this);
}