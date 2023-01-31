/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserThumbnailViewProxyModel.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>

#include <AzQtComponents/Components/Widgets/AssetFolderThumbnailView.h>

#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        AssetBrowserThumbnailViewProxyModel::AssetBrowserThumbnailViewProxyModel(QObject* parent)
            : QIdentityProxyModel(parent)
        {
        }

        AssetBrowserThumbnailViewProxyModel::~AssetBrowserThumbnailViewProxyModel() = default;

        QVariant AssetBrowserThumbnailViewProxyModel::data(const QModelIndex& index, int role) const
        {
            auto assetBrowserEntry = mapToSource(index).data(AssetBrowserModel::Roles::EntryRole).value<const AssetBrowserEntry*>();
            AZ_Assert(assetBrowserEntry, "Couldn't fetch asset entry for the given index.");
            if (!assetBrowserEntry)
            {
                return {};
            }

            switch (role)
            {
            case Qt::DecorationRole:
                {
                    SharedThumbnail thumbnail;
                    QPersistentModelIndex persistentIndex;

                    ThumbnailerRequestBus::BroadcastResult(
                        thumbnail, &ThumbnailerRequests::GetThumbnail, assetBrowserEntry->GetThumbnailKey());
                    AZ_Assert(thumbnail, "The shared thumbnail was not available from the ThumbnailerRequestBus.");
                    if (!thumbnail || thumbnail->GetState() == Thumbnail::State::Failed)
                    {
                        return QIcon{};
                    }

                    return QIcon(thumbnail->GetPixmap());
                }

            case static_cast<int>(AzQtComponents::AssetFolderThumbnailView::Role::IsExpandable):
                {
                    // We don't want to see children on folders in the thumbnail view
                    if (assetBrowserEntry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Folder)
                    {
                        return false;
                    }

                    return rowCount(index) > 0;
                }
            case static_cast<int>(AzQtComponents::AssetFolderThumbnailView::Role::IsTopLevel):
                {
                    if (m_searchResultsMode)
                    {
                        auto isExactMatch =
                            index.data(static_cast<int>(AzQtComponents::AssetFolderThumbnailView::Role::IsExactMatch)).value<bool>();
                        return isExactMatch;
                    }
                    else
                    {
                        if (m_rootIndex.isValid())
                        {
                            return index.parent() == m_rootIndex;
                        }
                        else
                        {
                            return index.parent().isValid() && !index.parent().parent().isValid();
                        }
                    }
                }
            case static_cast<int>(AzQtComponents::AssetFolderThumbnailView::Role::IsVisible):
                {
                    auto isExactMatch =
                        index.data(static_cast<int>(AzQtComponents::AssetFolderThumbnailView::Role::IsExactMatch)).value<bool>();
                    return !m_searchResultsMode || (m_searchResultsMode && isExactMatch);
                }
            }

            return QAbstractProxyModel::data(index, role);
        }

        void AssetBrowserThumbnailViewProxyModel::SetRootIndex(const QModelIndex& index)
        {
            m_rootIndex = index;
        }

        bool AssetBrowserThumbnailViewProxyModel::GetShowSearchResultsMode() const
        {
            return m_searchResultsMode;
        }

        void AssetBrowserThumbnailViewProxyModel::SetShowSearchResultsMode(bool searchMode)
        {
            if (m_searchResultsMode != searchMode)
            {
                m_searchResultsMode = searchMode;
                beginResetModel();
                endResetModel();
            }
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework
