/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AssetBrowser/AssetBrowserFilterModel.h>
#include <AssetBrowser/AssetBrowserTableModel.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        AssetBrowserTableModel::AssetBrowserTableModel(QObject* parent /* = nullptr */)
            : QSortFilterProxyModel(parent)
        {
            setDynamicSortFilter(false);
        }

        void AssetBrowserTableModel::setSourceModel(QAbstractItemModel* sourceModel)
        {
            m_filterModel = qobject_cast<AssetBrowserFilterModel*>(sourceModel);
            AZ_Assert(
                m_filterModel,
                "Error in AssetBrowserTableModel initialization, class expects source model to be an AssetBrowserFilterModel.");
            QSortFilterProxyModel::setSourceModel(sourceModel);
        }

        QModelIndex AssetBrowserTableModel::mapToSource(const QModelIndex& proxyIndex) const
        {
            Q_ASSERT(!proxyIndex.isValid() || proxyIndex.model() == this);
            if (!proxyIndex.isValid())
            {
                return QModelIndex();
            }
            return m_indexMap[proxyIndex.row()];
        }

        QVariant AssetBrowserTableModel::headerData(int section, Qt::Orientation orientation, int role) const
        {
            if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
            {
                return tr(AssetBrowserEntry::m_columnNames[section]);
            }
            return QSortFilterProxyModel::headerData(section, orientation, role);
        }

        QVariant AssetBrowserTableModel::data(const QModelIndex& index, int role) const
        {
            auto sourceIndex = mapToSource(index);
            if (!sourceIndex.isValid())
            {
                return QVariant();
            }

            AssetBrowserEntry* entry = GetAssetEntry(sourceIndex);
            if (entry == nullptr)
            {
                AZ_Assert(false, "AssetBrowserTableModel - QModelIndex does not reference an AssetEntry. Source model is not valid.");
                return QVariant();
            }

            return sourceIndex.data(role);
        }

        QModelIndex AssetBrowserTableModel::parent([[maybe_unused]] const QModelIndex& child) const
        {
            return QModelIndex();
        }

        QModelIndex AssetBrowserTableModel::sibling(
            [[maybe_unused]] int row, [[maybe_unused]] int column, [[maybe_unused]] const QModelIndex& idx) const
        {
            return QModelIndex();
        }

        QModelIndex AssetBrowserTableModel::index(int row, int column, const QModelIndex& parent) const
        {
            return parent.isValid() ? QModelIndex() : createIndex(row, column, m_indexMap[row].internalPointer());
        }

        int AssetBrowserTableModel::rowCount(const QModelIndex& parent) const
        {
            return !parent.isValid() ? m_indexMap.size() : 0;
        }

        int AssetBrowserTableModel::BuildTableModelMap(
            const QAbstractItemModel* model, const QModelIndex& parent /*= QModelIndex()*/, int row /*= 0*/)
        {
            int rows = model ? model->rowCount(parent) : 0;
            for (int i = 0; i < rows; ++i)
            {
                QModelIndex index = model->index(i, 0, parent);
                AssetBrowserEntry* entry = GetAssetEntry(m_filterModel->mapToSource(index));
                //We only wanna see the source assets.
                if (entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Source)
                {
                    beginInsertRows(parent, row, row);
                    m_indexMap[row] = index;
                    endInsertRows();

                    Q_EMIT dataChanged(index, index);
                    ++row;
                }

                if (model->hasChildren(index))
                {
                    row = BuildTableModelMap(model, index, row);
                }
            }
            return row;
        }

        AssetBrowserEntry* AssetBrowserTableModel::GetAssetEntry(QModelIndex index) const
        {
            if (index.isValid())
            {
                return static_cast<AssetBrowserEntry*>(index.internalPointer());
            }
            else
            {
                AZ_Error("AssetBrowser", false, "Invalid Source Index provided to GetAssetEntry.");
                return nullptr;
            }
        }

        void AssetBrowserTableModel::UpdateTableModelMaps()
        {
            emit layoutAboutToBeChanged();
            if (!m_indexMap.isEmpty())
            {
                beginRemoveRows(m_indexMap.first(), m_indexMap.first().row(), m_indexMap.last().row());
                m_indexMap.clear();
                endRemoveRows();
            }
            BuildTableModelMap(sourceModel());
            emit layoutChanged();
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework
#include "AssetBrowser/moc_AssetBrowserTableModel.cpp"
