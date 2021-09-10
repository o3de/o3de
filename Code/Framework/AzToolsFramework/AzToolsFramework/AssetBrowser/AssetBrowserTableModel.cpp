/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        }

        void AssetBrowserTableModel::setSourceModel(QAbstractItemModel* sourceModel)
        {
            m_filterModel = qobject_cast<AssetBrowserFilterModel*>(sourceModel);
            AZ_Assert(
                m_filterModel,
                "Error in AssetBrowserTableModel initialization, class expects source model to be an AssetBrowserFilterModel.");
            connect(sourceModel, &QAbstractItemModel::rowsInserted, this, &AssetBrowserTableModel::UpdateTableModelMaps);
            connect(sourceModel, &QAbstractItemModel::rowsRemoved, this, &AssetBrowserTableModel::UpdateTableModelMaps);
            connect(sourceModel, &QAbstractItemModel::modelAboutToBeReset, this, &AssetBrowserTableModel::beginResetModel);
            connect(
                sourceModel, &QAbstractItemModel::modelReset, this,
                [this]()
                {
                    {
                        QSignalBlocker sb(this);
                        UpdateTableModelMaps();
                    }
                    endResetModel();
                });
            connect(sourceModel, &QAbstractItemModel::layoutChanged, this, &AssetBrowserTableModel::UpdateTableModelMaps);
            connect(sourceModel, &QAbstractItemModel::dataChanged, this, &AssetBrowserTableModel::SourceDataChanged);


            QSortFilterProxyModel::setSourceModel(sourceModel);
        }

        QModelIndex AssetBrowserTableModel::mapToSource(const QModelIndex& proxyIndex) const
        {
            Q_ASSERT(!proxyIndex.isValid() || proxyIndex.model() != this);
            if (!proxyIndex.isValid() || !m_indexMap.contains(proxyIndex.row()))
            {
                return QModelIndex();
            }
            return m_indexMap[proxyIndex.row()];
        }

        QModelIndex AssetBrowserTableModel::mapFromSource(const QModelIndex& sourceIndex) const
        {
            Q_ASSERT(!sourceIndex.isValid() || sourceIndex.model() == sourceModel());
            if (!sourceIndex.isValid() || !m_rowMap.contains(sourceIndex))
            {
                return QModelIndex();
            }

            return createIndex(m_rowMap[sourceIndex], sourceIndex.column());
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
            Q_ASSERT(index.isValid() && index.model() == this);
            return sourceModel()->data(mapToSource(index), role);
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

        void AssetBrowserTableModel::SourceDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
        {
            for (int row = topLeft.row(); row <= bottomRight.row(); ++row)
            {
                if (!m_indexMap.contains(row))
                {
                    UpdateTableModelMaps();
                    return;
                }
            }
        }

        QModelIndex AssetBrowserTableModel::index(int row, int column, const QModelIndex& parent) const
        {
            Q_ASSERT(!parent.isValid());

            return parent.isValid() ? QModelIndex() : createIndex(row, column, m_indexMap[row].internalPointer());
        }

        int AssetBrowserTableModel::rowCount(const QModelIndex& parent) const
        {
            return !parent.isValid() ? m_indexMap.size() : sourceModel()->rowCount(parent);
        }

        int AssetBrowserTableModel::BuildTableModelMap(
            const QAbstractItemModel* model, const QModelIndex& parent /*= QModelIndex()*/, int row /*= 0*/)
        {
            int rows = model ? model->rowCount(parent) : 0;

            if (parent == QModelIndex())
            {
                m_displayedItemsCounter = 0;
            }

            for (int currentRow = 0; currentRow < rows; ++currentRow)
            {
                if (m_displayedItemsCounter < m_numberOfItemsDisplayed)
                {
                    QModelIndex index = model->index(currentRow, 0, parent);
                    AssetBrowserEntry* entry = GetAssetEntry(m_filterModel->mapToSource(index));
                    // We only want to see source and product assets.
                    if (entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Source ||
                        entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Product)
                    {
                        m_indexMap[row] = index;
                        m_rowMap[index] = row;
                        ++row;
                        ++m_displayedItemsCounter;
                    }

                    if (model->hasChildren(index))
                    {
                        row = BuildTableModelMap(model, index, row);
                    }
                }
                else
                {
                    break;
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
            m_indexMap.clear();
            m_rowMap.clear();

            AzToolsFramework::EditorSettingsAPIBus::BroadcastResult(
                m_numberOfItemsDisplayed, &AzToolsFramework::EditorSettingsAPIBus::Handler::GetMaxNumberOfItemsShownInSearchView);

            BuildTableModelMap(sourceModel());
            emit layoutChanged();
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework
#include "AssetBrowser/moc_AssetBrowserTableModel.cpp"
