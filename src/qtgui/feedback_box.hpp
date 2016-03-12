#ifndef SYNTAXIC_QTGUI_FEEDBACK_BOX_HPP
#define SYNTAXIC_QTGUI_FEEDBACK_BOX_HPP

#include <string>
#include <QColor>
#include <QWidget>

class QLabel;
class QPalette;
class QPropertyAnimation;

class FeedbackBox : public QWidget {
  Q_OBJECT
  Q_PROPERTY(int alpha READ alpha WRITE setAlpha) 
  
private:
  QPalette* q_palette;
  QLabel* q_title;
  QLabel* q_text;
  int prop_alpha;
  QColor q_text_color;
  QColor q_bg_color;
  QTimer* q_timer;
  QPropertyAnimation* q_appear_animation;
  QPropertyAnimation* q_disappear_animation;

public:
  FeedbackBox(QWidget* parent);
  
  void display(const std::string& title, const std::string& text, int msec);

  int alpha() const;
  void setAlpha(int a);
  
  virtual void paintEvent(QPaintEvent*);
  
public slots:
  void slot_timer_end();
  void slot_disappear_end();
};

#endif
