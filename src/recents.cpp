#include "recents.hpp"

#include <QSettings>

Recents::Recents(std::string k, int ms) : key(k), max_size(ms) {
  QSettings qs;
  qs.beginGroup("recents");

  if (!qs.contains(QString::fromStdString(key))) return;

  QStringList sl = qs.value(QString::fromStdString(key)).toStringList();
  for (QString& qstr: sl) {
    recents.push_back(qstr.toStdString());
  }
  qs.endGroup();
}

void Recents::add(const std::string& str) {
  if (!recents.empty()) {
    if (recents[0] == str) return;
  }

  // Try to find str in recents.
  for (unsigned int i = 0; i < recents.size(); i++) {
    if (recents[i] == str) {
      recents.erase(recents.begin() + i);
      break;
    }
  }

  // Insert str at the top
  recents.insert(recents.begin(), str);

  // Limit to max size
  while (int(recents.size()) > max_size) {
    recents.pop_back();
  }

  // Save recent to settings
  {
    QSettings qs;
    qs.beginGroup("recents");
    QStringList sl;
    for (std::string& str: recents) {
      sl.push_back(QString::fromStdString(str));
    }
    qs.setValue(QString::fromStdString(key), sl);
    qs.endGroup();
  }

  // Call hook to notify everyone
  recents_changed_hook.call();
}