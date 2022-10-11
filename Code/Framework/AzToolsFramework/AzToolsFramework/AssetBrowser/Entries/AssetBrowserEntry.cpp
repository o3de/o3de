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
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntryUtils.h>

#include <AzToolsFramework/Thumbnails/SourceControlThumbnail.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntryCache.h>

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
            "Display Name"
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

        const AZStd::string AssetBrowserEntry::GetFullPath() const
        {
            // the full path could use a decoding:
            if (AZ::IO::FileIOBase* instance = AZ::IO::FileIOBase::GetInstance())
            {
                char resolvedPath[AZ_MAX_PATH_LEN] = { 0 };
                instance->ResolvePath(m_fullPath.Native().c_str(), resolvedPath, AZ_MAX_PATH_LEN);
                return AZStd::string(resolvedPath);
            }

            return m_fullPath.Native();
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
                disconnect(m_thumbnailKey.data(), &ThumbnailKey::ThumbnailUpdatedSignal, this, &AssetBrowserEntry::ThumbnailUpdated);
            }
            m_thumbnailKey = thumbnailKey;
            connect(m_thumbnailKey.data(), &ThumbnailKey::ThumbnailUpdatedSignal, this, &AssetBrowserEntry::ThumbnailUpdated);
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

        void AssetBrowserEntry::ThumbnailUpdated()
        {
            if (EntryCache* cache = EntryCache::GetInstance())
            {
                cache->m_dirtyThumbnailsSet.insert(this);
            }
        }

    } // namespace AssetBrowser
} // namespace AzToolsFramework

#include "AssetBrowser/Entries/moc_AssetBrowserEntry.cpp"
