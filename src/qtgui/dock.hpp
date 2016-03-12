#ifndef SYNTAXIC_QTGUI_DOCK_HPP
#define SYNTAXIC_QTGUI_DOCK_HPP

#include <string>
#include <vector>
#include <QWidget>

class QAction;
class QLabel;
class QPushButton;

class DockNotifier {
public:
  virtual ~DockNotifier() {}

  virtual std::vector<std::string> get_dock_actions() = 0;
  virtual void dock_action(const std::string& action) = 0;
};

class Dock : public QWidget {
  Q_OBJECT

private:
  DockNotifier* notifier;
  QLabel* activity_label;
  QPushButton* button;

public:
  Dock(QWidget* parent);

  void set_text(const std::string& text);
  inline void set_notifier(DockNotifier* dn) { notifier = dn; }

public slots:
  void button_clicked();
  void menu_clicked(QAction* action);
};

#endif