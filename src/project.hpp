#ifndef SYNTAXIC_PROJECT_HPP
#define SYNTAXIC_PROJECT_HPP

#include "file_browser.hpp"

#include <string>

struct ProjectSpec {
  std::string project_json_path;
  std::string project_name;
  std::string root_path;
  std::string filtering_pattern;
};

class Project : public FileBrowser {
public:
  ProjectSpec project_spec;

  Project(const std::string& project_root, ProjectSpec spec);

  virtual void context_do(const STreeNode* fpn, const std::string& context, STreeChangeNotifier* fpcn) override;
  STreeSave get_save_info() const override;
  void reconfigure_project();
  void save_project();
};

void save_project(ProjectSpec spec);
ProjectSpec open_project(const std::string& project_path);

#endif