#include "core/util.hpp"
#include "core/utf8_util.hpp"
#include "core/util_path.hpp"
#include "document.hpp"
#include "file_browser.hpp"
#include "master.hpp"
#include "master_io_provider.hpp"
#include "master_js.hpp"
#include "master_navigators.hpp"
#include "project.hpp"
#include "qtgui/dock.hpp"
#include "qtgui/main_window.hpp"
#include "qtgui/qtmain.hpp"

#include <cstdio>
#include <QCoreApplication>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileOpenEvent>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QMessageBox>
#include <QStandardPaths>
#include <QStyleFactory>
#include <QTextStream>

MyApplication* my_application;

bool MyApplication::notify(QObject * rec, QEvent * ev) {
  try {
    return QApplication::notify(rec,ev);
  } catch(...) {
    QMessageBox::warning(0, "An unexpected error occurred", "This is likely a bug.");
  }
  return false;
}

bool MyApplication::event(QEvent* ev) {
  if (ev->type() == QEvent::FileOpen) {
    QFileOpenEvent* fev = dynamic_cast<QFileOpenEvent*>(ev);
    if (fev != nullptr) {
      master.open_document(fev->file().toStdString().c_str(), nullptr);
    }
    return true;
  }
  return QApplication::event(ev);
}

void MyApplication::reload_settings() {
  if (master.pref_manager.get_string("theme.gui_theme") == "Dark") {
    QFile f(":resources/style.qss");
    if (!f.exists()) printf("Unable to set stylesheet, file not found\n");
    else {
      f.open(QFile::ReadOnly | QFile::Text);
      QTextStream ts(&f);
      setStyleSheet(ts.readAll());
    }
  } else {
    setStyleSheet("");
  }
}

void MyApplication::setup_setting_hook() {
  setting_hook = master.pref_manager.change_hook.add(std::bind(&MyApplication::reload_settings, this));
}

std::string MyApplication::get_generic_config_dir() {
  return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation).toStdString();
}

std::string MyApplication::get_config_dir() {
  return UtilPath::join_components(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation).toStdString(), "syntaxic");
}

void MyApplication::slot_network_request_finished() {
  QByteArray arr = network_reply->readAll();
  std::string contents(arr.data(), arr.size());
  contents = utf8_strip(contents);
  if (contents != "current") {
    if (contents[0] == 'h') {
      if (QMessageBox::question(nullptr, "New version available", "A new version of Syntaxic is available.  Do you want to go to the download page?") == QMessageBox::Yes) {
        QDesktopServices::openUrl(QUrl("https://syntaxiceditor.com/download/"));
      }
    }
  }
}

#ifndef NO_QT_MAIN

