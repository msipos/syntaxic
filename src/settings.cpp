#include "settings.hpp"

#include <QFontDatabase>
#include <QSettings>

Settings::Settings() : cache_loaded(false) {
}

std::string Settings::get_lm_email() {
  QSettings qs;
  return qs.value("lm_email", "").toString().toStdString();
}

void Settings::set_lm_email(const std::string& str) {
  QSettings qs;
  qs.setValue("lm_email", QString::fromStdString(str));
}

std::string Settings::get_lm_key() {
  QSettings qs;
  return qs.value("lm_key", "").toString().toStdString();
}

void Settings::set_lm_key(const std::string& str) {
  QSettings qs;
  qs.setValue("lm_key", QString::fromStdString(str));
}

void Settings::get_invokes(std::vector<std::string>& out) {
  if (!cache_loaded) {
    QSettings qs;
    QVariant variant = qs.value("past_invokes");
    QStringList l = variant.toStringList();
    for (QString& v : l) {
      std::string c = v.toStdString();
      invokes.push_back(c);
    }
    cache_loaded = true;
  }

  out = invokes;
}

void Settings::add_invoke(const std::string& cmd) {
  if (!invokes.empty()) {
    if (invokes[0] == cmd) return;
  }
  invokes.insert(invokes.begin(), cmd);
  for (int i = invokes.size() - 1; i > 1; i--) {
    if (invokes[i] == cmd) invokes.erase(invokes.begin() + i);
  }

  if (invokes.size() > 20) invokes.erase(invokes.begin() + 20, invokes.end());
}

std::string Settings::get_current_path() {
  QSettings qs;
  return qs.value("current_path", "").toString().toStdString();
}

void Settings::set_current_path(const std::string& current_path) {
  QSettings qs;
  qs.setValue("current_path", QString::fromStdString(current_path));
}

std::vector<STreeSave> Settings::get_sidebar_state() {
  QSettings qs;

  std::vector<STreeSave> fpss;

  QList<QVariant> types = qs.value("file_provider_types", QVariant(QList<QVariant>())).toList();
  QStringList paths = qs.value("file_provider_paths", QVariant(QStringList())).toStringList();

  int min_size = std::min(types.size(), paths.size());
  for (int i = 0; i < min_size; i++) {
    STreeSaveType type = (STreeSaveType) types[i].toInt();
    std::string path = paths[i].toStdString();
    fpss.push_back({type, path});
  }
  return fpss;
}

void Settings::set_sidebar_state(const std::vector<STreeSave>& state) {
  QSettings qs;

  QList<QVariant> types;
  QStringList paths;

  for (const STreeSave& fps: state) {
    types.push_back(QVariant(fps.type));
    paths.push_back(QString::fromStdString(fps.path));
  }
  qs.setValue("file_provider_types", types);
  qs.setValue("file_provider_paths", paths);
}

void Settings::save_settings() {
  if (!cache_loaded) return;

  QSettings qs;
  QStringList l;
  for (std::string& s: invokes) {
    l.append(QString::fromStdString(s));
  }
  qs.setValue("past_invokes", l);
}

std::vector<SSHConnectionInfo> Settings::get_ssh_connections() {
  QSettings qs;
  std::vector<SSHConnectionInfo> vec;
  int size = qs.beginReadArray("ssh_connections");
  for (int i = 0; i < size; i++) {
    qs.setArrayIndex(i);
    SSHConnectionInfo sci;
    sci.name = qs.value("name").toString().toStdString();
    sci.cmd_line = qs.value("cmd_line").toString().toStdString();
    sci.login_events = qs.value("login_events").toString().toStdString();
    vec.push_back(sci);
  }
  qs.endArray();
  return vec;
}

void Settings::save_ssh_connections(std::vector<SSHConnectionInfo>& connections) {
  QSettings qs;
  qs.beginWriteArray("ssh_connections", connections.size());
  for (unsigned int i = 0; i < connections.size(); i++) {
    qs.setArrayIndex(i);
    SSHConnectionInfo& sci = connections[i];
    qs.setValue("name", QString::fromStdString(sci.name));
    qs.setValue("cmd_line", QString::fromStdString(sci.cmd_line));
    qs.setValue("login_events", QString::fromStdString(sci.login_events));
  }
  qs.endArray();
}

bool Settings::get_first_time() {
  QSettings qs;
  return qs.value("first_time", QVariant(true)).toBool();
}

void Settings::set_first_time() {
  QSettings qs;
  return qs.setValue("first_time", false);
}
