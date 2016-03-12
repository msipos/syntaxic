#ifndef SYNTAXIC_QTGUI_PROJECT_DIALOG_HPP
#define SYNTAXIC_QTGUI_PROJECT_DIALOG_HPP

#include "project.hpp"

#include <QDialog>

class QLineEdit;
class QPushButton;

class ProjectDialog : public QDialog {
  Q_OBJECT

  QPushButton* q_browse_button;
  QLineEdit* q_name_edit;
  QLineEdit* q_path_edit;
  QLineEdit* q_filter_edit;

public:
  ProjectDialog(QWidget* parent);

  ProjectSpec project_spec;
  //void showEvent(QShowEvent* event);

  int exec_dialog(bool new_project);

public slots:
  void slot_accept();
  void slot_browse();
};

#endif