#include "qtgui/sidebar_tree.hpp"

#include <QApplication>
#include <QContextMenuEvent>
#include <QMenu>

SidebarModel::SidebarModel(QWidget* parent) : QAbstractItemModel(parent) {
  QStyle* app_style = QApplication::style();
  dir_icon = QIcon(":resources/dir_icon.png");
  error_icon = app_style->standardIcon(QStyle::SP_MessageBoxCritical);
  file_icon = QIcon(":resources/file_icon.png");
  link_icon = app_style->standardIcon(QStyle::SP_FileLinkIcon);
  project_icon = QIcon(":resources/project_icon.png");
}

void SidebarModel::get_top_level_nodes(std::vector<const STreeNode*>& tl_nodes) {
  for (STree* fp : providers) {
    tl_nodes.push_back(fp->get_root_node());
  }
}

QModelIndex SidebarModel::to_index(const STreeNode* fpn) const {
  if (fpn == nullptr) {
    return QModelIndex();
  }
  return createIndex(fpn->get_index(), 0, (void*) fpn);
}

STreeNode* SidebarModel::to_node(const QModelIndex& index) const {
  if (!index.isValid()) return nullptr;
  return static_cast<STreeNode*>(index.internalPointer());
}

QModelIndex SidebarModel::index(int row, int column, const QModelIndex& parent) const {
  if (row < 0) return QModelIndex();

  const bool pvalid = parent.isValid();
  const int prow = parent.row();
  const int pcol = parent.column();

  if (pvalid) {
    const STreeNode* parent_fpn = static_cast<STreeNode*>(parent.internalPointer());
    return createIndex(row, column, (void*) (parent_fpn->get_child(row)));
  } else {
    if (row >= (int) (providers.size())) {
      return QModelIndex();
    }
    return createIndex(row, column, (void*) (providers[row]->get_root_node()));
  }
}

QVariant SidebarModel::data(const QModelIndex& index, int role) const {
  const STreeNode* fpn = to_node(index);
  if (fpn == nullptr) return QVariant();

  if (role == Qt::DisplayRole) {
    return QVariant(QString::fromStdString(fpn->text()));
  } else if (role == Qt::DecorationRole) {
    STreeNodeType t = fpn->get_type();
    if (t == NODE_TYPE_FILE) return QVariant(file_icon);
    if (t == NODE_TYPE_DIR) return QVariant(dir_icon);
    if (t == NODE_TYPE_PROJECT) return QVariant(project_icon);
    if (t == NODE_TYPE_WARNING) return QVariant(error_icon);
    if (t == NODE_TYPE_LINK) return QVariant(link_icon);
  }

  return QVariant();
}

QModelIndex SidebarModel::parent(const QModelIndex& index) const {
  const STreeNode* fpn = to_node(index);
  if (fpn == nullptr) return QModelIndex();
  return to_index(fpn->get_parent());
}

int SidebarModel::rowCount(const QModelIndex& index) const {
  const STreeNode* fpn = to_node(index);
  if (fpn == nullptr) return (int) providers.size();
  return fpn->num_children();
}

int SidebarModel::columnCount(const QModelIndex& /* parent */) const {
  return 1;
}

bool SidebarModel::hasChildren(const QModelIndex& parent) const {
  const STreeNode* fpn = to_node(parent);
  if (fpn == nullptr) return true;
  return !fpn->is_leaf();
}

void SidebarModel::begin_rows_delete(STreeNode* fpn, int num_rows) {
  beginRemoveRows(to_index(fpn), 0, num_rows-1);
}

void SidebarModel::begin_rows_delete(STreeNode* fpn, int start_row, int num_rows) {
  beginRemoveRows(to_index(fpn), start_row, start_row + num_rows-1);
}

void SidebarModel::end_rows_delete() {
  endRemoveRows();
}

void SidebarModel::begin_rows_insert(STreeNode* fpn, int num_rows) {
  beginInsertRows(to_index(fpn), 0, num_rows-1);
}

void SidebarModel::begin_rows_insert(STreeNode* fpn, int start_row, int num_rows) {
  beginInsertRows(to_index(fpn), start_row, start_row + num_rows-1);
}

void SidebarModel::end_rows_insert() {
  endInsertRows();
}

