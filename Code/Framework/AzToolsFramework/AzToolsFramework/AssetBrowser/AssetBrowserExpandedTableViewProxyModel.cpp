/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserExpandedTableViewProxyModel.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        AssetBrowserExpandedTableViewProxyModel::AssetBrowserExpandedTableViewProxyModel(QObject* parent)
            : QIdentityProxyModel(parent)
        {
        }

        AssetBrowserExpandedTableViewProxyModel::~AssetBrowserExpandedTableViewProxyModel() = default;

        QVariant AssetBrowserExpandedTableViewProxyModel::data(const QModelIndex& index, int role) const
        {
            auto assetBrowserEntry = mapToSource(index).data(AssetBrowserModel::Roles::EntryRole).value<const AssetBrowserEntry*>();
            AZ_Assert(assetBrowserEntry, "Couldn't fetch asset entry for the given index.");
            if (!assetBrowserEntry)
            {
                return " No Data ";
            }

            switch (role)
            {
            case Qt::DisplayRole:
                {
                    switch (index.column())
                    {
                    case 0:
                        return static_cast<const SourceAssetBrowserEntry*>(assetBrowserEntry)->GetName().c_str();
                    case 1:
                        {
                            switch (assetBrowserEntry->GetEntryType())
                            {
                            case AssetBrowserEntry::AssetEntryType::Root:
                                return "Root";
                            case AssetBrowserEntry::AssetEntryType::Folder:
                                return "Folder";
                            case AssetBrowserEntry::AssetEntryType::Source:
                                return ExtensionToType(static_cast<const SourceAssetBrowserEntry*>(assetBrowserEntry)->GetExtension()).c_str();
                            case AssetBrowserEntry::AssetEntryType::Product:
                                return "Product";
                            }
                        }
                    case 2:
                        if(assetBrowserEntry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Source)
                            return assetBrowserEntry->GetDiskSize();
                        return "";
                    default:
                        return "No data";
                    }
                }
            case Qt::UserRole:
                return QString(assetBrowserEntry->GetFullPath().c_str());
            case Qt::UserRole + 1:
                return assetBrowserEntry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Folder;
            }
            return QVariant();
        }

        QVariant AssetBrowserExpandedTableViewProxyModel::headerData(int section, Qt::Orientation orientation, int role) const
        {
            if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
            {
                section += section ? static_cast<int>(AssetBrowserEntry::Column::Type) - 1 : 0;
                return tr(AssetBrowserEntry::m_columnNames[section]);
            }

            return QVariant();
        }

        bool AssetBrowserExpandedTableViewProxyModel::hasChildren(const QModelIndex& parent) const
        {
            if (parent != m_rootIndex)
            {
                return false;
            }
            return true;
        }

        int AssetBrowserExpandedTableViewProxyModel::columnCount([[maybe_unused]]const QModelIndex& parent) const
        {
            return 6;
        }

        void AssetBrowserExpandedTableViewProxyModel::SetRootIndex(const QModelIndex& index)
        {
            m_rootIndex = index;
        }

        bool AssetBrowserExpandedTableViewProxyModel::GetShowSearchResultsMode() const
        {
            return m_searchResultsMode;
        }

        void AssetBrowserExpandedTableViewProxyModel::SetShowSearchResultsMode(bool searchMode)
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

        const AZStd::string AssetBrowserExpandedTableViewProxyModel::ExtensionToType(AZStd::string_view str) const
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
                return ".WAV";
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

    } // namespace AssetBrowser
} // namespace AzToolsFramework
