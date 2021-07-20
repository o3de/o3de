/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorDefs.h"

#include "ColumnGroupTreeView.h"

// Editor
#include "Util/ColumnGroupHeaderView.h"
#include "Util/ColumnGroupProxyModel.h"
#include "Util/ColumnGroupItemDelegate.h"


ColumnGroupTreeView::ColumnGroupTreeView(QWidget* parent)
    : QTreeView(parent)
    , m_header(new ColumnGroupHeaderView)
    , m_groupModel(new ColumnGroupProxyModel(this))
{
    setSortingEnabled(true);
    setHeader(m_header);
    setItemDelegate(new ColumnGroupItemDelegate(this));
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);

    QTreeView::setModel(m_groupModel);
    connect(m_groupModel, &QAbstractItemModel::modelAboutToBeReset, this, &ColumnGroupTreeView::SaveOpenState);
    connect(m_groupModel, &QAbstractItemModel::modelReset, this, &ColumnGroupTreeView::RestoreOpenState);
    connect(m_groupModel, SIGNAL(GroupUpdated()), this, SLOT(SpanGroups()));
    connect(m_groupModel, &ColumnGroupProxyModel::GroupsChanged, this, &ColumnGroupTreeView::expandAll);
}

void ColumnGroupTreeView::setModel(QAbstractItemModel* model)
{
    m_groupModel->setSourceModel(model);
    if (model)
    {
        connect(model, &QAbstractItemModel::modelReset, this, &QTreeView::expandAll, Qt::QueuedConnection);
    }
}

bool ColumnGroupTreeView::IsGroupsShown() const
{
    return m_header->IsGroupsShown();
}

void ColumnGroupTreeView::ShowGroups(bool showGroups)
{
    m_header->ShowGroups(showGroups);
}

QSet<QString> GetOpenNodes(QTreeView* tree, const QModelIndex& parent)
{
    int rows = tree->model()->rowCount(parent);
    QSet<QString> results;

    for (int row = 0; row < rows; ++row)
    {
        auto index = tree->model()->index(row, 0, parent);
        if (tree->isExpanded(index))
        {
            results.insert(index.data().toString());
        }
        results.unite(GetOpenNodes(tree, index));
    }
    return results;
}

void ColumnGroupTreeView::SaveOpenState()
{
    m_openNodes = GetOpenNodes(this, QModelIndex());
}

void RestoreOpenNodes(QTreeView* tree, const QSet<QString> openNodes, const QModelIndex& parent)
{
    int rows = tree->model()->rowCount(parent);
    QSet<QString> results;

    for (int row = 0; row < rows; ++row)
    {
        auto index = tree->model()->index(row, 0, parent);
        auto text = index.data().toString();
        if (openNodes.contains(text))
        {
            tree->expand(index);
        }
        RestoreOpenNodes(tree, openNodes, index);
    }
}

void ColumnGroupTreeView::RestoreOpenState()
{
    RestoreOpenNodes(this, m_openNodes, QModelIndex());
}

void ColumnGroupTreeView::Sort(int column, Qt::SortOrder order)
{
    m_groupModel->sort(column, order);
    m_header->setSortIndicator(column, order);
}

void ColumnGroupTreeView::ToggleSortOrder(int column)
{
    auto sortOrder = m_groupModel->SortOrder(column) == Qt::AscendingOrder ? Qt::DescendingOrder : Qt::AscendingOrder;
    m_groupModel->sort(column, sortOrder);
}

void ColumnGroupTreeView::AddGroup(int column)
{
    m_groupModel->AddGroup(column);
}

void ColumnGroupTreeView::RemoveGroup(int column)
{
    m_groupModel->RemoveGroup(column);
}

void ColumnGroupTreeView::SetGroups(const QVector<int>& columns)
{
    m_groupModel->SetGroups(columns);
}

void ColumnGroupTreeView::ClearGroups()
{
    m_groupModel->ClearGroups();
}

QVector<int> ColumnGroupTreeView::Groups() const
{
    return m_groupModel->Groups();
}

void ColumnGroupTreeView::SpanGroups(const QModelIndex& index)
{
    int childrenCount = m_groupModel->rowCount(index);
    for (int row = 0; row < childrenCount; ++row)
    {
        auto childIndex = m_groupModel->index(row, 0, index);
        bool hasChildren = m_groupModel->hasChildren(childIndex);
        if (hasChildren)
        {
            setFirstColumnSpanned(row, index, true);
            SpanGroups(childIndex);
        }
    }
}

QModelIndex ColumnGroupTreeView::mapToSource(const QModelIndex& proxyIndex) const
{
    auto sortProxy = qobject_cast<QAbstractProxyModel*>(m_groupModel->sourceModel());
    return sortProxy->mapToSource(m_groupModel->mapToSource(proxyIndex));
}

QModelIndex ColumnGroupTreeView::mapFromSource(const QModelIndex& sourceModel) const
{
    auto sortProxy = qobject_cast<QAbstractProxyModel*>(m_groupModel->sourceModel());
    return m_groupModel->mapFromSource(sortProxy->mapFromSource(sourceModel));
}


#include <Util/moc_ColumnGroupTreeView.cpp>
