#include "master.hpp"
#include "qtgui/pref_dialog.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QHideEvent>
#include <QFont>
#include <QFontDialog>
#include <QFormLayout>
#include <QFrame>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QShowEvent>
#include <QSpacerItem>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTreeWidget>
#include <QVBoxLayout>

PreferencesDialog::PreferencesDialog(QWidget* parent) : QDialog(parent) {
  vlayout = new QVBoxLayout;

  {
    q_main_tree = new QTreeWidget(this);
    q_main_tree->setColumnCount(2);
    QStringList header_labels;
    header_labels.push_back("Name");
    header_labels.push_back("Description");
    q_main_tree->setHeaderLabels(header_labels);
    q_main_tree->setMinimumHeight(350);
    q_main_tree->setMinimumWidth(450);
    connect(q_main_tree, &QTreeWidget::currentItemChanged, this, &PreferencesDialog::slot_item_clicked);
  }

  vlayout->addWidget(q_main_tree);

  QFrame* hr = new QFrame(this);
  hr->setFrameShape(QFrame::HLine);
  hr->setFrameShadow(QFrame::Sunken);

  vlayout->addWidget(hr);

  q_var_name_label = new QLabel(this);
  QFont f = q_var_name_label->font();
  f.setBold(true);
  q_var_name_label->setFont(f);
  vlayout->addWidget(q_var_name_label);

  q_long_description_label = new QLabel(this);
  q_long_description_label->setWordWrap(true);
  q_long_description_label->setMinimumHeight(40);
  vlayout->addWidget(q_long_description_label);

  {
    stacked_widget = new QStackedWidget(this);
    {
      // Page for bools
      QWidget* page = new QWidget(this);
      QFormLayout* flayout = new QFormLayout;

      #ifdef CMAKE_MACOSX
      flayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
      #endif

      q_check_box = new QCheckBox(page);
      connect(q_check_box, &QCheckBox::stateChanged, this, &PreferencesDialog::slot_bool_switched);
      flayout->addRow(q_check_box);

      page->setLayout(flayout);
      stacked_widget->addWidget(page);
    }

    {
      // Page for ints
      QWidget* page = new QWidget(this);
      QFormLayout* flayout = new QFormLayout;

      #ifdef CMAKE_MACOSX
      flayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
      #endif

      q_spin_box_label = new QLabel(page);
      q_spin_box = new QSpinBox(page);
      connect(q_spin_box, (void(QSpinBox::*)(int)) (&QSpinBox::valueChanged), this, &PreferencesDialog::slot_int_changed);
      flayout->addRow(q_spin_box_label, q_spin_box);

      page->setLayout(flayout);
      stacked_widget->addWidget(page);
    }

    {
      // Page for fonts
      QWidget* page = new QWidget(this);
      QFormLayout* flayout = new QFormLayout;

      #ifdef CMAKE_MACOSX
      flayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
      #endif

      q_font_label = new QLabel(page);
      q_font_label->setText("Font:");
      q_font_button = new QPushButton(page);
      q_font_button->setText("Select font.");
      connect(q_font_button, &QPushButton::clicked, this, &PreferencesDialog::slot_font_button);

      flayout->addRow(q_font_label, q_font_button);

      page->setLayout(flayout);
      stacked_widget->addWidget(page);
    }

    {
      // Page for combos
      QWidget* page = new QWidget(this);
      QFormLayout* flayout = new QFormLayout;

      // N.B. Looks ugly on OSX otherwise:
      #ifdef CMAKE_MACOSX
      flayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
      #endif

      q_combo_label = new QLabel(page);
      q_combo_box = new QComboBox(page);
      connect(q_combo_box, (void(QComboBox::*)(const QString&)) (&QComboBox::activated), this, &PreferencesDialog::slot_combo_changed);
      flayout->addRow(q_combo_label, q_combo_box);

      page->setLayout(flayout);
      stacked_widget->addWidget(page);
    }

    {
      // Page for keys
      QWidget* page = new QWidget(this);
      QFormLayout* flayout = new QFormLayout;

      // N.B. Looks ugly on OSX otherwise:
      #ifdef CMAKE_MACOSX
      flayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
      #endif

      q_key_label = new QLabel(page);
      q_key_label->setText("Key:");
      q_key_button = new QPushButton(page);
      q_key_button->setText("Enter key.");
      connect(q_key_button, &QPushButton::clicked, this, &PreferencesDialog::slot_key_button);

      flayout->addRow(q_key_label, q_key_button);

      page->setLayout(flayout);
      stacked_widget->addWidget(page);
    }

    {
      // Page for strings
      QWidget* page = new QWidget(this);
      QFormLayout* flayout = new QFormLayout;

      // N.B. Looks ugly on OSX otherwise:
      #ifdef CMAKE_MACOSX
      flayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
      #endif

      q_line_edit = new QLineEdit(page);
      connect(q_line_edit, &QLineEdit::textEdited, this, &PreferencesDialog::slot_line_edit_changed);
      flayout->addRow("Value:", q_line_edit);

      page->setLayout(flayout);
      stacked_widget->addWidget(page);
    }

    {
      // Empty page for when things break.
      QWidget* page = new QWidget(this);
      stacked_widget->addWidget(page);
    }

    stacked_widget->setCurrentIndex(0);

    vlayout->addWidget(stacked_widget);
  }

  QDialogButtonBox* button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply);
  connect(button_box, &QDialogButtonBox::accepted, this, &PreferencesDialog::slot_accept);
  connect(button_box, &QDialogButtonBox::rejected, this, &PreferencesDialog::slot_reject);
  connect(button_box->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &PreferencesDialog::slot_apply);
  vlayout->addWidget(button_box);

  setWindowTitle("Syntaxic Preferences");
  setLayout(vlayout);

  // N.B.: Without this the preferences are not properly centered at beginning. Not sure why.
  updateGeometry();
  adjustSize();
}

