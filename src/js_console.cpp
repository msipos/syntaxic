#include "js_console.hpp"
#include "master_js.hpp"

JSConsole::JSConsole() {
  console_output("This is the prototype Syntaxic JavaScript console.\n\nPresently, the API for the plugin system is being designed.  See https://github.com/msipos/syntaxic for more information.\n\nJoin the API design community by following the Git repo, or email comments and suggestions to support@kpartite.com.\n\nTry out typing in \"Syn.feedback('Hello', 'World');\"\n\n");
}

void JSConsole::console_input(const std::string& contents) {
  console_output(prompt + contents + "\n");
  std::string output = master_js->eval_string(contents);
  console_output(output + "\n", 4);
}

