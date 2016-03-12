#include "core/utf8_util.hpp"
#include "my_process.hpp"
#include "tool.hpp"

MyProcess::MyProcess(SynTool* t) : tool(t) {
  connect(this, &QProcess::started, this, &MyProcess::slot_started);
  connect(this, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
      this, &MyProcess::slot_finished);
  connect(this, &QProcess::readyReadStandardOutput, this, &MyProcess::slot_ready_out);
  connect(this, &QProcess::readyReadStandardError, this, &MyProcess::slot_ready_err);
  connect(this, static_cast<void (QProcess::*)(QProcess::ProcessError)>(&QProcess::error),
      this, &MyProcess::slot_error);
}

void MyProcess::slot_started() {
  tool->cb_started();
}

void MyProcess::slot_finished(int exit_code) {
  tool->cb_finished(exit_code);
}

void MyProcess::slot_error(QProcess::ProcessError err) {
  tool->cb_error(err);
}

void MyProcess::slot_ready_out() {
  QByteArray out = readAllStandardOutput();
  std::string text = utf8_convert_best(out.data(), out.size());
  tool->cb_output(text, 0);
}

void MyProcess::slot_ready_err() {
  QByteArray out = readAllStandardError();
  std::string text = utf8_convert_best(out.data(), out.size());
  tool->cb_output(text, 1);
}