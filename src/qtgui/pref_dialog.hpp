#ifndef SYNTAXIC_QTGUI_PREFERENCES_HPP
#define SYNTAXIC_QTGUI_PREFERENCES_HPP

#include <QDialog>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QShowEvent;
class QSpinBox;
class QStackedWidget;
class QTreeWidget;
class QTreeWidgetItem;
class QVBoxLayout;

class PreferencesDialog : public QDialog {
private:
  std::string current_var_name;

public:
  PreferencesDialog(QWidget* parent);

  void showEvent(QShowEvent* event);

  QVBoxLayout* vlayout;

  QTreeWidget* q_main_tree;
  QStackedWidget* stacked_widget;

  QLabel* q_var_name_label;
  QLabel* q_long_description_label;

  // Bool:
  QCheckBox* q_check_box;

  // Int:
  QLabel* q_spin_box_label;
  QSpinBox* q_spin_box;

  // Font:
  QLabel* q_font_label;
  QPushButton* q_font_button;

  // Choice:
  QLabel* q_combo_label;
  QComboBox* q_combo_box;

  // Key:
  QLabel* q_key_label;
  QPushButton* q_key_button;

  // String:
  QLineEdit* q_line_edit;

public slots:
  // Dialog:
  void slot_apply();
  void slot_accept();
  void slot_reject();

  // Changing selection in TreeWidget
  void slot_item_clicked(QTreeWidgetItem* item);

  // Boolean preference
  void slot_bool_switched(int state);

  // Integer preference
  void slot_int_changed(int new_value);

  // Font preference
  void slot_font_button();

  // Choice preference
  void slot_combo_changed(const QString& choice);

  // Key preference
  void slot_key_button();

  // String preference
  void slot_line_edit_changed(const QString& newstr);
};

#endif
