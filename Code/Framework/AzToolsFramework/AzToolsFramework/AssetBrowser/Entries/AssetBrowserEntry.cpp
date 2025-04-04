/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/FileIO.h>

#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/regex.h>
#include <AzCore/Serialization/Utils.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserTableViewProxyModel.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntryUtils.h>
#include <AzToolsFramework/AssetBrowser/Entries/FolderAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>

#include <AzToolsFramework/Thumbnails/SourceControlThumbnail.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntryCache.h>

#include <QCollator>
#include <QVariant>
#include <QMimeData>
#include <QUrl>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        QString AssetBrowserEntry::AssetEntryTypeToString(AssetEntryType assetEntryType)
        {
            switch (assetEntryType)
            {
            case AssetEntryType::Root:
                return QObject::tr("Root");
            case AssetEntryType::Folder:
                return QObject::tr("Folder");
            case AssetEntryType::Source:
                return QObject::tr("Source");
            case AssetEntryType::Product:
                return QObject::tr("Product");
            default:
                return QObject::tr("Unknown");
            }
        }

        const char* AssetBrowserEntry::m_columnNames[] =
        {
            "Name",
            "Path",
            "Source ID",
            "Fingerprint",
            "Guid",
            "ScanFolder ID",
            "Product ID",
            "Job ID",
            "Sub ID",
            "Asset Type",
            "Class ID",
            "Display Name",
            "Type",
            "Disk Size (KiB)",
            "Vertices",
            "Approximate Size",
            "Source Control Status"
        };

        AssetBrowserEntry::AssetBrowserEntry()
            : QObject()
        {}

        AssetBrowserEntry::~AssetBrowserEntry()
        {
            if (EntryCache* cache = EntryCache::GetInstance())
            {
                cache->m_dirtyThumbnailsSet.erase(this);
            }
            RemoveChildren();
        }

        void AssetBrowserEntry::AddChild(AssetBrowserEntry* child)
        {
            child->m_parentAssetEntry = this;

            UpdateChildPaths(child);

            AssetBrowserModelRequestBus::Broadcast(&AssetBrowserModelRequests::BeginAddEntry, this);
            child->m_row = static_cast<int>(m_children.size());
            m_children.push_back(child);
            AssetBrowserModelRequestBus::Broadcast(&AssetBrowserModelRequests::EndAddEntry, this);
            AssetBrowserModelNotificationBus::Broadcast(&AssetBrowserModelNotifications::EntryAdded, child);
        }

        void AssetBrowserEntry::RemoveChild(AssetBrowserEntry* child)
        {
            if (!child || child->m_row >= m_children.size() || child != m_children[child->m_row])
            {
                return;
            }

            AZStd::unique_ptr<AssetBrowserEntry> childToRemove(m_children[child->m_row]);
            if (!childToRemove)
            {
                return;
            }

            AssetBrowserModelRequestBus::Broadcast(&AssetBrowserModelRequests::BeginRemoveEntry, childToRemove.get());
            auto it = m_children.erase(m_children.begin() + child->m_row);

            // decrement the row of all children after the removed child
            while (it != m_children.end())
            {
                (*it++)->m_row--;
            }

            child->m_parentAssetEntry = nullptr;
            AssetBrowserModelRequestBus::Broadcast(&AssetBrowserModelRequests::EndRemoveEntry);
            AssetBrowserModelNotificationBus::Broadcast(&AssetBrowserModelNotifications::EntryRemoved, childToRemove.get());

            // before we allow the destructor of AssetBrowserEntry to run, we must remove its children, etc.
            childToRemove->RemoveChildren();
        }

        void AssetBrowserEntry::RemoveChildren()
        {
            while (!m_children.empty())
            {
                // child entries are removed from the end of the list, because this will incur minimum effort to update their rows
                RemoveChild(*m_children.rbegin());
            }
        }

        QVariant AssetBrowserEntry::data(int column) const
        {
            switch (static_cast<Column>(column))
            {
            case Column::Name:
                return QString::fromUtf8(m_name.c_str());
            case Column::DisplayName:
                return m_displayName;
            case Column::Path:
                return m_displayPath;
            default:
                return QVariant();
            }
        }

        int AssetBrowserEntry::row() const
        {
            return m_row;
        }

        bool AssetBrowserEntry::FromMimeData(const QMimeData* mimeData, AZStd::vector<const AssetBrowserEntry*>& entries)
        {
            // deprecated.  You can use the AssetBrowserEntryUtils::FromMimeData directly now.
            return Utils::FromMimeData(mimeData, entries);
        }

        QString AssetBrowserEntry::GetMimeType()
        {
            return "editor/assetinformation/entry";
        }

        const AZStd::string& AssetBrowserEntry::GetName() const
        {
            return m_name;
        }

        const AZ::u32 AssetBrowserEntry::GetGroupNameCrc() const
        {
            return m_groupNameCrc;
        }

        const QString& AssetBrowserEntry::GetGroupName() const
        {
            return m_groupName;
        }

        const QString& AssetBrowserEntry::GetDisplayName() const
        {
            return m_displayName;
        }

        const QString& AssetBrowserEntry::GetDisplayPath() const
        {
            return m_displayPath;
        }

        const AZStd::string& AssetBrowserEntry::GetRelativePath() const
        {
            return m_relativePath.Native();
        }

        const AZStd::string& AssetBrowserEntry::GetVisiblePath() const
        {
            return m_visiblePath.Native();
        }

        const AZStd::string& AssetBrowserEntry::GetFullPath() const
        {
            return m_fullPath.Native();
        }

        const size_t AssetBrowserEntry::GetDiskSize() const
        {
            return m_diskSize;
        }

        const AZ::u64 AssetBrowserEntry::GetModificationTime() const
        {
            return m_modificationTime;
        }

        const AZ::Vector3& AssetBrowserEntry::GetDimension() const
        {
            return m_dimension;
        }

        const uint32_t AssetBrowserEntry::GetNumVertices() const
        {
            return m_vertices;
        }

        void AssetBrowserEntry::SetFullPath(const AZ::IO::Path& fullPath)
        {
            m_fullPath = fullPath;

            // the full path could use a decoding:
            if (AZ::IO::FileIOBase* instance = AZ::IO::FileIOBase::GetInstance())
            {
                char resolvedPath[AZ_MAX_PATH_LEN] = { 0 };
                if (instance->ResolvePath(fullPath.Native().c_str(), resolvedPath, AZ_MAX_PATH_LEN))
                {
                    m_fullPath = resolvedPath;
                }
            }

            // Update the entryType string.
            switch (GetEntryType())
            {
            case AssetBrowserEntry::AssetEntryType::Root:
                m_entryType = tr("Root");
                break;
            case AssetBrowserEntry::AssetEntryType::Folder:
                m_entryType = tr("Folder");
                break;
            case AssetBrowserEntry::AssetEntryType::Source:
                m_entryType = ExtensionToType(static_cast<const SourceAssetBrowserEntry*>(this)->GetExtension()).c_str();
                break;
            case AssetBrowserEntry::AssetEntryType::Product:
                m_entryType = tr("Product");
                break;
            default:
                m_entryType = "";
                break;
            }
        }
        

        const QString& AssetBrowserEntry::GetEntryTypeAsString() const
        {
            return m_entryType;
        }

        inline constexpr auto operator"" _hash(const char* str, size_t len)
        {
            return AZStd::hash<AZStd::string_view>{}(AZStd::string_view{ str, len });
        }

        const AZStd::string AssetBrowserEntry::ExtensionToType(AZStd::string_view str)
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

        void AssetBrowserEntry::VisitUp(const AZStd::function<bool(const AssetBrowserEntry*)>& visitorFn) const
        {
            if (!visitorFn)
            {
                return;
            }

            for (auto entry = this; entry && entry->GetEntryType() != AssetEntryType::Root; entry = entry->GetParent())
            {
                if (!visitorFn(entry))
                {
                    return;
                }
            }
        }

        void AssetBrowserEntry::VisitDown(const AZStd::function<bool(const AssetBrowserEntry*)>& visitorFn) const
        {
            if (!visitorFn || !visitorFn(this))
            {
                return;
            }

            for (auto child : m_children)
            {
                child->VisitDown(visitorFn);
            }
        }

        const AssetBrowserEntry* AssetBrowserEntry::GetChild(int index) const
        {
            if (index < m_children.size())
            {
                return m_children[index];
            }
            return nullptr;
        }

        AssetBrowserEntry* AssetBrowserEntry::GetChild(int index)
        {
            if (index < m_children.size())
            {
                return m_children[index];
            }
            return nullptr;
        }

        int AssetBrowserEntry::GetChildCount() const
        {
            return static_cast<int>(m_children.size());
        }

        AssetBrowserEntry* AssetBrowserEntry::GetParent() const
        {
            return m_parentAssetEntry;
        }

        void AssetBrowserEntry::SetThumbnailKey(SharedThumbnailKey thumbnailKey)
        {
            if (m_thumbnailKey)
            {
                disconnect(m_thumbnailKey.data(), nullptr, this, nullptr);
            }
            m_thumbnailKey = thumbnailKey;
            connect(m_thumbnailKey.data(), &ThumbnailKey::ThumbnailUpdated, this, &AssetBrowserEntry::SetThumbnailDirty);
        }

        SharedThumbnailKey AssetBrowserEntry::GetThumbnailKey() const
        {
            return m_thumbnailKey;
        }

        void AssetBrowserEntry::UpdateChildPaths(AssetBrowserEntry* child) const
        {
            // by default, we just recurse here.
            // sources will do this differently from folders and products...
            child->PathsUpdated();
        }

        void AssetBrowserEntry::PathsUpdated()
        {
            SetThumbnailKey(CreateThumbnailKey());
        }

        void AssetBrowserEntry::SetThumbnailDirty()
        {
            if (EntryCache* cache = EntryCache::GetInstance())
            {
                cache->m_dirtyThumbnailsSet.insert(this);
            }
        }


        void AssetBrowserEntry::SetDisplayName(const QString name)
        {
            m_displayName = name;
        }

        void AssetBrowserEntry::SetIconPath(const AZ::IO::Path path)
        {
            m_iconPath = path;
        }

        AZ::IO::Path AssetBrowserEntry::GetIconPath() const
        {
            return m_iconPath;
        }
        
        bool AssetBrowserEntry::lessThan(const AssetBrowserEntry* other, const AssetEntrySortMode sortMode, const QCollator& collator) const
        {
            // folders should always come first
            if (GetEntryType() == AssetEntryType::Folder && other->GetEntryType() != AssetEntryType::Folder)
            {
                return true;
            }

            if (GetEntryType() != AssetEntryType::Folder && other->GetEntryType() == AssetEntryType::Folder)
            {
                return false;
            }

            switch (sortMode)
            {
            case AssetEntrySortMode::FileType:
                {
                    int comparison = collator.compare(GetEntryTypeAsString(), other->GetEntryTypeAsString());
                    if (comparison == 0)
                    {
                        return collator.compare(GetDisplayName(), other->GetDisplayName()) > 0;
                    }
                    return comparison > 0;
                }
            case AssetBrowserEntry::AssetEntrySortMode::LastModified:
                return GetModificationTime() < other->GetModificationTime();
            case AssetEntrySortMode::Size:
                return GetDiskSize() > other->GetDiskSize();
            case AssetEntrySortMode::Vertices:
                if (GetNumVertices() == other->GetNumVertices())
                {
                    return collator.compare(GetDisplayName(), other->GetDisplayName()) > 0;
                }
                return GetNumVertices() > other->GetNumVertices();
            case AssetEntrySortMode::Dimensions:
                {
                    AZ::Vector3 leftDimension = GetDimension();
                    AZ::Vector3 rightDimension = other->GetDimension();

                    if (AZStd::isnan(leftDimension.GetX()) && AZStd::isnan(rightDimension.GetX()))
                    {
                        return collator.compare(GetDisplayName(), other->GetDisplayName()) > 0;
                    }
                    if (AZStd::isnan(leftDimension.GetX()) && !AZStd::isnan(rightDimension.GetX()))
                    {
                        return false;
                    }
                    if (!AZStd::isnan(leftDimension.GetX()) && AZStd::isnan(rightDimension.GetX()))
                    {
                        return true;
                    }

                    return leftDimension.GetX() * leftDimension.GetY() * leftDimension.GetZ() >
                        rightDimension.GetX() * rightDimension.GetY() * rightDimension.GetZ();
                }
            default:
                // if both entries are of same type, sort alphabetically
                return collator.compare(GetDisplayName(), other->GetDisplayName()) > 0;
            }
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework

#include "AssetBrowser/Entries/moc_AssetBrowserEntry.cpp"