void PreferencesDialog::slot_apply() {
  master.pref_manager.start_setting();
  master.pref_manager.end_setting();
}

void PreferencesDialog::slot_accept() {
  slot_apply();
  accept();
}

void PreferencesDialog::slot_reject() {
  master.pref_manager.clear_temp();
  reject();
}

void PreferencesDialog::slot_item_clicked(QTreeWidgetItem* item) {
  if (item == nullptr) return;

  QString q_name = item->text(0);
  q_var_name_label->setText(q_name);
  current_var_name = q_name.toStdString();

  PrefSpec* current_spec = master.pref_manager.find_pref_spec(current_var_name);
  q_long_description_label->setText(QString::fromStdString(current_spec->user_long_text));
  if (current_spec != nullptr) {
    switch (current_spec->type) {
      case PREF_BOOL:
        q_check_box->blockSignals(true);
        q_check_box->setChecked(master.pref_manager.get_bool(current_var_name));
        q_check_box->setText(QString::fromStdString(current_spec->user_text));
        q_check_box->blockSignals(false);
        stacked_widget->setCurrentIndex(0);
        break;
      case PREF_INT:
        q_spin_box_label->setText(QString::fromStdString(current_spec->user_text));
        q_spin_box->blockSignals(true);
        q_spin_box->setRange(current_spec->min_int, current_spec->max_int);
        q_spin_box->setValue(master.pref_manager.get_int(current_var_name));
        q_spin_box->blockSignals(false);
        stacked_widget->setCurrentIndex(1);
        break;
      case PREF_FONT:
        {
          QFont font = master.pref_manager.get_font(current_var_name);
          q_font_button->setText(QString("%1 %2").arg(font.family()).arg(font.pointSize()));
          stacked_widget->setCurrentIndex(2);
        }
        break;
      case PREF_CHOICE:
        stacked_widget->setCurrentIndex(3);
        q_combo_box->blockSignals(true);
        q_combo_box->clear();
        for (const std::string& choice: current_spec->choices) {
          q_combo_box->addItem(QString::fromStdString(choice));
        }
        {
          std::string current_choice = master.pref_manager.get_string(current_var_name);
          q_combo_box->setCurrentText(QString::fromStdString(current_choice));
        }
        q_combo_box->blockSignals(false);
        break;
      case PREF_KEY:
        stacked_widget->setCurrentIndex(4);
        {
          std::string keys = master.pref_manager.get_key(current_var_name);
          if (keys.empty()) q_key_button->setText("<None>");
          else q_key_button->setText(QString::fromStdString(keys));
        }
        break;
      case PREF_STRING:
        stacked_widget->setCurrentIndex(5);
        {
          std::string str = master.pref_manager.get_string(current_var_name);
          q_line_edit->setText(QString::fromStdString(str));
          break;
        }
        break;
      default:
        stacked_widget->setCurrentIndex(6);
        break;
    }
  }
}

