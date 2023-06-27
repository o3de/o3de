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
#include <AzToolsFramework/Editor/RichTextHighlighter.h>

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
                        {
                            QString name = static_cast<const SourceAssetBrowserEntry*>(assetBrowserEntry)->GetName().c_str();

                            if (!m_searchString.empty())
                            {
                                // highlight characters in filter
                                name = AzToolsFramework::RichTextHighlighter::HighlightText(name, m_searchString.c_str());
                            }
                            return name;
                        }
                    case Type:
                        {
                            switch (assetBrowserEntry->GetEntryType())
                            {
                            case AssetBrowserEntry::AssetEntryType::Root:
                                return tr("Root");
                            case AssetBrowserEntry::AssetEntryType::Folder:
                                return tr("Folder");
                            case AssetBrowserEntry::AssetEntryType::Source:
                                return ExtensionToType(static_cast<const SourceAssetBrowserEntry*>(assetBrowserEntry)->GetExtension()).c_str();
                            case AssetBrowserEntry::AssetEntryType::Product:
                                return tr("Product");
                            }
                        }
                    case DiskSize:
                        if (GetShowSearchResultsMode())
                        {
                            return assetBrowserEntry->GetDisplayPath();
                        }
                        else
                        {
                            if (assetBrowserEntry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Source)
                            {
                                return QString{ "%1" }.arg(assetBrowserEntry->GetDiskSize() / 1024.0, 0, 'f', 3);
                            }
                        }
                        return "";
                    case Vertices:
                        if (!GetShowSearchResultsMode()  && assetBrowserEntry->GetNumVertices() > 0)
                        {
                            return assetBrowserEntry->GetNumVertices();
                        }
                        return "";
                    case ApproxSize:
                        if (!GetShowSearchResultsMode())
                        {
                            if (!AZStd::isnan(assetBrowserEntry->GetDimension().GetX()))
                            {
                                const AZ::Vector3& dim{ assetBrowserEntry->GetDimension() };
                                if (abs(dim.GetX()) < 1.0f && abs(dim.GetY()) < 1.0f && abs(dim.GetZ()) < 1.0f)
                                    return QString{ "%1 x %2 x %3" }.arg(dim.GetX()).arg(dim.GetY()).arg(dim.GetZ());
                                return QString{ "%1 x %2 x %3" }
                                    .arg(static_cast<int>(dim.GetX()))
                                    .arg(static_cast<int>(dim.GetY()))
                                    .arg(static_cast<int>(dim.GetZ()));
                            }
                        }
                        return "";
                    default:
                        return "";
                    }
                }
            case Qt::TextAlignmentRole:
                if ((index.column() == DiskSize && !GetShowSearchResultsMode()) || index.column() == Vertices)
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
                    const int sourceID = aznumeric_cast<int>(AssetBrowserEntry::Column::SourceID);
                    const int path = aznumeric_cast<int>(AssetBrowserEntry::Column::Path);
                    const int columnNameRequired = section ? section + aznumeric_cast<int>(AssetBrowserEntry::Column::Type) - 1 : 0;
                    if (GetShowSearchResultsMode())
                    {
                        if (section <= sourceID)
                        {
                            return tr(AssetBrowserEntry::m_columnNames[section == sourceID ? path : columnNameRequired]);
                        }
                    }
                    else
                    {
                        return tr(AssetBrowserEntry::m_columnNames[columnNameRequired]);
                    }
                }
                break;
            case Qt::TextAlignmentRole:
                if ((section == DiskSize  && !GetShowSearchResultsMode())|| section == Vertices)
                {
                    return QVariant(Qt::AlignRight | Qt::AlignVCenter);
                }
                break;
            }
            return QVariant();
        }

        bool AssetBrowserTableViewProxyModel::hasChildren(const QModelIndex& parent) const
        {
            if (m_searchResultsMode)
            {
                return (rowCount(parent) > 0) && (columnCount(parent) > 0);
            }
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
        inline constexpr auto operator"" _hash(const char* str, size_t len)
        {
            return AZStd::hash<AZStd::string_view>{}(AZStd::string_view{ str, len });
        }

        const AZStd::string AssetBrowserTableViewProxyModel::ExtensionToType(AZStd::string_view str) const
        {
            switch (AZStd::hash<AZStd::string_view>{}(str))
            {
            case ".png"_hash:
                return "PNG";
            case ".scriptcanvas"_hash:
                return "Script Canvas";
            case ".fbx"_hash:
                return "FBX";
            case ".mtl"_hash:
                return "Material";
            case ".animgraph"_hash:
                return "Anim Graph";
            case ".motionset"_hash:
                return "Motion Set";
            case ".assetinfo"_hash:
                return "Asset Info";
            case ".py"_hash:
                return "Python Script";
            case ".lua"_hash:
                return "Lua Script";
            case ".tif"_hash:
            case ".tiff"_hash:
                return "TIF";
            case ".physxmaterial"_hash:
                return "PhysX Material";
            case ".prefab"_hash:
                return "Prefab";
            case ".dds"_hash:
                return "DDS";
            case ".font"_hash:
                return "Font";
            case ".xml"_hash:
                return "XML";
            case ".json"_hash:
                return "JSON";
            case ".exr"_hash:
                return "EXR";
            case ".wav"_hash:
                return "WAV";
            case ".uicanvas"_hash:
                return "UI Canvas";
            case ".wwu"_hash:
                return "Wwise Work Unit";
            case ".wproj"_hash:
                return "Wwise Project File";
            default:
                if (str.length() > 0)
                {
                    str.remove_prefix(1);
                }
                return str;
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

        void AssetBrowserTableViewProxyModel::SetSearchString(const QString& searchString)
        {
             m_searchString = searchString.toUtf8().data();
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework
