/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Trace.h>
#include <AzCore/std/functional.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserTreeToTableProxyModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        AssetBrowserTreeToTableProxyModel::AssetBrowserTreeToTableProxyModel(QObject* parent)
            : QAbstractProxyModel(parent)
        {
        }

        void AssetBrowserTreeToTableProxyModel::ConnectSignals()
        {
            if (this->sourceModel())
            {
                connect(
                    this->sourceModel(), &QAbstractItemModel::dataChanged, this, &AssetBrowserTreeToTableProxyModel::onSourceDataChanged);
                connect(this->sourceModel(), &QAbstractItemModel::layoutChanged, this, &AssetBrowserTreeToTableProxyModel::FlattenTree);
                connect(this->sourceModel(), &QAbstractItemModel::modelReset, this, &AssetBrowserTreeToTableProxyModel::FlattenTree);
                connect(this->sourceModel(), &QAbstractItemModel::rowsInserted, this, &AssetBrowserTreeToTableProxyModel::FlattenTree);
                connect(this->sourceModel(), &QAbstractItemModel::rowsRemoved, this, &AssetBrowserTreeToTableProxyModel::FlattenTree);
            }
        }

        void AssetBrowserTreeToTableProxyModel::DisconnectSignals()
        {
            if (this->sourceModel())
            {
                disconnect(this->sourceModel(), nullptr, this, nullptr);
            }
        }

        QModelIndex AssetBrowserTreeToTableProxyModel::mapToSource(const QModelIndex& proxyIndex) const
        {
            if (!proxyIndex.isValid() || !flattenedData.contains(proxyIndex.row()))
            {
                return QModelIndex();
            }
            return flattenedData.value(proxyIndex.row());
        }

        QModelIndex AssetBrowserTreeToTableProxyModel::mapFromSource(const QModelIndex& sourceIndex) const
        {
            if (!sourceIndex.isValid())
            {
                return QModelIndex();
            }

            int proxyRow = flattenedData.key(sourceIndex, -1);
            return (proxyRow != -1) ? createIndex(proxyRow, sourceIndex.column()) : QModelIndex();
        }

        int AssetBrowserTreeToTableProxyModel::rowCount(const QModelIndex& parent) const
        {
            return parent.isValid() ? 0 : flattenedData.size();
        }

        int AssetBrowserTreeToTableProxyModel::columnCount(const QModelIndex& parent) const
        {
            if (parent.isValid() || !sourceModel())
            {
                return 0;
            }
            return sourceModel()->columnCount();
        }

        QVariant AssetBrowserTreeToTableProxyModel::data(const QModelIndex& index, int role) const
        {
            if (!index.isValid())
            {
                return QVariant();
            }

            QModelIndex sourceIndex = mapToSource(index);
            return sourceModel()->data(sourceIndex, role);
        }

        QModelIndex AssetBrowserTreeToTableProxyModel::index(int row, int column, const QModelIndex& parent) const
        {
            if (!hasIndex(row, column, parent))
            {
                return QModelIndex();
            }

            return createIndex(row, column);
        }

        QModelIndex AssetBrowserTreeToTableProxyModel::parent(const QModelIndex& child) const
        {
            Q_UNUSED(child);
            return QModelIndex(); // Flat model, no hierarchy
        }

        void AssetBrowserTreeToTableProxyModel::onSourceDataChanged(
            const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
        {
            Q_UNUSED(topLeft);
            Q_UNUSED(bottomRight);
            // we dont' need to consider refiltering or flattening if the role which changed is just the thumbnail icon.
            if (roles.contains(Qt::DecorationRole))
            {
                return;
            }
            FlattenTree();
        }


        void AssetBrowserTreeToTableProxyModel::FlattenTree()
        {
            beginResetModel();
            flattenedData.clear();
            if (sourceModel())
            {
                int row = 0;
                TraverseTree(QModelIndex(), row);
            }
            endResetModel();
        }


        void AssetBrowserTreeToTableProxyModel::TraverseTree(const QModelIndex& index, int& row)
        {
            int rowCount = sourceModel()->rowCount(index);
            for (int i = 0; i < rowCount; ++i)
            {
                QModelIndex childIndex = sourceModel()->index(i, 0, index);
                if (childIndex.isValid())
                {
                    // Map childIndex through the chain of proxies to the original tree model index
                    QModelIndex treeModelIndex = mapToTreeModel(childIndex);

                    if (treeModelIndex.isValid())
                    {
                        // Access the internalPointer of the tree model index
                        void* internalPointer = treeModelIndex.internalPointer();
                        auto entry = static_cast<AssetBrowserEntry*>(internalPointer);

                        if (entry->GetEntryType() != AssetBrowserEntry::AssetEntryType::Folder)
                        {
                            flattenedData.insert(row, childIndex);
                            ++row;
                        }
                        TraverseTree(childIndex, row);
                    }
                }
            }
        }

        QModelIndex AssetBrowserTreeToTableProxyModel::mapToTreeModel(const QModelIndex& proxyIndex) const
        {
            QModelIndex current = proxyIndex;
            QAbstractItemModel* model = sourceModel();
            while (QAbstractProxyModel* proxyModel = qobject_cast<QAbstractProxyModel*>(model))
            {
                current = proxyModel->mapToSource(current);
                model = proxyModel->sourceModel();
            }
            return current;
        }

    } // namespace AssetBrowser
} // namespace AzToolsFramework

#include "AssetBrowser/moc_AssetBrowserTreeToTableProxyModel.cpp"