int main(int argc, char **argv) {
  MyApplication app(argc, argv);
  my_application = &app;
  {
    //printf("prefix path = %s\n", QLibraryInfo::location(QLibraryInfo::PrefixPath).toStdString().c_str());
    //printf("plugins path = %s\n", QLibraryInfo::location(QLibraryInfo::PluginsPath).toStdString().c_str());
  }

  QCoreApplication::setOrganizationName("kpartite");
  QCoreApplication::setOrganizationDomain("kpartite.com");
  QCoreApplication::setApplicationName("Syntaxic");

  // Create app config dir
  {
    std::string generic_dir = app.get_generic_config_dir();
    QDir config_dir(QString::fromStdString(generic_dir));
    if (!config_dir.mkpath("syntaxic")) {
      printf("WARNING: Could not create %s/syntaxic.  Functionality will be broken.\n", generic_dir.c_str());
    }
  }

  // Make a check for updated version
  QNetworkAccessManager manager;
  QNetworkRequest req(QUrl("https://syntaxiceditor.com/version/0.9.3"));
  req.setRawHeader( "User-Agent" , "Syntaxic");
  my_application->network_reply = manager.get(req);
  QObject::connect(my_application->network_reply, &QNetworkReply::finished, my_application, &MyApplication::slot_network_request_finished);

  // Fusion style:
//  app.setStyle(QStyleFactory::create("Fusion"));
//  {
//  QPalette palette;
//  palette.setColor(QPalette::Window, QColor(53,53,53));
//  palette.setColor(QPalette::WindowText, QColor(200, 200, 200));
//  palette.setColor(QPalette::Base, QColor(15,15,15));
//  palette.setColor(QPalette::AlternateBase, QColor(53,53,53));
//  palette.setColor(QPalette::ToolTipBase, Qt::white);
//  palette.setColor(QPalette::ToolTipText, Qt::white);
//  palette.setColor(QPalette::Text, QColor(200, 200, 200));
//  palette.setColor(QPalette::Button, QColor(53,53,53));
//  palette.setColor(QPalette::ButtonText, QColor(200, 200, 200));
//  palette.setColor(QPalette::BrightText, Qt::red);
//
//  palette.setColor(QPalette::Highlight, QColor(0,45,90));
//  palette.setColor(QPalette::HighlightedText, Qt::black);
//  app.setPalette(palette);
//  }

  MasterIOProvider _master_io_provider;
  MasterNavigators _master_navigators;
  MasterJS _master_js;
  try {
    master.init_master();
  } catch (std::exception& e) {
    std::string text = "Error while starting Syntaxic:\n\n";
    text += e.what();
    text += "\n\nPlease reinstall or report a bug to support@kpartite.com.";
    text += "\n\nProgram startup will continue but functionality may be limited.";
    QMessageBox::critical(nullptr, "Failed to initialize program", QString::fromStdString(text));
  }

  app.reload_settings();

  try {
    master.create_main_window();
  } catch (std::exception& e) {
    QMessageBox::critical(nullptr, "Failed to create window", "Could not create a window. Please report this problem to support@kpartite.com.");
    return 1;
  }

  if (argc <= 1) master.new_document(nullptr);
  else {
    for (int i = 1; i < argc; i++) {
      char* arg = argv[i];
      master.open_document(arg, nullptr);
    }
  }

  // Check the saved state of file providers. If there are any projects open, then open them now.
  bool default_file_browser = true;
  {
    std::vector<STreeSave> fpss = master.settings.get_sidebar_state();
    if (fpss.size() > 0) {
      if (fpss.size() > 1 || fpss[0].type != FILE_PROVIDER_SAVE_FILE_BROWSER) {
        for (STreeSave& fps: fpss) {
          switch(fps.type) {
            case FILE_PROVIDER_SAVE_FILE_BROWSER:
            //  {
            //    std::unique_ptr<STree> fp(new FileBrowser(fps.path));
            //    master.add_file_provider(std::move(fp));
            //  }
              break;
            case FILE_PROVIDER_SAVE_PROJECT:
              master.open_project(fps.path);
              default_file_browser = false;
              break;
          }
        }
        //default_file_browser = false;
      }
    }
  }

  // Start a file browser if no other file providers are recorded
  if (default_file_browser) {
    std::string path = "";
    {
      std::vector<std::unique_ptr<Doc>>& documents = master.get_documents();
      if (documents.size() > 0) {
        Document* last_doc = dynamic_cast<Document*>(documents.back().get());
        if (last_doc != nullptr && !last_doc->get_text_file()->is_new()) {
          std::string abs_path = last_doc->get_text_file()->get_absolute_path();
          QFileInfo fi(QString::fromStdString(abs_path));
          path = fi.absolutePath().toStdString();
        }
      }
    }
    if (path.empty()) {
      path = master.settings.get_current_path();
    }
    if (path.empty()) {
    //     * If started from a dir, and this dir:
    //         - does not contain syntaxic
    //         - is a subdir of home
    }
    if (path.empty()) {
      path = QDir::homePath().toStdString();
    }
    std::unique_ptr<STree> fp(new FileBrowser(path));
    master.add_file_provider(std::move(fp));
  }

  // Open welcome page
  if (master.settings.get_first_time()) {
    master.open_welcome();
    master.settings.set_first_time();
  }

  // Load plugins
  try {
    master_js->make_default_syntaxic_js_file();
    master_js->eval_syntaxic_js_file();
  } catch (std::exception& e) {
    QMessageBox::critical(nullptr, "Failed to initialize plugin system", QString::fromStdString(std::string("Check '") + master_js->get_syntaxic_js_file() + "'."));
  }

  int rv = app.exec();
  master_io_provider->clear_io_providers();
  master.delete_windows();
  return rv;
}

#endif