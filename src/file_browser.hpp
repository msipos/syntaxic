#ifndef SYNTAXIC_FILE_BROWSER_HPP
#define SYNTAXIC_FILE_BROWSER_HPP

#include "stree.hpp"

#include <memory>
#include <string>
#include <vector>

#define FB_NONE       0
#define FB_FILE       1
#define FB_DIR        2
#define FB_PARENT     3
#define FB_WARNING    4
#define FB_ROOT       5
#define FB_PROJECT_ROOT 6
#define FB_LINK       7

class FileBrowser;
class UIWindow;

class FileNode : public STreeNode {
  friend class FileBrowser;
private:
  std::string short_name;
  std::string abs_path;
  std::vector<std::unique_ptr<FileNode>> children;
  bool is_loaded;
  int type;
  bool movable;

public:
  FileNode(STreeNode* p, FileBrowser* fb, int index, const std::string& short_name, const std::string& abs_path, int type);

  virtual STreeNodeType get_type() const;
  virtual std::string text() const;
  virtual int num_children() const;
  virtual bool is_leaf() const;
  virtual const STreeNode* get_child(int child) const;
  virtual STreeNode* get_child(int child);

  int count_and_process_dir(bool process);
  void add_known_documents(KnownDocuments* kd);
  inline void set_movable(bool b) { movable = b; }
};

class FileBrowser : public STree {
protected:
  std::string current_dir_abs;
  std::unique_ptr<FileNode> root_node;

  void refresh_node(FileNode* fn, STreeChangeNotifier* fpcn);
  bool movable;
  std::string filter;
  bool is_project;

  void set_project(const std::string& project_name);

public:
  FileBrowser(const std::string& path);

  inline std::string get_current_dir_abs() { return current_dir_abs; }

  // STree interface:
  virtual STreeNode* get_root_node() override;
  virtual void activate(const STreeNode* fpn, STreeChangeNotifier* fpcn) override;
  virtual void context_request(const STreeNode* fpn, std::vector<std::string>& contexts)
    override;
  virtual void context_do(const STreeNode* fpn, const std::string& context,
      STreeChangeNotifier* fpcn) override;
  virtual void populate_known_documents(KnownDocuments* kd) override;
  virtual STreeSave get_save_info() const override;

  std::string get_filter();
  inline void refresh_root_node(STreeChangeNotifier* fpcn) {
    refresh_node(root_node.get(), fpcn);
  }
  inline bool get_is_project() { return is_project; }
};

#endif