void PreferencesDialog::showEvent(QShowEvent* /* event */) {
  q_main_tree->clear();
  const std::vector<PrefSpecCategory>& categories = master.pref_manager.spec_categories;

  QTreeWidgetItem* first_item = nullptr;
  for (const PrefSpecCategory& category : categories) {
    for (const PrefSpec& spec: category.specs) {
      QStringList labels;
      labels.push_back(QString::fromStdString(spec.var_name));
      labels.push_back(QString::fromStdString(spec.user_text));
      QTreeWidgetItem* item = new QTreeWidgetItem((QTreeWidget*) nullptr, labels);
      q_main_tree->addTopLevelItem(item);
      if (first_item == nullptr) { first_item = item; q_main_tree->setCurrentItem(item); }
    }
  }
  q_main_tree->resizeColumnToContents(0);
  slot_item_clicked(first_item);
}

void PreferencesDialog::slot_bool_switched(int state) {
  PrefSpec* current_spec = master.pref_manager.find_pref_spec(current_var_name);
  if (current_spec == nullptr || current_spec->type != PREF_BOOL) return;
  master.pref_manager.set_bool(current_var_name, state == Qt::Checked);
}

void PreferencesDialog::slot_int_changed(int new_value) {
  PrefSpec* current_spec = master.pref_manager.find_pref_spec(current_var_name);
  if (current_spec == nullptr || current_spec->type != PREF_INT) return;
  master.pref_manager.set_int(current_var_name, new_value);
}


void PreferencesDialog::slot_font_button() {
  PrefSpec* current_spec = master.pref_manager.find_pref_spec(current_var_name);
  if (current_spec == nullptr || current_spec->type != PREF_FONT) return;

  bool ok;
  QFont font = QFontDialog::getFont(&ok, master.pref_manager.get_font(current_var_name), this);
  if (ok) {
    master.pref_manager.set_font(current_var_name, font);
    q_font_button->setText(QString("%1 %2").arg(font.family()).arg(font.pointSize()));
  }
}

void PreferencesDialog::slot_combo_changed(const QString& choice) {
  PrefSpec* current_spec = master.pref_manager.find_pref_spec(current_var_name);
  if (current_spec == nullptr || current_spec->type != PREF_CHOICE) return;
  master.pref_manager.set_string(current_var_name, choice.toStdString());
}

void PreferencesDialog::slot_key_button() {
  PrefSpec* current_spec = master.pref_manager.find_pref_spec(current_var_name);
  if (current_spec == nullptr || current_spec->type != PREF_KEY) return;

  for (;;) {
    bool ok;
    QString def;
    if (q_key_button->text() != "<None>") def = q_key_button->text();
    QString text = QInputDialog::getText(this, "Key preference", "Key spec:", QLineEdit::Normal, def, &ok).trimmed();
    if (ok) {
      if (KeyMapper::are_keys_valid(text.toStdString())) {
        master.pref_manager.set_key(current_var_name, text.toStdString());
        if (text.isEmpty()) q_key_button->setText("<None>");
        else q_key_button->setText(text);
        break;
      } else {
        QMessageBox::critical(this, "Invalid key spec", "Key spec '" + text + "' is not valid.");
      }
    } else break;
  }
}

void PreferencesDialog::slot_line_edit_changed(const QString& newstr) {
  PrefSpec* current_spec = master.pref_manager.find_pref_spec(current_var_name);
  if (current_spec == nullptr || current_spec->type != PREF_STRING) return;
  master.pref_manager.set_string(current_var_name, newstr.toStdString());
}