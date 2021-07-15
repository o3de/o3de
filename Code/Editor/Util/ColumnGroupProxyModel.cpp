/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorDefs.h"

#include "ColumnGroupProxyModel.h"

// Editor
#include "Util/ColumnSortProxyModel.h"
#include "Util/AbstractSortModel.h"


ColumnGroupProxyModel::ColumnGroupProxyModel(QObject* parent)
    : AbstractGroupProxyModel(parent)
    , m_sortModel(new ColumnSortProxyModel(this))
    , m_freeSortColumn(-1)
{
    AbstractGroupProxyModel::setSourceModel(m_sortModel);
    connect(m_sortModel, &ColumnSortProxyModel::SortChanged, this, &ColumnGroupProxyModel::SortChanged);
}

void ColumnGroupProxyModel::sort(int column, Qt::SortOrder order)
{
    if (m_freeSortColumn != -1)
    {
        m_sortModel->RemoveColumnWithoutSorting(m_freeSortColumn);
        m_freeSortColumn = -1;
    }
    if (!m_groups.contains(column))
    {
        m_freeSortColumn = column;
    }
    m_sortModel->sort(column, order);
}

void ColumnGroupProxyModel::setSourceModel(QAbstractItemModel* sourceModel)
{
    Q_ASSERT(qobject_cast<AbstractSortModel*>(sourceModel));
    m_sortModel->setSourceModel(sourceModel);
    RebuildTree();
}

void ColumnGroupProxyModel::AddGroup(int column)
{
    if (!m_groups.contains(column))
    {
        m_groups.push_back(column);
        sort(column);
        Q_EMIT GroupsChanged();
    }
}

void ColumnGroupProxyModel::RemoveGroup(int column)
{
    if (m_groups.contains(column))
    {
        m_groups.remove(m_groups.indexOf(column));
        m_sortModel->RemoveColumn(column);
        Q_EMIT GroupsChanged();
    }
}

void ColumnGroupProxyModel::SetGroups(const QVector<int>& columns)
{
    m_groups.clear();
    foreach(int col, columns)
    {
        m_groups.push_back(col);
        m_sortModel->AddColumnWithoutSorting(col);
    }
    m_sortModel->SortModel();
    Q_EMIT GroupsChanged();
}

void ColumnGroupProxyModel::ClearGroups()
{
    m_groups.clear();
    m_sortModel->ClearColumns();
    Q_EMIT GroupsChanged();
}

QVector<int> ColumnGroupProxyModel::Groups() const
{
    return m_groups;
}

bool ColumnGroupProxyModel::IsColumnSorted(int col) const
{
    return m_sortModel->IsColumnSorted(col);
}

Qt::SortOrder ColumnGroupProxyModel::SortOrder(int col) const
{
    return m_sortModel->SortOrder(col);
}

QStringList ColumnGroupProxyModel::GroupForSourceIndex(const QModelIndex& sourceIndex) const
{
    QStringList group;
    foreach(int column, m_groups)
    group.push_back(QString::fromLatin1("%1: %2").arg(headerData(column, Qt::Horizontal).toString(), sourceIndex.sibling(sourceIndex.row(), column).data().toString()));
    return group;
}

#include <Util/moc_ColumnGroupProxyModel.cpp>
