/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "AbstractGroupProxyModel.h"

AbstractGroupProxyModel::AbstractGroupProxyModel(QObject* parent)
    : QAbstractProxyModel(parent)
{
}

AbstractGroupProxyModel::~AbstractGroupProxyModel()
{
}

QVariant AbstractGroupProxyModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
    {
        return QVariant();
    }

    const GroupItem* group = reinterpret_cast<GroupItem*>(index.internalPointer());
    if (index.row() >= group->subGroups.count())
    {
        return sourceModel()->data(mapToSource(index), role);
    }
    else if (role == Qt::DisplayRole && index.column() == 0 && !group->subGroups.at(index.row())->groupTitle.isEmpty())
    {
        return group->subGroups[index.row()]->groupTitle;
    }
    else if (group->subGroups.at(index.row())->groupSourceIndex.isValid())
    {
        return group->subGroups.at(index.row())->groupSourceIndex.data(role);
    }
    return QVariant();
}

int AbstractGroupProxyModel::rowCount(const QModelIndex& parent) const
{
    if (!sourceModel())
    {
        return 0;
    }

    // invalid parent - root item is used
    if (!parent.isValid())
    {
        return m_rootItem.subGroups.count() + m_rootItem.sourceIndexes.count();
    }
    // this is the group the parent is in.
    const GroupItem* group = reinterpret_cast<GroupItem*>(parent.internalPointer());
    if (parent.row() < group->subGroups.count())
    {
        return group->subGroups[parent.row()]->subGroups.count() + group->subGroups[parent.row()]->sourceIndexes.count();
    }
    return 0;
}

int AbstractGroupProxyModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    if (!sourceModel())
    {
        return 0;
    }
    return sourceModel()->columnCount(QModelIndex());
}

QModelIndex AbstractGroupProxyModel::index(int row, int column, const QModelIndex& parent) const
{
    if (row >= rowCount(parent) || column >= columnCount(parent))
    {
        return QModelIndex();
    }
    if (!parent.isValid())
    {
        return createIndex(row, column, const_cast<GroupItem*>(&m_rootItem));
    }
    const GroupItem* group = reinterpret_cast<GroupItem*>(parent.internalPointer());
    GroupItem* newParent = group->subGroups[parent.row()];
    return createIndex(row, column, newParent);
}

QVariant AbstractGroupProxyModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return sourceModel()->headerData(section, orientation, role);
}

QModelIndex AbstractGroupProxyModel::parent(const QModelIndex& index) const
{
    GroupItem* group = reinterpret_cast<GroupItem*>(index.internalPointer());
    if (!group)
    {
        return QModelIndex();
    }
    const GroupItem* parentGroup = FindGroup(group);
    if (!parentGroup)
    {
        return QModelIndex();
    }
    const int row = parentGroup->subGroups.indexOf(group);
    return createIndex(row, 0, const_cast<GroupItem*>(parentGroup));
}

bool AbstractGroupProxyModel::hasChildren(const QModelIndex& parent) const
{
    if (!parent.isValid())
    {
        return true;
    }
    const GroupItem* group = reinterpret_cast<GroupItem*>(parent.internalPointer());
    return parent.row() < group->subGroups.count();
}

Qt::ItemFlags AbstractGroupProxyModel::flags(const QModelIndex& index) const
{
    const QModelIndex sourceIndex = mapToSource(index);
    if (!sourceIndex.isValid())
    {
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }
    return sourceModel()->flags(sourceIndex);
}

QModelIndex AbstractGroupProxyModel::mapToSource(const QModelIndex& proxyIndex) const
{
    GroupItem* group = reinterpret_cast<GroupItem*>(proxyIndex.internalPointer());
    if (!group)
    {
        return QModelIndex();
    }
    const int i = proxyIndex.row() - group->subGroups.count();
    if (i < 0)
    {
        return QModelIndex();
    }
    return group->sourceIndexes[i].sibling(group->sourceIndexes[i].row(), proxyIndex.column());
}

