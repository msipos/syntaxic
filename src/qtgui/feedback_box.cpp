#include "qtgui/feedback_box.hpp"
#include "master.hpp"

#include <QLabel>
#include <QPainter>
#include <QPaintEvent>
#include <QPalette>
#include <QPropertyAnimation>
#include <QTimer>
#include <QVBoxLayout>

FeedbackBox::FeedbackBox(QWidget* parent) : QWidget(parent), prop_alpha(0) {
  q_palette = new QPalette();
  q_text_color = q_palette->color(QPalette::Active, QPalette::WindowText);
  if (master.pref_manager.get_string("theme.gui_theme") == "Dark") {
    q_bg_color = QColor(48, 47, 47, 0);
  } else {
    q_bg_color = q_palette->color(QPalette::Active, QPalette::Window);
  }
  QVBoxLayout* vlayout = new QVBoxLayout;

  q_title = new QLabel(this);
  QFont font = q_title->font();
  font.setBold(true);
  q_title->setFont(font);
  vlayout->addWidget(q_title, 0, Qt::AlignCenter);

  q_text = new QLabel(this);
  q_text->setWordWrap(true);
  vlayout->addWidget(q_text);

  q_appear_animation = new QPropertyAnimation(this, "alpha");
  q_appear_animation->setDuration(250);
  q_appear_animation->setStartValue(0);
  q_appear_animation->setEndValue(255);

  q_disappear_animation = new QPropertyAnimation(this, "alpha");
  q_disappear_animation->setDuration(250);
  q_disappear_animation->setStartValue(255);
  q_disappear_animation->setEndValue(0);
  connect(q_disappear_animation, &QPropertyAnimation::finished,
      this, &FeedbackBox::slot_disappear_end);

  setLayout(vlayout);
  hide();

  q_timer = new QTimer(this);
  q_timer->setSingleShot(true);
  connect(q_timer, &QTimer::timeout, this, &FeedbackBox::slot_timer_end);
}

int FeedbackBox::alpha() const {
  return prop_alpha;
}

void FeedbackBox::setAlpha(int a) {
  if (a < 0) a = 0;
  if (a > 255) a = 255;
  prop_alpha = a;

  q_text_color.setAlpha(prop_alpha);
  q_palette->setColor(QPalette::WindowText, q_text_color);
  q_title->setPalette(*q_palette);
  q_text->setPalette(*q_palette);

  update();
}

void FeedbackBox::display(const std::string& title, const std::string& text, int msec) {
  q_title->setText(QString::fromStdString(title));
  q_text->setText(QString::fromStdString(text));
  adjustSize();
  q_disappear_animation->stop();
  q_timer->start(msec);

  if (!isVisible()) {
    if (master.pref_manager.get_string("theme.gui_theme") == "Dark") {
      setAlpha(255);
    } else {
      q_appear_animation->start();
    }
  }
  show();
}

void FeedbackBox::paintEvent(QPaintEvent* /* event */) {
  QPainter painter(this);
  q_bg_color.setAlpha(prop_alpha);
  QSize sz = size();
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setBrush(QBrush(q_bg_color));
  painter.drawRoundedRect(0, 0, sz.width(), sz.height(), 5, 5);
}

void FeedbackBox::slot_timer_end() {
  if (master.pref_manager.get_string("theme.gui_theme") == "Dark") {
    hide();
  } else {
    q_disappear_animation->start();
  }
}

void FeedbackBox::slot_disappear_end() {
  hide();
}