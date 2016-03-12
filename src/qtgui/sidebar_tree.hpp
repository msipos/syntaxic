#ifndef SYNTAXIC_QTGUI_SIDEBAR_TREE_HPP
#define SYNTAXIC_QTGUI_SIDEBAR_TREE_HPP

#include "stree.hpp"

#include <vector>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QTreeView>
#include <QVariant>

class QAction;
class QContextMenuEvent;

class SidebarModel : public QAbstractItemModel, public STreeChangeNotifier {
  Q_OBJECT

private:
  std::vector<STree*> providers;

  // Few icons
  QIcon file_icon, dir_icon, project_icon, error_icon, link_icon;

public:
  SidebarModel(QWidget* parent);

  void get_top_level_nodes(std::vector<const STreeNode*>& tl_nodes);

  QModelIndex to_index(const STreeNode* fpn) const;
  STreeNode* to_node(const QModelIndex& index) const;

  // Implement QAbstractItemModel:
  virtual QModelIndex index(int row, int column, const QModelIndex& parent=QModelIndex()) const
     override;
  virtual QVariant data(const QModelIndex& index, int role=Qt::DisplayRole) const override;
  virtual QModelIndex parent(const QModelIndex& index) const override;
  virtual int rowCount(const QModelIndex& parent=QModelIndex()) const override;
  virtual int columnCount(const QModelIndex& parent=QModelIndex()) const override;
  virtual bool hasChildren(const QModelIndex & parent = QModelIndex()) const override;

  // Implement STreeChangeNotifier:
  virtual void begin_rows_delete(STreeNode* fpn, int num_rows) override;
  virtual void begin_rows_delete(STreeNode* fpn, int start_row, int num_rows) override;
  virtual void end_rows_delete() override;
  virtual void begin_rows_insert(STreeNode* fpn, int num_rows) override;
  virtual void begin_rows_insert(STreeNode* fpn, int start_row, int num_rows) override;
  virtual void end_rows_insert() override;

  void add_provider(STree* provider);
  void add_provider_first(STree* provider);
  void remove_provider(STree* provider);
};

class SidebarTree : public QTreeView {
  Q_OBJECT

private:
  STree* triggering_file_provider;
  STreeNode* triggering_file_provider_node;

public:
  SidebarTree(QWidget* parent);

  void expand_top_level();
  void add_provider(STree* provider, bool expanded);
  void add_provider_first(STree* provider, bool expanded);
  void remove_provider(STree* provider);

  virtual void contextMenuEvent(QContextMenuEvent* event) override;

public slots:
  void item_double_clicked(const QModelIndex& index);
  void context_clicked(QAction* action);
};

#endif