QModelIndex AbstractGroupProxyModel::mapFromSource(const QModelIndex& sourceIndex) const
{
    if (!sourceIndex.isValid())
    {
        return QModelIndex();
    }
    
    GroupItem* group = FindIndex(sourceIndex.sibling(sourceIndex.row(), 0));
    if (!group)
    {
        return QModelIndex();
    }

    if (group->groupSourceIndex == sourceIndex)
    {
        GroupItem* parentGroup = FindGroup(group);
        return createIndex(parentGroup->subGroups.indexOf(group), sourceIndex.column(), parentGroup);
    }
    return createIndex(group->subGroups.count() + group->sourceIndexes.indexOf(sourceIndex.sibling(sourceIndex.row(), 0)),
        sourceIndex.column(), const_cast<GroupItem*>(group));
}

AbstractGroupProxyModel::GroupItem* AbstractGroupProxyModel::FindIndex(const QModelIndex& index, GroupItem* group) const
{
    if (group == nullptr)
    {
        group = const_cast<GroupItem*>(&m_rootItem);
    }

    if (group->sourceIndexes.contains(index) || group->groupSourceIndex == index)
    {
        return group;
    }
    for (GroupItem* subGroup : group->subGroups)
    {
        GroupItem* g = FindIndex(index, subGroup);
        if (g)
        {
            return g;
        }
    }
    return nullptr;
}

AbstractGroupProxyModel::GroupItem* AbstractGroupProxyModel::FindGroup(GroupItem* group, GroupItem* parent) const
{
    if (parent == nullptr)
    {
        parent = const_cast<GroupItem*>(&m_rootItem);
    }

    if (parent->subGroups.contains(group))
    {
        return parent;
    }
    for (GroupItem* subGroup : parent->subGroups)
    {
        GroupItem* g = FindGroup(group, subGroup);
        if (g)
        {
            return g;
        }
    }
    return nullptr;
}

void AbstractGroupProxyModel::setSourceModel(QAbstractItemModel* sourceModel)
{
    QAbstractProxyModel::setSourceModel(sourceModel);
    connect(sourceModel, &QAbstractItemModel::rowsInserted, this, &AbstractGroupProxyModel::SourceRowsInserted);
    connect(sourceModel, &QAbstractItemModel::rowsAboutToBeRemoved, this, &AbstractGroupProxyModel::SourceRowsAboutToBeRemoved);
    connect(sourceModel, &QAbstractItemModel::dataChanged, this, &AbstractGroupProxyModel::SourceDataChanged);
    connect(sourceModel, &QAbstractItemModel::modelAboutToBeReset, this, &AbstractGroupProxyModel::slotSourceAboutToBeReset);
    connect(sourceModel, &QAbstractItemModel::modelReset, this, &AbstractGroupProxyModel::slotSourceReset);
    connect(sourceModel, &QAbstractItemModel::layoutAboutToBeChanged, this, &AbstractGroupProxyModel::slotSourceAboutToBeReset);
    connect(sourceModel, &QAbstractItemModel::layoutChanged, this, &AbstractGroupProxyModel::slotSourceReset);
    RebuildTree();
}

void AbstractGroupProxyModel::slotSourceAboutToBeReset()
{
    beginResetModel();
    qDeleteAll(m_rootItem.subGroups);
    m_rootItem.subGroups.clear();
    m_rootItem.sourceIndexes.clear();
}

void AbstractGroupProxyModel::slotSourceReset()
{
    const int rowCount = sourceModel() ? sourceModel()->rowCount() : 0;
    for (int row = 0; row < rowCount; ++row)
    {
        const QModelIndex sourceIndex = sourceModel()->index(row, 0);
        GroupItem* group = CreateGroupIfNotExists(GroupForSourceIndex(sourceIndex));
        if (IsGroupIndex(sourceIndex))
        {
            group->groupSourceIndex = sourceIndex;
        }
        else
        {
            group->sourceIndexes.push_back(sourceIndex);
        }
    }
    endResetModel();
}

void AbstractGroupProxyModel::RebuildTree()
{
    beginResetModel();
    {
        QSignalBlocker blocker(this);
        slotSourceAboutToBeReset();
        slotSourceReset();
    }
    endResetModel();
    Q_EMIT GroupUpdated();
}

int AbstractGroupProxyModel::subGroupCount() const
{
    return m_rootItem.subGroups.count();
}

