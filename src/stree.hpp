#ifndef SYNTAXIC_FILE_PROVIDER_HPP
#define SYNTAXIC_FILE_PROVIDER_HPP

#include <memory>
#include <string>
#include <vector>

/** STree instances can be added to the sidebar.  One can be a file browser.  One can be
a project, for example. */

class STree;
class STreeNode;
class KnownDocuments;
class UIWindow;

enum STreeSaveType {
  FILE_PROVIDER_SAVE_PROJECT, FILE_PROVIDER_SAVE_FILE_BROWSER
};

struct STreeSave {
  STreeSaveType type;
  std::string path;
};

enum STreeNodeType {
  NODE_TYPE_NONE, NODE_TYPE_FILE, NODE_TYPE_DIR, NODE_TYPE_WARNING, NODE_TYPE_LINK, NODE_TYPE_PROJECT
};

class STreeChangeNotifier {
public:
  /** All rows of this node are being deleted. */
  virtual void begin_rows_delete(STreeNode* fpn, int num_rows) = 0;
  virtual void begin_rows_delete(STreeNode* fpn, int start_row, int num_rows) = 0;
  /** All rows of this node have been deleted. */
  virtual void end_rows_delete() = 0;
  /** All rows of this node are being inserted. */
  virtual void begin_rows_insert(STreeNode* fpn, int num_rows) = 0;
  virtual void begin_rows_insert(STreeNode* fpn, int start_row, int num_rows) = 0;
  /** All rows of this node have been inserted. */
  virtual void end_rows_insert() = 0;
};

class STreeNode {
protected:
  STreeNode* parent;
  STree* file_provider;
  int node_index;

public:
  STreeNode(STreeNode* p, STree* fp, int index) : parent(p), file_provider(fp), node_index(index) {}
  virtual ~STreeNode(){}

  inline const STreeNode* get_parent() const { return parent; }
  inline STreeNode* get_parent() { return parent; }
  inline const STree* get_file_provider() const { return file_provider; }
  inline STree* get_file_provider() { return file_provider; }
  inline int get_index() const { return node_index; }
  inline void set_index(int index) { node_index = index; }

  virtual STreeNodeType get_type() const = 0;
  virtual std::string text() const = 0;
  virtual int num_children() const = 0;
  virtual bool is_leaf() const = 0;
  virtual const STreeNode* get_child(int child) const = 0;
  virtual STreeNode* get_child(int child) = 0;
};

class STree {
protected:
  UIWindow* ui_window;

public:
  inline STree() : ui_window(nullptr){}
  virtual ~STree(){}
  inline UIWindow* get_ui_window() { return ui_window; }
  inline void set_ui_window(UIWindow* uiw) { ui_window = uiw; }

  virtual STreeNode* get_root_node() = 0;
  virtual void activate(const STreeNode* fpn, STreeChangeNotifier* fpcn) = 0;
  /** Context requested on fpn, return a list of commands. Separators are '-'. Don't do anything
      to provide no context. */
  virtual void context_request(const STreeNode* fpn, std::vector<std::string>& contexts) = 0;
  /** Context command is chosen, invoke it. */
  virtual void context_do(const STreeNode* fpn, const std::string& context,
      STreeChangeNotifier* fpcn) = 0;
  /** Populate all known documents. */
  virtual void populate_known_documents(KnownDocuments* kd) = 0;
  virtual STreeSave get_save_info() const = 0;
};

#endif