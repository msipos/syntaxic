#include "core/utf8_util.hpp"
#include "core/util_path.hpp"
#include "file_browser.hpp"
#include "known_documents.hpp"
#include "master.hpp"
#include "master_io_provider.hpp"
#include "uiwindow.hpp"

#include <QDir>
#include <QFileInfo>

FileNode::FileNode(STreeNode* p, FileBrowser* fb, int index, const std::string& name, const std::string& fn, int t) : STreeNode(p, fb, index), short_name(name), abs_path(fn), is_loaded(false), type(t), movable(true) {
}

STreeNodeType FileNode::get_type() const {
  if (type == FB_DIR || type == FB_ROOT || type == FB_PARENT) return NODE_TYPE_DIR;
  if (type == FB_FILE) return NODE_TYPE_FILE;
  if (type == FB_PROJECT_ROOT) return NODE_TYPE_PROJECT;
  if (type == FB_WARNING) return NODE_TYPE_WARNING;
  if (type == FB_LINK) return NODE_TYPE_LINK;
  return NODE_TYPE_NONE;
}

std::string FileNode::text() const {
  if (type == FB_PARENT) return "..";
  if (type == FB_ROOT) {
    if (abs_path == "") return "Root";
    return abs_path;
  }
  return short_name;
}

int FileNode::num_children() const {
  if (type == FB_FILE || type == FB_PARENT || type == FB_LINK) return 0;
  if (!is_loaded) {
    // TODO: Ugly! But no other way.  QT's tree model has const virtual interface but we need lazy
    // loading.
    FileNode* fn = const_cast<FileNode*>(this);
    fn->count_and_process_dir(true);
  }
  return children.size();
}

bool FileNode::is_leaf() const {
  if (type == FB_DIR || type == FB_ROOT || type == FB_PROJECT_ROOT) return false;
  return true;
}

void FileNode::add_known_documents(KnownDocuments* kd) {
  if (is_leaf()) {
    kd->add_document(abs_path);
    return;
  }
  if (!is_loaded) {
    FileBrowser* file_browser = dynamic_cast<FileBrowser*>(file_provider);
    if (file_browser->get_is_project()) {
      count_and_process_dir(true);
    } else return;
  }
  for (std::unique_ptr<FileNode>& fn: children) {
    fn->add_known_documents(kd);
  }
}

// TODO: Annoying duplication?
const STreeNode* FileNode::get_child(int child) const {
  if (type == FB_FILE) {
    printf("WARNING: FileNode::get_child but not dir.\n");
    return nullptr;
  }
  if (child < 0 || child >= int(children.size())) {
    printf("WARNING: FileNode::get_child out of range.\n");
    return nullptr;
  }
  return children[child].get();
}

STreeNode* FileNode::get_child(int child) {
  if (type == FB_FILE) {
    printf("WARNING: FileNode::get_child but not dir.\n");
    return nullptr;
  }
  if (child < 0 ||  child >= int(children.size())) {
    printf("WARNING: FileNode::get_child out of range.\n");
    return nullptr;
  }
  return children[child].get();
}

int FileNode::count_and_process_dir(bool process) {
  FileBrowser* file_browser = dynamic_cast<FileBrowser*>(file_provider);
  const std::string& filter = file_browser->get_filter();

  try {
    std::vector<DirEntry> entries = master_io_provider->list_dir(abs_path, filter);
    is_loaded = true;

    int j = 0;
    if (type == FB_ROOT && movable) {
      std::string parent_path;
      if (master_io_provider->parent_dir(abs_path, parent_path)) {
        j++;
        if (process) {
          const int child_type = FB_PARENT;
          children.push_back(std::unique_ptr<FileNode>(new FileNode(this, file_browser, j, "..", parent_path, child_type)));
        }
      }
    }

    for (DirEntry& de: entries) {
      if (process) {
        int child_type = FB_FILE;
        if (de.type == DirEntryType::DIR) child_type = FB_DIR;
        if (de.type == DirEntryType::LINK) child_type = FB_LINK;
        std::string child_full_path;
        if (abs_path.empty()) child_full_path = de.name;
        else child_full_path = UtilPath::join_components(abs_path, de.name);
        children.push_back(std::unique_ptr<FileNode>(new FileNode(this, file_browser, j, de.name, child_full_path, child_type)));
      }
      j++;
    }
    return j;
  } catch (std::exception& e) {
    if (process) {
      // TODO: In the future, maybe make IO error clickable with e.what().
      children.push_back(std::unique_ptr<FileNode>(new FileNode(this, file_browser, 0, "I/O Error", "", FB_WARNING)));
    }
    return 1;
  }
}

