#include "qtgui/dock.hpp"

#include <QCursor>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPushButton>

Dock::Dock(QWidget* parent) : QWidget(parent), notifier(nullptr) {
  QHBoxLayout* layout = new QHBoxLayout;

  activity_label = new QLabel("Foo bar", this);
  layout->addWidget(activity_label);

  layout->addStretch();

  button = new QPushButton("...", this);
  connect(button, &QPushButton::clicked, this, &Dock::button_clicked);
  layout->addWidget(button);

  setLayout(layout);
}

void Dock::set_text(const std::string& text) {
  activity_label->setText(QString::fromStdString(text));
}

void Dock::button_clicked() {
  if (!notifier) return;

  std::vector<std::string> choices = notifier->get_dock_actions();

  QMenu menu;
  connect(&menu, &QMenu::triggered, this, &Dock::menu_clicked);
  for (std::string& str: choices) {
    if (str == "-") menu.addSeparator();
    else {
      menu.addAction(QString::fromStdString(str));
    }
  }
  menu.exec(QCursor::pos());
}

void Dock::menu_clicked(QAction* action) {
  if (notifier) notifier->dock_action(action->text().toStdString());
}