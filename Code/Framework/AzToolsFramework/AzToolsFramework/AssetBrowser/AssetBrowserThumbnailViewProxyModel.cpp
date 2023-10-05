/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserThumbnailViewProxyModel.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntryUtils.h>
#include <AzToolsFramework/AssetBrowser/Entries/FolderAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/ProductAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserViewUtils.h>

#include <AzQtComponents/Components/Widgets/AssetFolderThumbnailView.h>
#include <AzToolsFramework/Editor/RichTextHighlighter.h>


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
            case Qt::DisplayRole:
                {
                    QString name = static_cast<const SourceAssetBrowserEntry*>(assetBrowserEntry)->GetName().c_str();

                    if (!m_searchString.empty())
                    {
                        // highlight characters in filter
                        name = AzToolsFramework::RichTextHighlighter::HighlightText(name, m_searchString.c_str());
                    }
                    return name;
                }
            case Qt::DecorationRole:
                {
                    return AssetBrowserViewUtils::GetThumbnail(assetBrowserEntry);
                }
            case Qt::ToolTipRole:
                {
                    return assetBrowserEntry->data(11).toString();
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

        const QModelIndex AssetBrowserThumbnailViewProxyModel::GetRootIndex() const
        {
            return m_rootIndex;
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

        void AssetBrowserThumbnailViewProxyModel::SetSearchString(const QString& searchString)
        {
            m_searchString = searchString.toUtf8().data();
        }

        Qt::DropActions AssetBrowserThumbnailViewProxyModel::supportedDropActions() const
        {
            return Qt::CopyAction | Qt::MoveAction;
        }

        Qt::ItemFlags AssetBrowserThumbnailViewProxyModel::flags(const QModelIndex& index) const
        {
            Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index) | Qt::ItemIsEditable;

            if (index.isValid())
            {
                // We can only drop items onto folders so set flags accordingly
                const AssetBrowserEntry* item = mapToSource(index).data(AssetBrowserModel::Roles::EntryRole).value<const AssetBrowserEntry*>();
                if (item)
                {
                    if (item->GetEntryType() == AssetBrowserEntry::AssetEntryType::Product || item->GetEntryType() == AssetBrowserEntry::AssetEntryType::Source)
                    {
                        return Qt::ItemIsDragEnabled | defaultFlags;
                    }
                    if (item->GetEntryType() == AssetBrowserEntry::AssetEntryType::Folder)
                    {
                        return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
                    }
                }
            }
            return defaultFlags;
        }

        bool AssetBrowserThumbnailViewProxyModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
        {
            if (action == Qt::IgnoreAction)
            {
                return true;
            }

            auto sourceparent = mapToSource(parent).data(AssetBrowserModel::Roles::EntryRole).value<const AssetBrowserEntry*>();

            // We should only have an item as a folder but will check
            if (sourceparent && (sourceparent->GetEntryType() == AssetBrowserEntry::AssetEntryType::Folder))
            {
                AZStd::vector<const AssetBrowserEntry*> entries;

                if (Utils::FromMimeData(data, entries))
                {
                    Qt::DropAction selectedAction = AssetBrowserViewUtils::SelectDropActionForEntries(entries);
                    if (selectedAction == Qt::IgnoreAction)
                    {
                        return false;
                    }

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
                            toPath = sourceparent->GetFullPath();
                            toPath /= filename;
                            isFolder = false;
                        }
                        else
                        {
                            fromPath = entry->GetFullPath() + "/*";
                            Path filename = static_cast<Path>(entry->GetFullPath()).Filename();
                            toPath = AZ::IO::Path(sourceparent->GetFullPath()) / filename.c_str() / "*";
                        }
                        if (selectedAction == Qt::MoveAction)
                        {
                            AssetBrowserViewUtils::MoveEntry(fromPath.c_str(), toPath.c_str(), isFolder);
                        }
                        else
                        {
                            AssetBrowserViewUtils::CopyEntry(fromPath.c_str(), toPath.c_str(), isFolder);
                        }
                    }
                    return true;
                }
            }
            return QAbstractItemModel::dropMimeData(data, action, row, column, parent);
        }

        QMimeData* AssetBrowserThumbnailViewProxyModel::mimeData(const QModelIndexList& indexes) const
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

        QStringList AssetBrowserThumbnailViewProxyModel::mimeTypes() const
        {
            QStringList list = QAbstractItemModel::mimeTypes();
            list.append(AssetBrowserEntry::GetMimeType());
            return list;
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework
