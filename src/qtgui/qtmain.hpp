#ifndef SYNTAXIC_QTGUI_QTMAIN_HPP
#define SYNTAXIC_QTGUI_QTMAIN_HPP

#include "core/hooks.hpp"

#include <QApplication>

class QEvent;
class QObject;
class QNetworkReply;

class MyApplication : public QApplication {
  Q_OBJECT

  Hook<> setting_hook;


public:
  inline MyApplication(int& argc, char** argv) : QApplication(argc, argv) {}

  void reload_settings();
  void setup_setting_hook();
  virtual bool notify(QObject * rec, QEvent * ev);
  virtual bool event(QEvent* e);

  std::string get_config_dir();
  std::string get_generic_config_dir();

  /** For the network request. */
  QNetworkReply* network_reply;

public slots:
  void slot_network_request_finished();
};

extern MyApplication* my_application;

#endif