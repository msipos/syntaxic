#include "core/util.hpp"
#include "core/util_json.hpp"
#include "core/util_path.hpp"
#include "master.hpp"
#include "project.hpp"
#include "qtgui/main_window.hpp"

Project::Project(const std::string& project_root, ProjectSpec spec) : FileBrowser(project_root), project_spec(spec) {
  reconfigure_project();
}

void Project::context_do(const STreeNode* fpn, const std::string& context, STreeChangeNotifier* fpcn) {
  if (context == "Configure project") {
    MainWindow* mw = dynamic_cast<MainWindow*>(master.get_main_window());
    mw->configure_project(this);
    return;
  }
  FileBrowser::context_do(fpn, context, fpcn);
}

void Project::reconfigure_project() {
  set_project(project_spec.project_name);
  filter = "*.synproj " + project_spec.filtering_pattern;
}

void Project::save_project() {
  ::save_project(project_spec);
}

STreeSave Project::get_save_info() const {
  return { FILE_PROVIDER_SAVE_PROJECT, project_spec.project_json_path };
}

void save_project(ProjectSpec spec) {
  Json::Value root(Json::objectValue);
  root["name"] = Json::Value(spec.project_name);
  root["filter"] = Json::Value(spec.filtering_pattern);
  std::string contents = root.toStyledString();
  write_file(contents.c_str(), contents.size(), spec.project_json_path);
}

ProjectSpec open_project(const std::string& project_path) {
  std::vector<char> contents;
  read_file(contents, project_path);
  const Json::Value root = parse_json(contents.data(), contents.size());
  ProjectSpec spec;
  spec.project_name = root["name"].asString();
  spec.filtering_pattern = root["filter"].asString();
  // Root path is the directory containing the file.
  spec.root_path = UtilPath::parent_components(UtilPath::to_absolute(project_path));
  spec.project_json_path = project_path;
  return spec;
}