FileBrowser::FileBrowser(const std::string& path) : movable(true), is_project(false) {
  current_dir_abs = UtilPath::to_absolute(path);
  root_node = std::unique_ptr<FileNode>(new FileNode(nullptr, this, 0, "File browser", current_dir_abs, FB_ROOT));
  root_node->set_movable(movable);
}

STreeNode* FileBrowser::get_root_node() {
  return root_node.get();
}

void FileBrowser::activate(const STreeNode* fpn, STreeChangeNotifier* fpcn) {
  const FileNode* fn = dynamic_cast<const FileNode*>(fpn);

  switch (fn->type) {
    case FB_DIR:
    case FB_PARENT:
      if (!movable) return;

      {
        std::string new_path = fn->abs_path;

        int num_rows = root_node->num_children();
        fpcn->begin_rows_delete(root_node.get(), num_rows);
        root_node->children.clear();
        fpcn->end_rows_delete();

        current_dir_abs = new_path;
        master.settings.set_current_path(current_dir_abs);

        root_node->abs_path = current_dir_abs;
        root_node->type = FB_ROOT;
        num_rows = root_node->count_and_process_dir(false);
        fpcn->begin_rows_insert(root_node.get(), num_rows);
        root_node->count_and_process_dir(true);
        fpcn->end_rows_insert();
      }
      break;
    case FB_FILE:
      master.open_document(fn->abs_path.c_str(), nullptr);
      break;
  }
}

void FileBrowser::refresh_node(FileNode* fn, STreeChangeNotifier* fpcn) {
  int num_rows = fn->num_children();
  fpcn->begin_rows_delete(fn, num_rows);
  fn->children.clear();
  fpcn->end_rows_delete();

  num_rows = fn->count_and_process_dir(false);
  fpcn->begin_rows_insert(fn, num_rows);
  fn->count_and_process_dir(true);
  fpcn->end_rows_insert();
}

// TODO: Ugly to use strings like this, so fix it one day!

void FileBrowser::context_request(const STreeNode* fpn, std::vector<std::string>& contexts) {
  FileNode* fn = (FileNode*) fpn;
  if (fn->type == FB_WARNING || fn->type == FB_PARENT || fn->type == FB_NONE) return;
  contexts.push_back("New file...");
  contexts.push_back("New directory...");
  contexts.push_back("-");
  if (fpn != root_node.get()) {
    contexts.push_back("Rename...");
    contexts.push_back("Delete...");
    contexts.push_back("-");
  }
  contexts.push_back("Refresh");
  contexts.push_back("-");
  if (is_project) {
    contexts.push_back("Configure project");
    contexts.push_back("Close project");
  } else {
    contexts.push_back("Close file browser");
  }
}

