#ifndef SYNTAXIC_JS_CONSOLE_HPP
#define SYNTAXIC_JS_CONSOLE_HPP

#include "console.hpp"

class JSConsole : public Console {
private:

public:
  JSConsole();

  virtual void console_input(const std::string& contents) override;
};



#endif