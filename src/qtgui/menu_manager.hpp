#ifndef SYNTAXIC_QTGUI_MENU_MANAGER_HPP
#define SYNTAXIC_QTGUI_MENU_MANAGER_HPP

#include <functional>
#include <stdexcept>
#include <string>
#include <unordered_map>

class MainWindow;
class QAction;

class MenuError : public std::runtime_error {
public: MenuError(const std::string& what) : std::runtime_error(what) {}
};

struct MenuEntry {
  std::function<void()> callback;
  QAction* action;
  // Settings or key.
  std::string sok;
  bool b_sok;
  int menu_handle;
};

class MenuManager : public QObject {
  Q_OBJECT

private:
  MainWindow* main_window;
  std::unordered_map<std::string, MenuEntry> menu_map;

public:
  MenuManager(MainWindow* mw);

  void add_menu(const std::string& menu, const std::string& sok, bool b_sok, std::function<void()>& callback, int menu_handle);
  void clear_plugin_menu();
  void init();

public slots:
  void triggered(QAction*);
};

#endif