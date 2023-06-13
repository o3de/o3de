/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserTableViewProxyModel.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntryUtils.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserViewUtils.h>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        AssetBrowserTableViewProxyModel::AssetBrowserTableViewProxyModel(QObject* parent)
            : QIdentityProxyModel(parent)
        {
        }

        AssetBrowserTableViewProxyModel::~AssetBrowserTableViewProxyModel() = default;

        QVariant AssetBrowserTableViewProxyModel::data(const QModelIndex& index, int role) const
        {
            auto assetBrowserEntry = mapToSource(index).data(AssetBrowserModel::Roles::EntryRole).value<const AssetBrowserEntry*>();
            AZ_Assert(assetBrowserEntry, "Couldn't fetch asset entry for the given index.");
            if (!assetBrowserEntry)
            {
                return tr(" No Data ");
            }

            switch (role)
            {
            case Qt::DisplayRole:
                {
                    switch (index.column())
                    {
                    case Name:
                        return static_cast<const SourceAssetBrowserEntry*>(assetBrowserEntry)->GetName().c_str();
                    case Type:
                        {
                            return assetBrowserEntry->GetEntryTypeAsString();
                        }
                    case DiskSize:
                        if (assetBrowserEntry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Source)
                        {
                            return QString{ "%1" }.arg(assetBrowserEntry->GetDiskSize() / 1024.0, 0, 'f', 3);
                        }
                        return "";
                    case Vertices:
                        if (assetBrowserEntry->GetNumVertices() > 0)
                        {
                            return assetBrowserEntry->GetNumVertices();
                        }
                        return "";
                    case ApproxSize:
                        if (!AZStd::isnan(assetBrowserEntry->GetDimension().GetX()))
                        {
                            const AZ::Vector3& dim{ assetBrowserEntry->GetDimension() };
                            if (abs(dim.GetX())<1.0f && abs(dim.GetY())<1.0f && abs(dim.GetZ())<1.0f)
                                return QString{ "%1 x %2 x %3" }.arg(dim.GetX()).arg(dim.GetY()).arg(dim.GetZ());
                            return QString{ "%1 x %2 x %3" }
                                .arg(static_cast<int>(dim.GetX()))
                                .arg(static_cast<int>(dim.GetY()))
                                .arg(static_cast<int>(dim.GetZ()));
                        }
                        return "";
                    default:
                        return "";
                    }
                }
            case Qt::TextAlignmentRole:
                if (index.column() == DiskSize || index.column() == Vertices)
                {
                    return QVariant(Qt::AlignRight | Qt::AlignVCenter);
                }
                break;
            case Qt::UserRole:
                return QString(assetBrowserEntry->GetFullPath().c_str());
            case Qt::UserRole + 1:
                return AssetBrowserViewUtils::GetThumbnail(assetBrowserEntry);
            }
            return QAbstractProxyModel::data(index, role);
        }

        QVariant AssetBrowserTableViewProxyModel::headerData(int section, Qt::Orientation orientation, int role) const
        {
            switch (role)
            {
            case Qt::DisplayRole:
                if (orientation == Qt::Horizontal)
                {
                    section += section ? aznumeric_cast<int>(AssetBrowserEntry::Column::Type) - 1 : 0;
                    return tr(AssetBrowserEntry::m_columnNames[section]);
                }
                break;
            case Qt::TextAlignmentRole:
                if (section == DiskSize || section == Vertices)
                {
                    return QVariant(Qt::AlignRight | Qt::AlignVCenter);
                }
                break;
            }
            return QVariant();
        }

        bool AssetBrowserTableViewProxyModel::hasChildren(const QModelIndex& parent) const
        {
            if (parent != m_rootIndex)
            {
                return false;
            }
            return true;
        }

        int AssetBrowserTableViewProxyModel::columnCount([[maybe_unused]]const QModelIndex& parent) const
        {
            return ColumnCount;
        }

        void AssetBrowserTableViewProxyModel::SetRootIndex(const QModelIndex& index)
        {
            m_rootIndex = index;
        }

        const QModelIndex AssetBrowserTableViewProxyModel::GetRootIndex() const
        {
            return m_rootIndex;
        }

        bool AssetBrowserTableViewProxyModel::GetShowSearchResultsMode() const
        {
            return m_searchResultsMode;
        }

        void AssetBrowserTableViewProxyModel::SetShowSearchResultsMode(bool searchMode)
        {
            if (m_searchResultsMode != searchMode)
            {
                m_searchResultsMode = searchMode;
                beginResetModel();
                endResetModel();
            }
        }
        

        bool AssetBrowserTableViewProxyModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
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
    } // namespace AssetBrowser
} // namespace AzToolsFramework
