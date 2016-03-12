#include "qtgui/main_window.hpp"
#include "qtgui/menu_manager.hpp"

#include <QAction>
#include <QActionGroup>
#include <QMenu>

MenuManager::MenuManager(MainWindow* mw) : main_window(mw) {
}

void MenuManager::add_menu(const std::string& menu, const std::string& sok, bool b_sok, std::function<void()>& callback, int menu_handle) {
  std::size_t loc = menu.rfind('>');
  if (loc == std::string::npos) {
    throw MenuError("Invalid menu name definition.");
  }

  std::string prefix = menu.substr(0, loc);
  std::string name = menu.substr(loc+1);

  QMenu* q_menu;
  QActionGroup* q_action_group;
  if (prefix == "Plugins") {
    q_action_group = main_window->q_menu_plugins_actions;
    q_menu = main_window->q_menu_plugins;
  } else {
    throw MenuError("Invalid menu name definition.");
  }

  MenuEntry menu_entry;
  menu_entry.callback = callback;
  menu_entry.menu_handle = menu_handle;
  menu_entry.b_sok = b_sok;
  menu_entry.sok = sok;

  QAction* action = new QAction(QString::fromStdString(name), q_action_group);
  action->setData(QString::fromStdString(menu));
  if (!sok.empty()) {
    action->setShortcut(QString::fromStdString(sok));
  }
  menu_entry.action = action;
  q_menu->addAction(action);
  menu_map[menu] = menu_entry;
}

void MenuManager::init() {
  connect(main_window->q_menu_plugins_actions, &QActionGroup::triggered, this, &MenuManager::triggered);
}

void MenuManager::triggered(QAction* action) {
  std::string data = action->data().toString().toStdString();

  if (menu_map.count(data) > 0) menu_map[data].callback();
}

void MenuManager::clear_plugin_menu() {
  for (auto& p : menu_map) {
    delete p.second.action;
  }
  menu_map.clear();
}