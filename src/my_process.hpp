#ifndef SYNTAXIC_MY_PROCESS_HPP
#define SYNTAXIC_MY_PROCESS_HPP

#include <QProcess>

class SynTool;

/** A shim to go around the whole QObject slot and signal thing. */
class MyProcess : public QProcess {
  Q_OBJECT

private:
  SynTool* tool;

public:
  MyProcess(SynTool* t);

public slots:
  void slot_started();
  void slot_finished(int exit_code);
  void slot_error(QProcess::ProcessError err);
  void slot_ready_out();
  void slot_ready_err();
};

#endif