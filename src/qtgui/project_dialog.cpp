#include "qtgui/project_dialog.hpp"

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpacerItem>

ProjectDialog::ProjectDialog(QWidget* parent) : QDialog(parent) {
  QFormLayout* flayout = new QFormLayout;

  q_name_edit = new QLineEdit(this);
  flayout->addRow("Project name:", q_name_edit);

  flayout->addItem(new QSpacerItem(20, 20));

  {
    QHBoxLayout* hlayout = new QHBoxLayout;

    q_path_edit = new QLineEdit(this);
    hlayout->addWidget(q_path_edit);
    q_browse_button = new QPushButton("Browse...", this);
    connect(q_browse_button, &QAbstractButton::clicked, this, &ProjectDialog::slot_browse);
    hlayout->addWidget(q_browse_button);

    flayout->addRow("Project root path:", hlayout);
  }

  flayout->addItem(new QSpacerItem(20, 20));

  QLabel* note = new QLabel("You can specify multiple filters as globs (using * and ? syntax) to "
      "ignore files and directories from the project.  Separate filters with spaces or semicolons.",
      this);
  note->setWordWrap(true);
  flayout->addRow(note);
  q_filter_edit = new QLineEdit(this);
  q_filter_edit->setMinimumWidth(300);
  flayout->addRow("Filter out:", q_filter_edit);

  QDialogButtonBox* button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(button_box, &QDialogButtonBox::accepted, this, &ProjectDialog::slot_accept);
  connect(button_box, &QDialogButtonBox::rejected, this, &ProjectDialog::reject);
  flayout->addRow(button_box);

  setWindowTitle("Project Preferences");
  setLayout(flayout);
}

int ProjectDialog::exec_dialog(bool new_project) {
  if (new_project) {
    setWindowTitle("New Project Preferences");
    q_path_edit->setText("");
    q_path_edit->setEnabled(true);
    q_name_edit->setText("");
    q_name_edit->setEnabled(true);
    q_filter_edit->setText("");

    q_browse_button->setEnabled(true);
  } else {
    setWindowTitle("Project Preferences");
    q_path_edit->setText(QString::fromStdString(project_spec.root_path));
    q_path_edit->setEnabled(false);
    q_name_edit->setText(QString::fromStdString(project_spec.project_name));
    q_name_edit->setEnabled(false);
    q_filter_edit->setText(QString::fromStdString(project_spec.filtering_pattern));

    q_browse_button->setEnabled(false);
  }
  return exec();
}

void ProjectDialog::slot_accept() {
  QString q_project_name = q_name_edit->text().trimmed();
  QString q_project_path = q_path_edit->text().trimmed();
  QString q_filter_text = q_filter_edit->text().trimmed();

  if (q_project_name.size() == 0) {
    QMessageBox::critical(this, "Invalid project name", "You must specify a project name");
    return;
  }
  if (q_project_path.size() == 0) {
    QMessageBox::critical(this, "Invalid project path", "You must specify a project path");
    return;
  }

  project_spec.project_name = q_project_name.toStdString();
  project_spec.root_path = q_project_path.toStdString();
  project_spec.filtering_pattern = q_filter_text.toStdString();
  accept();
}

void ProjectDialog::slot_browse() {
  QString q_dir = QFileDialog::getExistingDirectory(this, "Choose project directory");
  q_path_edit->setText(q_dir);
}