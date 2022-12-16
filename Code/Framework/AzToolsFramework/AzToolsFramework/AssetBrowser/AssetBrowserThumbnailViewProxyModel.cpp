/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserThumbnailViewProxyModel.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>

#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        AssetBrowserThumbnailViewProxyModel::AssetBrowserThumbnailViewProxyModel(QObject* parent)
            : QAbstractProxyModel(parent)
        {
        }

        AssetBrowserThumbnailViewProxyModel::~AssetBrowserThumbnailViewProxyModel() = default;

        void AssetBrowserThumbnailViewProxyModel::SetSourceModelCurrentSelection(const QModelIndex& sourceIndex)
        {
            m_sourceSelectionIndex = sourceIndex;

            beginResetModel();
            RecomputeValidEntries();
            endResetModel();
        }

        QVariant AssetBrowserThumbnailViewProxyModel::data(const QModelIndex& index, int role) const
        {
            if (role == Qt::DecorationRole)
            {
                auto assetBrowserEntry = mapToSource(index).data(AssetBrowserModel::Roles::EntryRole).value<const AssetBrowserEntry*>();

                SharedThumbnail thumbnail;
                QPersistentModelIndex persistentIndex;

                ThumbnailerRequestBus::BroadcastResult(thumbnail, &ThumbnailerRequests::GetThumbnail, assetBrowserEntry->GetThumbnailKey());
                AZ_Assert(thumbnail, "The shared thumbnail was not available from the ThumbnailerRequestBus.");
                if (!thumbnail || thumbnail->GetState() == Thumbnail::State::Failed)
                {
                    return QIcon{};
                }

                return QIcon(thumbnail->GetPixmap());
            }

            return QAbstractProxyModel::data(index, role);
        }

        QModelIndex AssetBrowserThumbnailViewProxyModel::index(int row, int column, const QModelIndex& parent) const
        {
            if (!sourceModel())
            {
                return {};
            }

            if (column > 0)
            {
                return {};
            }

            if (parent.isValid())
            {
                // Anything with a grandparent is not valid in this model, we only accept
                // - Children of the selected item on the asset tree view of type "Source" or "Folder"
                // - All their children without filtering
                if (parent.parent().isValid())
                {
                    return {};
                }

                // We are dealing with a level 2 (child) item here, fetching the inner pointer from the source model directly
                // as we're not currently doing any filtering on children
                else
                {
                    auto sourceParentIdx = mapToSource(parent);
                    if (row >= sourceModel()->rowCount(sourceParentIdx))
                    {
                        return {};
                    }

                    auto sourceData = sourceModel()
                                          ->data(sourceModel()->index(row, 0, sourceParentIdx), AssetBrowserModel::Roles::EntryRole)
                                          .value<const AssetBrowserEntry*>();

                    if (sourceData)
                    {
                        return createIndex(row, column, const_cast<AssetBrowserEntry*>(sourceData));
                    }

                    return {};
                }
            }

            // Here we deal with a level 1 (parent) item. Get the inner pointer from the local cache, which contains some filtered
            // children of the currently selected item on the asset tree view.
            if (row >= m_sourceValidChildren.size())
            {
                return {};
            }

            auto sourceData =
                sourceModel()
                    ->data(sourceModel()->index(m_sourceValidChildren[row], 0, m_sourceSelectionIndex), AssetBrowserModel::Roles::EntryRole)
                    .value<const AssetBrowserEntry*>();

            return createIndex(row, column, const_cast<AssetBrowserEntry*>(sourceData));
        }

        QModelIndex AssetBrowserThumbnailViewProxyModel::parent(const QModelIndex& child) const
        {
            auto childData = static_cast<AssetBrowserEntry*>(child.internalPointer());
            if (!childData)
            {
                return {};
            }

            auto childDataParent = childData->GetParent();

            if (!childDataParent)
            {
                return {};
            }

            // If the parent of the source entry is the currently selected item on the asset tree view, we have a level 1 item,
            // which has no parent in this model
            if (childDataParent == m_sourceSelectionIndex.internalPointer())
            {
                return {};
            }
            else
            {
                auto childDataGrandParent = childDataParent->GetParent();

                // Rule out anything that is not a grandchildren of the selected item on the asset tree view
                if (childDataGrandParent != m_sourceSelectionIndex.internalPointer() || !childDataGrandParent)
                {
                    return {};
                }

                for (int i = 0; i < childDataGrandParent->GetChildCount(); i++)
                {
                    if (childDataGrandParent->GetChild(i) == childDataParent)
                    {
                        return createIndex(i, 0, childDataGrandParent);
                    }
                }
                return {};
            }
        }

        QModelIndex AssetBrowserThumbnailViewProxyModel::mapToSource(const QModelIndex& proxyIndex) const
        {
            if (!sourceModel())
            {
                return {};
            }

            auto indexRow = proxyIndex.row();

            // We're mapping a level 1 (parent) item, using the local cache to map to the proxy row properly
            if (!proxyIndex.parent().isValid())
            {
                if (indexRow >= m_sourceValidChildren.size())
                {
                    return {};
                }

                return sourceModel()->index(m_sourceValidChildren[indexRow], 0, m_sourceSelectionIndex);
            }
            // We're mapping a level 2 (child) item, passing through the row number
            else
            {
                return sourceModel()->index(indexRow, 0, mapToSource(proxyIndex.parent()));
            }
        }

        QModelIndex AssetBrowserThumbnailViewProxyModel::mapFromSource(const QModelIndex& sourceIndex) const
        {
            if (!sourceModel())
            {
                return {};
            }

            if (!sourceIndex.isValid())
            {
                return {};
            }

            auto indexRow = sourceIndex.row();

            // We're mapping a level 1 (parent) item, using the local cache to map the source row
            if (sourceIndex.parent() == m_sourceSelectionIndex)
            {
                auto findRowInValidRowsArray = AZStd::equal_range(m_sourceValidChildren.begin(), m_sourceValidChildren.end(), indexRow);
                if (findRowInValidRowsArray.first == m_sourceValidChildren.end())
                {
                    return {};
                }

                return index(static_cast<int>(AZStd::distance(m_sourceValidChildren.begin(), findRowInValidRowsArray.first)), 0, {});
            }
            // We're mapping a level 2 (child) item, passing through the row number
            else
            {
                return index(indexRow, 0, mapFromSource(sourceIndex.parent()));
            }
        }

        int AssetBrowserThumbnailViewProxyModel::rowCount(const QModelIndex& parent) const
        {
            Q_UNUSED(parent);

            if (!sourceModel())
            {
                return 0;
            }

            // We're counting level 2 (child) items, return 0 if the parent is a folder, otherwise forward row count from the source model
            if (parent.isValid())
            {
                auto parentData = mapToSource(parent).data(AssetBrowserModel::Roles::EntryRole).value<const AssetBrowserEntry*>();
                if (parentData->GetEntryType() == AssetBrowserEntry::AssetEntryType::Folder)
                {
                    return 0;
                }
                return sourceModel()->rowCount(mapToSource(parent));
            }

            // We're counting level 1 (parent) items, use the local cache to determine the row count
            return static_cast<int>(m_sourceValidChildren.size());
        }

        int AssetBrowserThumbnailViewProxyModel::columnCount(const QModelIndex& parent) const
        {
            Q_UNUSED(parent);

            return 1;
        }

        void AssetBrowserThumbnailViewProxyModel::RecomputeValidEntries()
        {
            m_sourceValidChildren.clear();

            if (!m_sourceSelectionIndex.isValid())
            {
                return;
            }

            if (!sourceModel())
            {
                return;
            }

            // Using AssetBrowserEntry::GetEntryType to determine which children of the selected item on the asset tree
            // should be shown on the thumbnail view.
            // Storing the valid indexes in a local cache that gets reused throughout the model.
            int selectionChildren = sourceModel()->rowCount(m_sourceSelectionIndex);
            for (int i = 0; i < selectionChildren; i++)
            {
                auto childData = sourceModel()
                                     ->index(i, 0, m_sourceSelectionIndex)
                                     .data(AssetBrowserModel::Roles::EntryRole)
                                     .value<const AssetBrowserEntry*>();
                AZ_Assert(childData, "Source model doesn't have a valid entry for item %d.", i);
                if (childData->GetEntryType() == AssetBrowserEntry::AssetEntryType::Source ||
                    childData->GetEntryType() == AssetBrowserEntry::AssetEntryType::Folder)
                {
                    m_sourceValidChildren.push_back(i);
                }
            }
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework
