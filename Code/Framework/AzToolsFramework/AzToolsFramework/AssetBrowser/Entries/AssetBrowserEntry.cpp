/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/regex.h>
#include <AzCore/Serialization/Utils.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>
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
            default:
                return QVariant();
            }
        }

        int AssetBrowserEntry::row() const
        {
            return m_row;
        }

        bool AssetBrowserEntry::FromMimeData(const QMimeData* mimeData, AZStd::vector<AssetBrowserEntry*>& entries)
        {
            if (!mimeData)
            {
                return false;
            }

            for (auto format : mimeData->formats())
            {
                if (format != GetMimeType())
                {
                    continue;
                }

                QByteArray arrayData = mimeData->data(format);
                AZ::IO::MemoryStream ms(arrayData.constData(), arrayData.size());
                AssetBrowserEntry* entry = AZ::Utils::LoadObjectFromStream<AssetBrowserEntry>(ms, nullptr);
                if (entry)
                {
                    entries.push_back(entry);
                }
            }
            return entries.size() > 0;
        }

        void AssetBrowserEntry::AddToMimeData(QMimeData* mimeData) const
        {
            if (!mimeData)
            {
                return;
            }

            AZStd::vector<char> buffer;

            AZ::IO::ByteContainerStream<AZStd::vector<char> > byteStream(&buffer);
            AZ::Utils::SaveObjectToStream(byteStream, AZ::DataStream::ST_BINARY, this, this->RTTI_GetType());

            QByteArray dataArray(buffer.data(), static_cast<int>(sizeof(char) * buffer.size()));
            mimeData->setData(GetMimeType(), dataArray);
            mimeData->setUrls({ QUrl::fromLocalFile(GetFullPath().c_str()) });
        }

        QString AssetBrowserEntry::GetMimeType()
        {
            return "editor/assetinformation/entry";
        }

        void AssetBrowserEntry::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<AssetBrowserEntry>()
                    ->Field("m_name", &AssetBrowserEntry::m_name)
                    ->Field("m_children", &AssetBrowserEntry::m_children)
                    ->Field("m_row", &AssetBrowserEntry::m_row)
                    ->Field("m_fullPath", &AssetBrowserEntry::m_fullPath)
                    ->Version(2);
            }
        }

        const AZStd::string& AssetBrowserEntry::GetName() const
        {
            return m_name;
        }

        const QString& AssetBrowserEntry::GetDisplayName() const
        {
            return m_displayName;
        }

        const AZStd::string& AssetBrowserEntry::GetRelativePath() const
        {
            return m_relativePath;
        }

        const AZStd::string& AssetBrowserEntry::GetFullPath() const
        {
            return m_fullPath;
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