void FileBrowser::context_do(const STreeNode* fpn, const std::string& context,
    STreeChangeNotifier* fpcn) {
  FileNode* fn = (FileNode*) fpn;

  // Find "effective fn".  If "New file...", "New directory..." or "Refresh" were called on a file,
  // then the file's parent is assumed to be the fn.
  if (context == "New file..." || context == "New directory..." || context == "Refresh") {
    if (fn->type == FB_FILE)
    fn = (FileNode*)(fn->get_parent());
  }
  if (fn->type == FB_WARNING || fn->type == FB_PARENT || fn->type == FB_NONE) return;

  if (fn == nullptr) return;
  if (context == "Refresh") {
    refresh_node(fn, fpcn);
    return;
  }

  if (context == "New file..." || context == "New directory...") {
    bool is_dir = context == "New directory...";
    std::string new_name;
    const std::string thing = is_dir ? "directory" : "file";
    if (ui_window->get_user_text_input("New " + thing, "Enter new " + thing + " name:",
        "", 0, new_name) == UI_CANCEL) return;
    if (new_name.empty()) {
      ui_window->feedback("New " + thing, "Invalid " + thing + " name");
    }

    std::string full_name = UtilPath::join_components(fn->abs_path, new_name);

    try {
      master_io_provider->touch(full_name, is_dir ? DirEntryType::DIR : DirEntryType::FILE);
    } catch (std::exception& e) {
      ui_window->get_user_input("Could not create", e.what(), UI_OK | UI_WARNING);
    }

    refresh_node(fn, fpcn);
    return;
  }

  if (context == "Rename...") {
    if (fn->type == FB_ROOT || fn->type == FB_PARENT) return;
    FileNode* parent = (FileNode*) (fn->get_parent());
    if (parent == nullptr) return;

    std::string new_name;
    if (ui_window->get_user_text_input("Rename", "Enter new name:", fn->short_name, 0, new_name) == UI_CANCEL) return;
    std::string abs_new_path = UtilPath::join_components(parent->abs_path, new_name);

    try {
      master_io_provider->rename(fn->abs_path, abs_new_path);
    } catch (std::exception& e) {
      ui_window->get_user_input("Could not rename", e.what(), UI_OK | UI_WARNING);
    }

    refresh_node(parent, fpcn);
    return;
  }

  if (context == "Delete...") {
    if (fn->type == FB_ROOT || fn->type == FB_PARENT) return;

    if (fn->type == FB_DIR) {
      std::string text = "Are you sure you wish to delete the directory:\n\n";
      text += fn->abs_path;
      text += "\n\nAll contents of the directory will be recursively deleted!";
      int rv = ui_window->get_user_input("Delete directory", text, UI_WARNING | UI_YES | UI_NO);
      if (rv != UI_YES) return;
    } else if (fn->type == FB_FILE) {
      std::string text = "Are you sure you wish to delete the file:\n\n";
      text += fn->abs_path;
      text += "\n\nFile will be irrevocably deleted!";
      int rv = ui_window->get_user_input("Delete file", text, UI_WARNING | UI_YES | UI_NO);
      if (rv != UI_YES) return;
    } else {
      ui_window->feedback("Unknown type", "Can't delete.");
      return;
    }

    try {
      master_io_provider->remove(fn->abs_path);
    } catch (std::exception& e) {
      ui_window->get_user_input("Could not delete", e.what(), UI_OK | UI_WARNING);
    }

    FileNode* parent = (FileNode*) (fn->get_parent());
    refresh_node(parent, fpcn);
    return;
  }

  if (context == "Close project" || context == "Close file browser") {
    master.remove_file_provider(this);
    return;
  }
}

STreeSave FileBrowser::get_save_info() const {
  return { FILE_PROVIDER_SAVE_FILE_BROWSER, current_dir_abs };
}

void FileBrowser::populate_known_documents(KnownDocuments* kd) {
  root_node->add_known_documents(kd);
}

void FileBrowser::set_project(const std::string& project_name) {
  movable = false;
  root_node->set_movable(false);
  root_node->type = FB_PROJECT_ROOT;
  root_node->short_name = project_name;
  is_project = true;
}

std::string FileBrowser::get_filter() {
  if (filter.empty()) {
    return master.pref_manager.get_string("sidebar.default_filter");
  }
  return filter;
}