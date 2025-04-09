/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AssetBrowser/AssetBrowserFilterModel.h>
#include <AssetBrowser/AssetBrowserListModel.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntryUtils.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserViewUtils.h>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        AssetBrowserListModel::AssetBrowserListModel(QObject* parent /* = nullptr */)
            : QSortFilterProxyModel(parent)
        {
        }

        void AssetBrowserListModel::setSourceModel(QAbstractItemModel* sourceModel)
        {
            m_filterModel = qobject_cast<AssetBrowserFilterModel*>(sourceModel);
            AZ_Assert(
                m_filterModel,
                "Error in AssetBrowserListModel initialization, class expects source model to be an AssetBrowserFilterModel.");

            QSortFilterProxyModel::setSourceModel(sourceModel);

            connect(m_filterModel, &QAbstractItemModel::rowsInserted, this, &AssetBrowserListModel::StartUpdateModelMapTimer);
            connect(m_filterModel, &QAbstractItemModel::rowsRemoved, this, &AssetBrowserListModel::StartUpdateModelMapTimer);
            connect(m_filterModel, &QAbstractItemModel::layoutChanged, this, &AssetBrowserListModel::StartUpdateModelMapTimer);
            connect(m_filterModel, &AssetBrowserFilterModel::filterChanged, this, &AssetBrowserListModel::UpdateListModelMaps);
            connect(m_filterModel, &QAbstractItemModel::dataChanged, this, &AssetBrowserListModel::SourceDataChanged);
        }

        QModelIndex AssetBrowserListModel::mapToSource(const QModelIndex& proxyIndex) const
        {
            Q_ASSERT(!proxyIndex.isValid() || proxyIndex.model() == this);
            if (!proxyIndex.isValid() || !m_indexMap.contains(proxyIndex.row()))
            {
                return QModelIndex();
            }
            return m_indexMap[proxyIndex.row()];
        }

        QModelIndex AssetBrowserListModel::mapFromSource(const QModelIndex& sourceIndex) const
        {
            Q_ASSERT(!sourceIndex.isValid() || sourceIndex.model() == sourceModel());
            if (!sourceIndex.isValid() || !m_rowMap.contains(sourceIndex))
            {
                return QModelIndex();
            }

            return createIndex(m_rowMap[sourceIndex], sourceIndex.column());
        }

        bool AssetBrowserListModel::filterAcceptsRow(int source_row, [[maybe_unused]] const QModelIndex& source_parent) const
        {
            return m_indexMap.contains(source_row);
        }

        QVariant AssetBrowserListModel::headerData(int section, Qt::Orientation orientation, int role) const
        {
            if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
            {
                return tr(AssetBrowserEntry::m_columnNames[section]);
            }
            return QSortFilterProxyModel::headerData(section, orientation, role);
        }

        QVariant AssetBrowserListModel::data(const QModelIndex& index, int role) const
        {
            Q_ASSERT(index.isValid() && index.model() == this);
            return sourceModel()->data(mapToSource(index), role);
        }

        QModelIndex AssetBrowserListModel::parent([[maybe_unused]] const QModelIndex& child) const
        {
            return QModelIndex();
        }

        QModelIndex AssetBrowserListModel::sibling(
            [[maybe_unused]] int row, [[maybe_unused]] int column, [[maybe_unused]] const QModelIndex& idx) const
        {
            return QModelIndex();
        }

        void AssetBrowserListModel::SourceDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
        {
            for (int row = topLeft.row(); row <= bottomRight.row(); ++row)
            {
                if (!m_indexMap.contains(row))
                {
                    UpdateListModelMaps();
                    return;
                }
            }
        }

        QModelIndex AssetBrowserListModel::index(int row, int column, const QModelIndex& parent) const
        {
            Q_ASSERT(!parent.isValid());

            return parent.isValid() ? QModelIndex() : createIndex(row, column, m_indexMap[row].internalPointer());
        }

        int AssetBrowserListModel::rowCount(const QModelIndex& parent) const
        {
            return !parent.isValid() ? m_indexMap.size() : 0;
        }

        void AssetBrowserListModel::timerEvent([[maybe_unused]] QTimerEvent* event)
        {
            killTimer(m_updateModelMapTimerId);
            m_updateModelMapTimerId = 0;
            UpdateListModelMaps();
        }

        void AssetBrowserListModel::StartUpdateModelMapTimer()
        {
            constexpr int ModelRefreshWaitTimeMS = 250;
            if (m_updateModelMapTimerId > 0)
            {
                killTimer(m_updateModelMapTimerId);
            }
            m_updateModelMapTimerId = startTimer(ModelRefreshWaitTimeMS);
        }

        int AssetBrowserListModel::BuildListModelMap(
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

                        // We only want to increase the displayed counter if it is a parent (Source)
                        // so we don't cut children entries.
                        if (entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Source)
                        {
                            ++m_displayedItemsCounter;
                        }
                    }

                    if (model->hasChildren(index))
                    {
                        row = BuildListModelMap(model, index, row);
                    }
                }
                else
                {
                    break;
                }
            }
            return row;
        }

        AssetBrowserEntry* AssetBrowserListModel::GetAssetEntry(QModelIndex index) const
        {
            if (!index.isValid())
            {
                AZ_Error("AssetBrowser", false, "Invalid Source Index provided to GetAssetEntry.");
                return nullptr;
            }
            return static_cast<AssetBrowserEntry*>(index.internalPointer());
        }

        void AssetBrowserListModel::UpdateListModelMaps()
        {
            beginResetModel();
            emit layoutAboutToBeChanged();

            if (!m_indexMap.isEmpty() || !m_rowMap.isEmpty())
            {
                m_indexMap.clear();
                m_rowMap.clear();
            }
            AzToolsFramework::EditorSettingsAPIBus::BroadcastResult(
                m_numberOfItemsDisplayed, &AzToolsFramework::EditorSettingsAPIBus::Handler::GetMaxNumberOfItemsShownInSearchView);
            BuildListModelMap(sourceModel());
            emit layoutChanged();
            endResetModel();
        }
        
        Qt::DropActions AssetBrowserListModel::supportedDropActions() const
        {
            return Qt::CopyAction | Qt::MoveAction;
        }

        Qt::ItemFlags AssetBrowserListModel::flags(const QModelIndex& index) const
        {
            Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index) | Qt::ItemIsEditable;

            if (index.isValid())
            {
                return AssetBrowserViewUtils::GetAssetBrowserEntryCommonItemFlags(
                    mapToSource(index).data(AssetBrowserModel::Roles::EntryRole).value<const AssetBrowserEntry*>(), defaultFlags);
            }

            return defaultFlags;
        }

        bool AssetBrowserListModel::dropMimeData(
            const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
        {
            if (action == Qt::IgnoreAction)
            {
                return true;
            }

            auto sourceParent = mapToSource(parent).data(AssetBrowserModel::Roles::EntryRole).value<const AssetBrowserEntry*>();

            // We should only have an item as a folder but will check
            if (sourceParent && (sourceParent->GetEntryType() == AssetBrowserEntry::AssetEntryType::Folder))
            {
                AZStd::vector<const AssetBrowserEntry*> entries;

                if (Utils::FromMimeData(data, entries))
                {
                    for (auto entry : entries)
                    {
                        using namespace AZ::IO;
                        Path fromPath;
                        Path toPath;
                        bool isFolder{ true };

                        if (entry && (entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Source))
                        {
                            fromPath = entry->GetFullPath();
                            PathView filename = fromPath.Filename();
                            toPath = sourceParent->GetFullPath();
                            toPath /= filename;
                            isFolder = false;
                        }
                        else
                        {
                            fromPath = entry->GetFullPath() + "/*";
                            Path filename = static_cast<Path>(entry->GetFullPath()).Filename();
                            toPath = AZ::IO::Path(sourceParent->GetFullPath()) / filename.c_str() / "*";
                        }
                        AssetBrowserViewUtils::MoveEntry(fromPath.c_str(), toPath.c_str(), isFolder);
                    }
                    return true;
                }
            }
            return QAbstractItemModel::dropMimeData(data, action, row, column, parent);
        }

        QMimeData* AssetBrowserListModel::mimeData(const QModelIndexList& indexes) const
        {
            QMimeData* mimeData = new QMimeData;

            AZStd::vector<const AssetBrowserEntry*> collected;
            collected.reserve(indexes.size());

            for (const auto& index : indexes)
            {
                if (index.isValid())
                {
                    auto item = mapToSource(index).data(AssetBrowserModel::Roles::EntryRole).value<const AssetBrowserEntry*>();
                    if (item)
                    {
                        collected.push_back(item);
                    }
                }
            }

            Utils::ToMimeData(mimeData, collected);

            return mimeData;
        }

        QStringList AssetBrowserListModel::mimeTypes() const
        {
            QStringList list = QAbstractItemModel::mimeTypes();
            list.append(AssetBrowserEntry::GetMimeType());
            return list;
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework
#include "AssetBrowser/moc_AssetBrowserListModel.cpp"