void AbstractGroupProxyModel::SourceRowsInserted(const QModelIndex& p, int from, int to)
{
    if (p.isValid())
    {
        return;
    }

    for (int row = from; row <= to; ++row)
    {
        const QModelIndex sourceIndex = sourceModel()->index(row, 0);
        GroupItem* group = CreateGroupIfNotExists(GroupForSourceIndex(sourceIndex));
        if (IsGroupIndex(sourceIndex))
        {
            group->groupSourceIndex = sourceIndex;
        }
        else
        {
            const int modelRow = group->subGroups.count() + group->sourceIndexes.count();
            beginInsertRows(parent(createIndex(0, 0, group)), modelRow, modelRow);
            group->sourceIndexes.push_back(sourceIndex);
            endInsertRows();
        }
    }
    Q_EMIT GroupUpdated();
}

void AbstractGroupProxyModel::SourceRowsAboutToBeRemoved(const QModelIndex& p, int from, int to)
{
    if (p.isValid())
    {
        return;
    }

    for (int row = from; row <= to; ++row)
    {
        const QModelIndex sourceIndex = sourceModel()->index(row, 0);
        GroupItem* group = const_cast<GroupItem*>(FindIndex(sourceIndex));
        if (!group)
        {
            continue;
        }
        if (group->groupSourceIndex != sourceIndex)
        {
            const int modelRow = group->subGroups.count() + group->sourceIndexes.indexOf(sourceIndex);
            beginRemoveRows(parent(createIndex(0, 0, group)), modelRow, modelRow);
            group->sourceIndexes.remove(group->sourceIndexes.indexOf(sourceIndex));
            endRemoveRows();
        }
        RemoveEmptyGroup(group);
    }
    Q_EMIT GroupUpdated();
}

void AbstractGroupProxyModel::SourceDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    if (topLeft.parent().isValid())
    {
        return;
    }

    for (int row = topLeft.row(); row <= bottomRight.row(); ++row)
    {
        const QModelIndex sourceIndex = sourceModel()->index(row, 0);
        const GroupItem* currentGroup = FindIndex(sourceIndex);
        GroupItem* newGroup = CreateGroupIfNotExists(GroupForSourceIndex(sourceIndex));
        if (currentGroup != newGroup)
        {
            SourceRowsAboutToBeRemoved(QModelIndex(), row, row);
            SourceRowsInserted(QModelIndex(), row, row);
        }
        else
        {
            emit dataChanged(mapFromSource(sourceIndex), mapFromSource(sourceIndex.sibling(row, columnCount() - 1)));
        }
    }
    Q_EMIT GroupUpdated();
}

AbstractGroupProxyModel::GroupItem* AbstractGroupProxyModel::CreateGroupIfNotExists(QStringList group)
{
    GroupItem* currentGroup = &m_rootItem;

    while (true)
    {
        if (group.isEmpty())
        {
            return currentGroup;
        }
        auto matchingSubGroup = std::find_if(currentGroup->subGroups.begin(), currentGroup->subGroups.end(),
                [=](const GroupItem* g)
                {
                    return QString::compare(g->groupTitle, group.first(), Qt::CaseInsensitive) == 0;
                }
                );
        if (matchingSubGroup == currentGroup->subGroups.end())
        {
            GroupItem* newGroup = new GroupItem;
            newGroup->groupTitle = group.first();
            beginInsertRows(parent(createIndex(0, 0, currentGroup)),
                currentGroup->subGroups.size(), currentGroup->subGroups.size());
            currentGroup->subGroups.push_back(newGroup);
            endInsertRows();
            currentGroup = currentGroup->subGroups[currentGroup->subGroups.size() - 1];
        }
        else
        {
            currentGroup = *matchingSubGroup;
        }
        group.pop_front();
    }
}

void AbstractGroupProxyModel::RemoveEmptyGroup(GroupItem* group)
{
    if (!group->subGroups.isEmpty() || !group->sourceIndexes.isEmpty() || group == &m_rootItem
        || (group->groupSourceIndex.isValid() && !IsGroupIndex(group->groupSourceIndex)))
    {
        return;
    }
    GroupItem* parentGroup = const_cast<GroupItem*>(FindGroup(group));
    const int row = parentGroup->subGroups.indexOf(group);
    beginRemoveRows(parent(createIndex(0, 0, parentGroup)), row, row);
    delete parentGroup->subGroups.takeAt(row);
    endRemoveRows();
    RemoveEmptyGroup(parentGroup);
}

#include <Util/moc_AbstractGroupProxyModel.cpp>
