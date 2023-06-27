/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzToolsFramework/AssetBrowser/AssetBrowserTreeToTableProxyModel.h>
#include <AzCore/std/functional.h>
#include <AzCore/Debug/Trace.h>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        ConstTableIterator IndexToMap::TableConstBegin() const
        {
            return m_tableToTree.constBegin();
        }

        ConstTableIterator IndexToMap::TableConstEnd() const
        {
            return m_tableToTree.constEnd();
        }

        TableIterator IndexToMap::TableLowerBound(const int& key)
        {
            return m_tableToTree.lowerBound(key);
        }

        ConstTableIterator IndexToMap::TableLowerBound(const int& key) const
        {
            return m_tableToTree.lowerBound(key);
        }

        TableIterator IndexToMap::TableUpperBound(const int& key)
        {
            return m_tableToTree.upperBound(key);
        }

        TableIterator IndexToMap::TableEnd()
        {
            return m_tableToTree.end();
        }

        TableIterator IndexToMap::EraseFromTable(TableIterator it)
        {
            m_treeToTable.remove(it.value());
            return m_tableToTree.erase(it);
        }

        bool IndexToMap::Empty() const
        {
            return m_treeToTable.isEmpty();
        }

        bool IndexToMap::TreeContains(TableType map) const
        {
            return m_treeToTable.contains(map);
        }

        TreeType IndexToMap::TreeToTable(TableType map) const
        {
            return m_treeToTable.value(map);
        }

        bool IndexToMap::RemoveFromTree(TableType map)
        {
            const TreeType row = m_treeToTable.take(map);
            return m_tableToTree.remove(row) != 0;
        }

        TreeIterator IndexToMap::Insert(TableType map, TreeType row)
        {
            if (m_treeToTable.contains(map))
            {
                m_tableToTree.remove(m_treeToTable.take(map));
            }
            if (m_tableToTree.contains(row))
            {
                m_treeToTable.remove(m_tableToTree.take(row));
            }

            m_tableToTree.insert(row, map);
            return m_treeToTable.insert(map, row);
        }

        TableIterator IndexToMap::TableBegin()
        {
            return m_tableToTree.begin();
        }

        void IndexToMap::Clear()
        {
            m_treeToTable.clear();
            m_tableToTree.clear();
        }

        AssetBrowserTreeToTableProxyModel::AssetBrowserTreeToTableProxyModel(QObject* parent)
            : QAbstractProxyModel(parent)
        {
        }

        void AssetBrowserTreeToTableProxyModel::setSourceModel(QAbstractItemModel* model)
        {
            beginResetModel();

            if (sourceModel())
            {
                disconnect(sourceModel(), nullptr, this, nullptr);
            }

            QAbstractProxyModel::setSourceModel(model);
            if (model)
            {
                connect(model, &QAbstractItemModel::rowsAboutToBeInserted, this,
                    [this](const QModelIndex& parent, int start, int end)
                    {
                        RowsAboutToBeInserted(parent, start, end);
                    });

                connect(model, &QAbstractItemModel::rowsInserted, this,
                    [this](const QModelIndex& parent, int start, int end)
                    {
                        RowsInserted(parent, start, end);
                    });

                connect(model, &QAbstractItemModel::rowsAboutToBeRemoved, this,
                    [this](const QModelIndex& parent, int start, int end)
                    {
                        RowsAboutToBeRemoved(parent, start, end);
                    });

                connect(model, &QAbstractItemModel::rowsRemoved, this,
                    [this](const QModelIndex& parent, int start)
                    {
                        RowsRemoved(parent, start);
                    });

                connect(model, &QAbstractItemModel::rowsAboutToBeMoved, this,
                    [this]()
                    {
                        LayoutChanged();
                    });

                connect(model, &QAbstractItemModel::rowsMoved, this,
                    [this](const QModelIndex& srcParent, int srcStart, [[maybe_unused]]int srcEnd, const QModelIndex& destParent, int destStart)
                    {
                        RowsMoved(srcParent, srcStart, destParent, destStart);
                    });

                connect(model, &QAbstractItemModel::modelAboutToBeReset, this,
                    [this]()
                    {
                        beginResetModel();
                    });

                connect(model, &QAbstractItemModel::modelReset, this,
                    [this]()
                    {
                        ModelReset();
                    });

                connect(model, &QAbstractItemModel::dataChanged, this,
                    [this](const QModelIndex& topLeft, const QModelIndex& bottomRight)
                    {
                        DataChanged(topLeft, bottomRight);
                    });

                connect(model, &QAbstractItemModel::layoutAboutToBeChanged, this,
                    [this]()
                    {
                        LayoutAboutToBeChanged();
                    });

                connect(model, &QAbstractItemModel::layoutChanged, this,
                    [this]()
                    {
                        LayoutChanged();
                    });

                connect(model, &QObject::destroyed, this,
                    [this]()
                    {
                        resetInternalData();
                    });
             }

            resetInternalData();
            if (model && model->hasChildren())
            {
                RefreshMap();
            }
            endResetModel();
        }

        QVariant AssetBrowserTreeToTableProxyModel::headerData(int section, Qt::Orientation orientation, int role) const
        {
            if (!sourceModel() || columnCount() <= section)
            {
                return QVariant();
            }

            return sourceModel()->headerData(section, orientation, role);
        }

        QModelIndex AssetBrowserTreeToTableProxyModel::parent([[maybe_unused]] const QModelIndex& index) const
        {
            return QModelIndex();
        }

        bool AssetBrowserTreeToTableProxyModel::hasChildren(const QModelIndex& parent) const
        {
            return !(m_map.Empty() || parent.isValid());
        }

        void AssetBrowserTreeToTableProxyModel::DataChangedAllSiblings(const QModelIndex& parent)
        {
            if (!parent.isValid())
            {
                return;
            }

            QModelIndex localParent = mapFromSource(parent);
            emit dataChanged(localParent, localParent);
            for (int i = 0; i < sourceModel()->rowCount(parent); ++i)
            {
                DataChangedAllSiblings(sourceModel()->index(i, 0, parent));
            }
        }

        void AssetBrowserTreeToTableProxyModel::RowsAboutToBeInserted(const QModelIndex& parent, int start, int end)
        {
            if (!sourceModel()->hasChildren(parent))
            {
                AZ_Assert(sourceModel()->rowCount(parent) == 0, "Row alredy has children");
                return;
            }

            int proxyStart = -1;

            const int rowCount = sourceModel()->rowCount(parent);

            if (rowCount > start)
            {
                const QModelIndex newStart = sourceModel()->index(start, 0, parent);
                proxyStart = mapFromSource(newStart).row();
            }
            else if (rowCount == 0)
            {
                proxyStart = mapFromSource(parent).row() + 1;
            }
            else
            {
                static const int column = 0;
                QModelIndex idx = sourceModel()->index(rowCount - 1, column, parent);
                while (sourceModel()->hasChildren(idx) && sourceModel()->rowCount(idx) > 0)
                {
                    idx = sourceModel()->index(sourceModel()->rowCount(idx) - 1, column, idx);
                }
                proxyStart = mapFromSource(idx).row() + 1;
            }
            const int proxyEnd = proxyStart + (end - start);

            m_insertPair = qMakePair(proxyStart, proxyEnd);
            beginInsertRows(QModelIndex(), proxyStart, proxyEnd);
        }

        void AssetBrowserTreeToTableProxyModel::RowsInserted(const QModelIndex& parent, int start, int end)
        {
            AZ_Assert(sourceModel()->index(start, 0, parent).isValid(), "Index invalid");

            const int rowCount = sourceModel()->rowCount(parent);
            const int span = end - start + 1;

            if (rowCount == span)
            {
                const QModelIndex index = mapFromSource(parent);
                if (parent.isValid())
                {
                    emit dataChanged(index, index);
                }
                m_parents.append(parent);
                ProcessParents();
                if (start > 0)
                {
                    DataChangedAllSiblings(sourceModel()->index(start - 1, 0, parent));
                }
                return;
            }

            const int proxyStart = m_insertPair.first;
            constexpr int column{ 0 };

            AZ_Assert(proxyStart >= 0, "proxyStart before zero");

            UpdateInternalIndices(proxyStart, span);

            if (rowCount == end + 1)
            {
                const QModelIndex oldIndex = sourceModel()->index(rowCount - 1 - span, column, parent);
                AZ_Assert(m_map.TreeContains(oldIndex), "Tree does not contain index");

                const QModelIndex newIndex = sourceModel()->index(rowCount - 1, column, parent);

                QModelIndex indexAbove = oldIndex;

                if (start > 0)
                {
                    while (sourceModel()->hasChildren(indexAbove))
                    {
                        indexAbove = sourceModel()->index(sourceModel()->rowCount(indexAbove) - 1, column, indexAbove);
                    }
                }

                AZ_Assert(m_map.TreeContains(indexAbove), "Index not in map");

                const int newProxyRow = m_map.TreeToTable(indexAbove) + span;

                m_map.RemoveFromTree(oldIndex);

                m_map.Insert(newIndex, newProxyRow);
            }

            for (int row = start; row <= end; ++row)
            {
                const QModelIndex idx = sourceModel()->index(row, column, parent);

                if (sourceModel()->hasChildren(idx) && sourceModel()->rowCount(idx) > 0)
                {
                    m_parents.append(idx);
                }
            }

            m_rowCount += span;

            endInsertRows();
            ProcessParents();
            if (parent.isValid())
            {
                const QModelIndex index = mapFromSource(parent);
                emit dataChanged(index, index);
            }

            if (start > 0)
            {
                DataChangedAllSiblings(sourceModel()->index(start - 1, 0, parent));
            }
        }

        void AssetBrowserTreeToTableProxyModel::RefreshMap()
        {
            m_rowCount = 0;
            m_map.Clear();
            m_parents.clear();

            m_parents.append(QModelIndex());

            m_processing = true;
            while (!m_parents.isEmpty())
            {
                ProcessParents();
            }
            m_processing = false;
        }

        int AssetBrowserTreeToTableProxyModel::columnCount(const QModelIndex& parent) const
        {
            if (parent.isValid() || !sourceModel())
            {
                return 0;
            }

            return sourceModel()->columnCount();
        }

        void AssetBrowserTreeToTableProxyModel::UpdateInternalIndices(int start, int offset)
        {
            QHash<int, QPersistentModelIndex> updates;

            const TableIterator end = m_map.TableEnd();

            for (TableIterator it = m_map.TableLowerBound(start); it != end; ++it)
            {
                updates.insert(it.key() + offset, *it);
            }
              
            const QHash<int, QPersistentModelIndex>::const_iterator end2 = updates.constEnd();

            for (QHash<int, QPersistentModelIndex>::const_iterator it = updates.constBegin(); it != end2; ++it)
            {
                m_map.Insert(it.value(), it.key());
            }
        }

        void AssetBrowserTreeToTableProxyModel::RowsAboutToBeRemoved(const QModelIndex& parent, int start, int end)
        {
            const int proxyStart = mapFromSource(sourceModel()->index(start, 0, parent)).row();

            constexpr int column{ 0 };
            QModelIndex idx = sourceModel()->index(end, column, parent);
            while (sourceModel()->hasChildren(idx) && sourceModel()->rowCount(idx) > 0)
            {
                idx = sourceModel()->index(sourceModel()->rowCount(idx) - 1, column, idx);
            }
            const int proxyEnd = mapFromSource(idx).row();

            m_removePair = qMakePair(proxyStart, proxyEnd);

            beginRemoveRows(QModelIndex(), proxyStart, proxyEnd);
        }

        QModelIndex AssetBrowserTreeToTableProxyModel::GetFirstDeepest(QAbstractItemModel* model, const QModelIndex& parent, int& count)
        {
            constexpr int column{ 0 };
            for (int row = 0; row < model->rowCount(parent); ++row)
            {
                count++;
                const QModelIndex child = model->index(row, column, parent);
                AZ_Assert(child.isValid(), "Child is invalid");
                if (model->hasChildren(child))
                {
                    return GetFirstDeepest(model, child, count);
                }
            }
            return model->index(model->rowCount(parent) - 1, column, parent);
        }


        void AssetBrowserTreeToTableProxyModel::RowsRemoved(const QModelIndex& parent, int start)
        {
            const int rowCount = sourceModel()->rowCount(parent);

            const int proxyStart = m_removePair.first;
            const int proxyEnd = m_removePair.second;

            const int difference = proxyEnd - proxyStart + 1;
            const TableIterator endIt = m_map.TableUpperBound(proxyEnd);

            if (endIt != m_map.TableEnd())
            {
                for (TableIterator it = m_map.TableLowerBound(proxyStart); it != endIt;)
                {
                    it = m_map.EraseFromTable(it);
                }
            }
            else
            {
                for (TableIterator it = m_map.TableLowerBound(proxyStart); it != m_map.TableUpperBound(proxyEnd);)
                {
                    it = m_map.EraseFromTable(it);
                }
            }

            m_removePair = qMakePair(-1, -1);
            m_rowCount -= difference;

            UpdateInternalIndices(proxyStart, -1 * difference);

            if (rowCount != start || rowCount == 0)
            {
                endRemoveRows();
                if (parent.isValid())
                {
                    const QModelIndex index = mapFromSource(parent);
                    emit dataChanged(index, index);
                }
                if (start > 0)
                {
                    DataChangedAllSiblings(sourceModel()->index(start - 1, 0, parent));
                }
                return;
            }

            constexpr int column = 0;
            const QModelIndex newEnd = sourceModel()->index(rowCount - 1, column, parent);
            if (m_map.Empty())
            {
                m_map.Insert(newEnd, newEnd.row());
                endRemoveRows();
                if (start > 0)
                {
                    DataChangedAllSiblings(sourceModel()->index(start - 1, 0, parent));
                }
                return;
            }
            if (sourceModel()->hasChildren(newEnd))
            {
                int count = 0;
                const QModelIndex firstDeepest = GetFirstDeepest(sourceModel(), newEnd, count);
                const int firstDeepestProxy = m_map.TreeToTable(firstDeepest);

                m_map.Insert(newEnd, firstDeepestProxy - count);
                endRemoveRows();
                if (start > 0)
                {
                    DataChangedAllSiblings(sourceModel()->index(start - 1, 0, parent));
                }
                return;
            }
            TableIterator lowerBound = m_map.TableLowerBound(proxyStart);
            if (lowerBound == m_map.TableEnd())
            {
                int proxyRow = AZStd::prev(lowerBound).key();

                for (int row = newEnd.row(); row >= 0; --row)
                {
                    const QModelIndex newEndSibling = sourceModel()->index(row, column, parent);
                    if (!sourceModel()->hasChildren(newEndSibling))
                    {
                        ++proxyRow;
                    }
                    else
                    {
                        break;
                    }
                }
                m_map.Insert(newEnd, proxyRow);
                endRemoveRows();
                if (start > 0)
                {
                    DataChangedAllSiblings(sourceModel()->index(start - 1, 0, parent));
                }
                return;
            }
            else if (lowerBound == m_map.TableBegin())
            {
                int proxyRow = rowCount - 1;
                QModelIndex trackedParent = parent;
                while (trackedParent.isValid())
                {
                    proxyRow += (trackedParent.row() + 1);
                    trackedParent = trackedParent.parent();
                }
                m_map.Insert(newEnd, proxyRow);
                endRemoveRows();
                if (start > 0)
                {
                    DataChangedAllSiblings(sourceModel()->index(start - 1, 0, parent));
                }
                return;
            }
            const TableIterator boundAbove = AZStd::prev(lowerBound);

            QList<QModelIndex> targetParents;
            targetParents.push_back(parent);
            QModelIndex target = parent;
            int count = 0;
            while (target.isValid())
            {
                if (target == boundAbove.value())
                {
                    m_map.Insert(newEnd, count + boundAbove.key() + newEnd.row() + 1);
                    endRemoveRows();
                    if (start > 0)
                    {
                        DataChangedAllSiblings(sourceModel()->index(start - 1, 0, parent));
                    }
                    return;
                }
                count += (target.row() + 1);
                target = target.parent();
                if (target.isValid())
                {
                    targetParents.push_back(target);
                }
            }

            QModelIndex boundParent = boundAbove.value().parent();
            QModelIndex prevParent = boundParent;
            while (boundParent.isValid())
            {
                prevParent = boundParent;
                boundParent = boundParent.parent();

                if (targetParents.contains(prevParent))
                {
                    break;
                }

                if (!m_map.TreeContains(prevParent))
                {
                    break;
                }

                if (m_map.TreeToTable(prevParent) > boundAbove.key())
                {
                    break;
                }
            }

            QModelIndex trackedParent = parent;

            int proxyRow = boundAbove.key();
            proxyRow -= prevParent.row();
            while (trackedParent != boundParent)
            {
                proxyRow += (trackedParent.row() + 1);
                trackedParent = trackedParent.parent();
            }
            m_map.Insert(newEnd, proxyRow + newEnd.row());
            endRemoveRows();

            if (start > 0)
            {
                DataChangedAllSiblings(sourceModel()->index(start - 1, 0, parent));
            }
        }

        void AssetBrowserTreeToTableProxyModel::RowsMoved(
            const QModelIndex& srcParent, int srcStart, const QModelIndex& destParent, int destStart)
        {
            LayoutChanged();

            const QModelIndex index1 = mapFromSource(srcParent);
            const QModelIndex index2 = mapFromSource(destParent);
            const QModelIndex lastIndex1 = mapFromSource(sourceModel()->index(sourceModel()->rowCount(srcParent) - 1, 0, srcParent));
            const QModelIndex lastIndex2 = mapFromSource(sourceModel()->index(sourceModel()->rowCount(destParent) - 1, 0, destParent));
            emit dataChanged(index1, lastIndex1);
            if (index1 != index2)
            {
                emit dataChanged(index2, lastIndex2);
            }

            if (srcStart > 0)
            {
                DataChangedAllSiblings(sourceModel()->index(srcStart - 1, 0, srcParent));
            }
            if (destStart > 0)
            {
                DataChangedAllSiblings(sourceModel()->index(destStart - 1, 0, destParent));
            }
        }

        void AssetBrowserTreeToTableProxyModel::ModelReset()
        {
            resetInternalData();
            if (sourceModel()->hasChildren() && sourceModel()->rowCount() > 0)
            {
                m_parents.append(QModelIndex());
                ProcessParents();
            }
            endResetModel();
        }

        void AssetBrowserTreeToTableProxyModel::LayoutAboutToBeChanged()
        {
            if (m_map.Empty())
            {
                return;
            }

            emit layoutAboutToBeChanged();

            QPersistentModelIndex srcPersistentIndex;
            const auto lst = persistentIndexList();
            for (const QModelIndex& proxyPersistentIndex : lst)
            {
                m_proxyIndices << proxyPersistentIndex;
                srcPersistentIndex = mapToSource(proxyPersistentIndex);
                m_changePersistentIndexes << srcPersistentIndex;
            }
        }

        void AssetBrowserTreeToTableProxyModel::LayoutChanged()
        {
            if (m_map.Empty())
            {
                return;
            }

            m_rowCount = 0;

            RefreshMap();

            for (int i = 0; i < m_proxyIndices.size(); ++i)
            {
                changePersistentIndex(m_proxyIndices.at(i), mapFromSource(m_changePersistentIndexes.at(i)));
            }

            m_changePersistentIndexes.clear();
            m_proxyIndices.clear();

            emit layoutChanged();
        }

        void AssetBrowserTreeToTableProxyModel::DataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
        {
            if (!topLeft.isValid() || !bottomRight.isValid())
            {
                return;
            }
            const int topRow = topLeft.row();
            const int bottomRow = bottomRight.row();

            for (int i = topRow; i <= bottomRow; ++i)
            {
                const QModelIndex sourceTopLeft = sourceModel()->index(i, topLeft.column(), topLeft.parent());

                const QModelIndex proxyTopLeft = mapFromSource(sourceTopLeft);
                const QModelIndex sourceBottomRight = sourceModel()->index(i, bottomRight.column(), bottomRight.parent());
                const QModelIndex proxyBottomRight = mapFromSource(sourceBottomRight);
                emit dataChanged(proxyTopLeft, proxyBottomRight);
            }
        }

        QModelIndex AssetBrowserTreeToTableProxyModel::mapFromSource(const QModelIndex& sourceIndex) const
        {
            if (!sourceModel())
            {
                return QModelIndex();
            }

            if (m_map.Empty())
            {
                return QModelIndex();
            }

            {
                const ConstTableIterator end = m_map.TableConstEnd();
                const QModelIndex sourceParent = sourceIndex.parent();
                ConstTableIterator result = end;

                for (ConstTableIterator it = m_map.TableConstBegin(); it != end; ++it)
                {
                    QModelIndex index = it.value();
                    bool found_block = false;
                    while (index.isValid())
                    {
                        const QModelIndex ancestor = index.parent();
                        if (ancestor == sourceParent && index.row() >= sourceIndex.row())
                        {
                            found_block = true;
                            if (result == end || it.key() < result.key())
                            {
                                result = it;
                                break;
                            }
                        }
                        index = ancestor;
                    }
                    if (found_block && !index.isValid())
                    {
                        break;
                    }
                }
                const QModelIndex sourceLastChild = result.value();
                int proxyRow = result.key();
                QModelIndex index = sourceLastChild;
                while (index.isValid())
                {
                    const QModelIndex ancestor = index.parent();
                    if (ancestor == sourceParent)
                    {
                        return createIndex(proxyRow - (index.row() - sourceIndex.row()), sourceIndex.column());
                    }
                    proxyRow -= (index.row() + 1);
                    index = ancestor;
                }
                AZ_Assert(0, "Couldn't find valid mapping.");
                return QModelIndex();
            }
        }

        QModelIndex AssetBrowserTreeToTableProxyModel::mapToSource(const QModelIndex& proxyIndex) const
        {
            if (m_map.Empty() || !proxyIndex.isValid() || !sourceModel())
            {
                return QModelIndex();
            }

            const ConstTableIterator result = m_map.TableLowerBound(proxyIndex.row());

            const int proxyLastRow = result.key();
            const QModelIndex sourceLastChild = result.value();

            int verticalDistance = proxyLastRow - proxyIndex.row();

            QModelIndex ancestor = sourceLastChild;
            while (ancestor.isValid())
            {
                const int ancestorRow = ancestor.row();
                if (verticalDistance <= ancestorRow)
                {
                    return ancestor.sibling(ancestorRow - verticalDistance, proxyIndex.column());
                }
                verticalDistance -= (ancestorRow + 1);
                ancestor = ancestor.parent();
            }
            AZ_Assert(0, "Could't find target row.");
            return QModelIndex();
        }

        QModelIndex AssetBrowserTreeToTableProxyModel::index(int row, int column, const QModelIndex& parent) const
        {
            if (parent.isValid())
            {
                return QModelIndex();
            }

            if (!hasIndex(row, column, parent))
            {
                return QModelIndex();
            }

            return createIndex(row, column);
        }

        Qt::ItemFlags AssetBrowserTreeToTableProxyModel::flags(const QModelIndex& index) const
        {
            if (!index.isValid() || !sourceModel())
            {
                return QAbstractProxyModel::flags(index);
            }

            const QModelIndex srcIndex = mapToSource(index);
            return sourceModel()->flags(srcIndex);
        }

        int AssetBrowserTreeToTableProxyModel::rowCount(const QModelIndex& parent) const
        {
            if (m_parents.contains(parent) || parent.isValid() || !sourceModel())
            {
                return 0;
            }

            if (m_map.Empty() && sourceModel()->hasChildren())
            {
                const_cast<AssetBrowserTreeToTableProxyModel*>(this)->RefreshMap();
            }
            return m_rowCount;
        }

        QVariant AssetBrowserTreeToTableProxyModel::data(const QModelIndex& index, int role) const
        {
            if (!sourceModel())
            {
                return QVariant();
            }

            if (!index.isValid())
            {
                return sourceModel()->data(index, role);
            }

            QModelIndex sourceIndex = mapToSource(index);
            {
                return sourceIndex.data(role);
            }
        }
  
        void AssetBrowserTreeToTableProxyModel::ProcessParents()
        {
            const QPersistentModelIndexList::iterator begin = m_parents.begin();
            const QPersistentModelIndexList::iterator end = m_parents.end();

            QPersistentModelIndexList newPendingParents;

            for (QPersistentModelIndexList::iterator it = begin; it != end && it != m_parents.end();)
            {
                const QModelIndex sourceParent = *it;
                if (!sourceParent.isValid() && m_rowCount > 0)
                {
                    it = m_parents.erase(it);
                    continue;
                }

                const int rowCount = sourceModel()->rowCount(sourceParent);

                if (rowCount == 0)
                {
                    it = m_parents.erase(it);
                    continue;
                }
                const QPersistentModelIndex sourceIndex = sourceModel()->index(rowCount - 1, 0, sourceParent);
                const QModelIndex proxyParent = mapFromSource(sourceParent);
                const int proxyEndRow = proxyParent.row() + rowCount;
                const int proxyStartRow = proxyEndRow - rowCount + 1;

                if (!m_processing)
                {
                    beginInsertRows(QModelIndex(), proxyStartRow, proxyEndRow);
                }

                UpdateInternalIndices(proxyStartRow, rowCount);
                m_map.Insert(sourceIndex, proxyEndRow);
                it = m_parents.erase(it);
                m_rowCount += rowCount;

                if (!m_processing)
                {
                    endInsertRows();
                }

                for (int sourceRow = 0; sourceRow < rowCount; ++sourceRow)
                {
                    static const int column = 0;
                    const QModelIndex child = sourceModel()->index(sourceRow, column, sourceParent);
                    AZ_Assert(child.isValid(), "Child isn't valid");

                    if (sourceModel()->hasChildren(child) && sourceModel()->rowCount(child) > 0)
                    {
                        newPendingParents.append(child);
                    }
                }
            }
            m_parents += newPendingParents;
            if (!m_parents.isEmpty())
            {
                ProcessParents();
            }
        }

    } // namespace AssetBrowser
} // namespace AzToolsFramework

#include "AssetBrowser/moc_AssetBrowserTreeToTableProxyModel.cpp"