void SidebarModel::add_provider(STree* provider) {
  begin_rows_insert(nullptr, providers.size(), 1);
  provider->get_root_node()->set_index(providers.size());
  providers.push_back(provider);
  end_rows_insert();
}

void SidebarModel::add_provider_first(STree* provider) {
  begin_rows_insert(nullptr, 0, 1);
  provider->get_root_node()->set_index(0);
  providers.insert(providers.begin(), provider);
  for (unsigned int i = 1; i < providers.size(); i++) {
    providers[i]->get_root_node()->set_index(i);
  }
  end_rows_insert();
}

void SidebarModel::remove_provider(STree* provider) {
  for (unsigned int i = 0; i < providers.size(); i++) {
    STree* fp = providers[i];
    if (fp == provider) {
      begin_rows_delete(nullptr, i, 1);
      providers.erase(providers.begin() + i);
      end_rows_delete();
      return;
    }
  }
}

SidebarTree::SidebarTree(QWidget* parent) : QTreeView(parent) {
  setHeaderHidden(true);
  setIndentation(12);
  setExpandsOnDoubleClick(false);
  setFocusPolicy(Qt::NoFocus);
  setTextElideMode(Qt::ElideLeft);
  connect(this, &QTreeView::doubleClicked, this, &SidebarTree::item_double_clicked);
}

void SidebarTree::expand_top_level() {
  SidebarModel* mdl = dynamic_cast<SidebarModel*>(model());
  std::vector<const STreeNode*> tl_nodes;
  mdl->get_top_level_nodes(tl_nodes);
  for (const STreeNode* fpn: tl_nodes) {
    QModelIndex index = mdl->to_index(fpn);
    expand(index);
  }
  update();
}

void SidebarTree::add_provider(STree* provider, bool expanded) {
  SidebarModel* mdl = dynamic_cast<SidebarModel*>(model());
  mdl->add_provider(provider);
  std::vector<const STreeNode*> tl_nodes;
  mdl->get_top_level_nodes(tl_nodes);
  const STreeNode* fpn = tl_nodes.back();
  QModelIndex index = mdl->to_index(fpn);
  if (expanded) expand(index);
  update();
}

void SidebarTree::add_provider_first(STree* provider, bool expanded) {
  SidebarModel* mdl = dynamic_cast<SidebarModel*>(model());
  mdl->add_provider_first(provider);
  std::vector<const STreeNode*> tl_nodes;
  mdl->get_top_level_nodes(tl_nodes);
  const STreeNode* fpn = tl_nodes.front();
  QModelIndex index = mdl->to_index(fpn);
  if (expanded) expand(index);
  update();
}

void SidebarTree::remove_provider(STree* provider) {
  SidebarModel* mdl = dynamic_cast<SidebarModel*>(model());
  mdl->remove_provider(provider);
}

void SidebarTree::contextMenuEvent(QContextMenuEvent* event) {
  SidebarModel* mdl = dynamic_cast<SidebarModel*>(model());

  QPoint pt = event->pos();
  QModelIndex index = indexAt(pt);
  STreeNode* fpn = mdl->to_node(index);
  if (fpn == nullptr) return;
  STree* fp = fpn->get_file_provider();
  std::vector<std::string> contexts;
  fp->context_request(fpn, contexts);
  if (contexts.empty()) return;

  // Create menu and execute it.
  QMenu menu;
  for (std::string& ctx: contexts) {
    if (ctx == "-") menu.addSeparator();
    else {
      QAction* action = menu.addAction(QString::fromStdString(ctx));
    }
  }
  connect(&menu, &QMenu::triggered, this, &SidebarTree::context_clicked);

  // Save these because they will be called in the triggered callback:
  triggering_file_provider = fp;
  triggering_file_provider_node = fpn;
  menu.exec(event->globalPos());
}

void SidebarTree::item_double_clicked(const QModelIndex& index) {
  const bool valid = index.isValid();
  if (!valid) return;
  STreeNode* fpn = static_cast<STreeNode*>(index.internalPointer());
  STree* fp = fpn->get_file_provider();
  fp->activate(fpn, dynamic_cast<STreeChangeNotifier*>(model()));
}

void SidebarTree::context_clicked(QAction* action) {
  triggering_file_provider->context_do(triggering_file_provider_node, action->text().toStdString(),
      dynamic_cast<STreeChangeNotifier*>(model()));
}