#include "lm.hpp"
#include "qtgui/register_dialog.hpp"
#include "settings.hpp"

#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpacerItem>
#include <QUrl>

RegisterDialog::RegisterDialog(QWidget* parent) : QDialog(parent) {
  QFormLayout* flayout = new QFormLayout;
  flayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

  feedback_label = new QLabel("This is an unlicensed copy.", this);
  feedback_label->setAlignment(Qt::AlignCenter);
  flayout->addRow(feedback_label);

  flayout->addItem(new QSpacerItem(20, 20));

  email_edit = new QLineEdit(this);
  flayout->addRow("Email:", email_edit);

  key_edit = new QLineEdit(this);
  flayout->addRow("Key:", key_edit);

  QDialogButtonBox* q_bbox = new QDialogButtonBox(this);

  QPushButton* q_buy_button = q_bbox->addButton("Buy...", QDialogButtonBox::ActionRole);
  connect(q_buy_button, &QPushButton::clicked, this, &RegisterDialog::slot_buy_clicked);
  QPushButton* q_cancel_button = q_bbox->addButton(QDialogButtonBox::Cancel);
  connect(q_cancel_button, &QPushButton::clicked, this, &RegisterDialog::slot_rejected);
  QPushButton* q_ok_button = q_bbox->addButton("Enter License", QDialogButtonBox::AcceptRole);
  connect(q_ok_button, &QPushButton::clicked, this, &RegisterDialog::slot_accepted);
  flayout->addRow(q_bbox);

  setLayout(flayout);
}

void RegisterDialog::showEvent(QShowEvent* event) {
  QDialog::showEvent(event);
  email_edit->setText("");
  email_edit->setFocus();
  key_edit->setText("");

  Settings settings;
  std::string email = settings.get_lm_email();
  if (!email.empty()) {
    email_edit->setText(QString::fromStdString(email));
    std::string text = "This copy is licensed to '";
    text += email;
    text += "'.";
    feedback_label->setText(QString::fromStdString(text));
  } else {
    feedback_label->setText("This is an unlicensed copy.");
  }
}

void RegisterDialog::slot_buy_clicked() {
  QDesktopServices::openUrl(QUrl("https://www.syntaxiceditor.com/"));
}

void RegisterDialog::slot_accepted() {
  std::string email = email_edit->text().trimmed().toStdString();
  std::string key = key_edit->text().trimmed().toStdString();

  if (email.empty()) {
    QMessageBox::critical(this, "Must supply email address", "You must provide an email address."
        "\n\nThe email address must be the one you used to purchase the license key."
        "\n\nPlease contact us at support@kpartite.com if you are having problems.");
    return;
  }
  if (key.empty() || !well_formed(key)) {
    QMessageBox::critical(this, "Invalid license key", "Your license key is invalid."
        "\n\nThe license key must have the format 'xxxx-xxxx-xxxx-xxxx-xxxx'."
        "\n\nPlease contact us at support@kpartite.com if you are having problems.");
    return;
  }

  Settings settings;
  settings.set_lm_email(email);
  settings.set_lm_key(key);
  hide();

  QMessageBox::information(this, "Thank you!", "Thank you for purchasing Syntaxic!");
}

void RegisterDialog::slot_rejected() {
  hide();
}
