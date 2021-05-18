/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */
#include <AssetBrowser/AssetBrowserTableModel.h>
#include <AssetBrowser/AssetBrowserFilterModel.h>
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
            AZ_Assert(m_filterModel, "Expecting AssetBrowserFilterModel");
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
                auto columnRole = aznumeric_cast<AssetBrowserEntry::Column>(role);
                switch (columnRole)
                {
                case AssetBrowserEntry::Column::Name:
                    return QString("Name");
                case AssetBrowserEntry::Column::Path:
                    return QString("Path");
                default:
                    return QString::number(section);
                }
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
                AZ_Assert(false, "ERROR - index internal pointer not pointing to an AssetEntry. Tree provided by the AssetBrowser invalid?");
                return Qt::PartiallyChecked;
            }

            return sourceIndex.data(role);
        }

        QModelIndex AssetBrowserTableModel::index(int row, int column, const QModelIndex& parent) const
        {
            return parent.isValid() ? QModelIndex() : createIndex(row, column, m_indexMap[row].internalPointer());
        }

        QModelIndex AssetBrowserTableModel::parent([[maybe_unused]] const QModelIndex& child) const
        {
            return QModelIndex();
        }

        int AssetBrowserTableModel::rowCount(const QModelIndex& parent) const
        {
            return !parent.isValid() ? m_indexMap.size() : 0;
        }

        int AssetBrowserTableModel::BuildTableModelMap(const QAbstractItemModel* model, const QModelIndex& parent /*= QModelIndex()*/, int row /*= 0*/)
        {
            int rows = model ? model->rowCount(parent) : 0;
            for (int i = 0; i < rows; ++i)
            {
                QModelIndex index = model->index(i, 0, parent);
                if (!model->hasChildren(index))
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
