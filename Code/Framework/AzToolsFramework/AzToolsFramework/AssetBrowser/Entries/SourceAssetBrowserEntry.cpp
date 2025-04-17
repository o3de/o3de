/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/containers/vector.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/ProductAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Thumbnails/SourceThumbnail.h>
#include <AzToolsFramework/Thumbnails/SourceControlThumbnail.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntryCache.h>

#include <QVariant>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        SourceAssetBrowserEntry::~SourceAssetBrowserEntry()
        {
            if (EntryCache* cache = EntryCache::GetInstance())
            {
                cache->m_fileIdMap.erase(m_fileId);
                cache->m_absolutePathToFileId.erase(m_fullPath.LexicallyNormal().Native());

                if (m_sourceId != -1)
                {
                    cache->m_sourceUuidMap.erase(m_sourceUuid);
                    cache->m_sourceIdMap.erase(m_sourceId);
                }
            }
        }

        QVariant SourceAssetBrowserEntry::data(int column) const
        {
            switch (static_cast<Column>(column))
            {
            case Column::SourceID:
            {
                return QVariant(m_sourceId);
            }
            case Column::ScanFolderID:
            {
                return QVariant(m_scanFolderId);
            }
            default:
            {
                return AssetBrowserEntry::data(column);
            }
            }
        }

        AssetBrowserEntry::AssetEntryType SourceAssetBrowserEntry::GetEntryType() const
        {
            return AssetEntryType::Source;
        }

        const AZStd::string SourceAssetBrowserEntry::GetExtension() const
        {
            AZStd::string extension;
            AZ::StringFunc::Path::GetExtension(GetFullPath().c_str(), extension);
            return extension;
        };

        const AZStd::string SourceAssetBrowserEntry::GetFileName() const
        {
            AZStd::string stem;
            AZ::StringFunc::Path::GetFileName(GetFullPath().c_str(), stem);
            return stem;
        };

        AZ::s64 SourceAssetBrowserEntry::GetFileID() const
        {
            return m_fileId;
        }

        const AZ::Uuid& SourceAssetBrowserEntry::GetSourceUuid() const
        {
            return m_sourceUuid;
        }

        AZ::s64 SourceAssetBrowserEntry::GetSourceID() const
        {
            return m_sourceId;
        }

        AZ::s64 SourceAssetBrowserEntry::GetScanFolderID() const
        {
            return m_scanFolderId;
        }

        AZ::Data::AssetType SourceAssetBrowserEntry::GetPrimaryAssetType() const
        {
            AZStd::vector<const ProductAssetBrowserEntry*> products;
            GetChildren<ProductAssetBrowserEntry>(products);
            for (const ProductAssetBrowserEntry* product : products)
            {
                AZ::Data::AssetType productType = product->GetAssetType();
                if (productType != AZ::Data::s_invalidAssetType)
                {
                    return productType;
                }
            }

            return AZ::Data::s_invalidAssetType;
        }

        bool SourceAssetBrowserEntry::HasProductType(const AZ::Data::AssetType& assetType) const
        {
            AZStd::vector<const ProductAssetBrowserEntry*> products;
            GetChildren<ProductAssetBrowserEntry>(products);
            for (const ProductAssetBrowserEntry* product : products)
            {
                AZ::Data::AssetType productType = product->GetAssetType();
                if (productType == assetType)
                {
                    return true;
                }
            }
            return false;
        }

        const SourceAssetBrowserEntry* SourceAssetBrowserEntry::GetSourceByUuid(const AZ::Uuid& sourceUuid)
        {
            if (EntryCache* cache = EntryCache::GetInstance())
            {
                return cache->m_sourceUuidMap[sourceUuid];
            }
            return nullptr;
        }

        void SourceAssetBrowserEntry::PathsUpdated()
        {
            AssetBrowserEntry::PathsUpdated();
            UpdateSourceControlThumbnail();
        }

        void SourceAssetBrowserEntry::UpdateSourceControlThumbnail()
        {
            if (m_sourceControlThumbnailKey)
            {
                disconnect(m_sourceControlThumbnailKey.data(), nullptr, this, nullptr);
            }
            m_sourceControlThumbnailKey = MAKE_TKEY(SourceControlThumbnailKey, m_fullPath.c_str());
            connect(m_sourceControlThumbnailKey.data(), &ThumbnailKey::ThumbnailUpdated, this, &AssetBrowserEntry::SetThumbnailDirty);
        }

        SharedThumbnailKey SourceAssetBrowserEntry::CreateThumbnailKey()
        {
            return MAKE_TKEY(SourceThumbnailKey, m_sourceUuid);
        }

        SharedThumbnailKey SourceAssetBrowserEntry::GetSourceControlThumbnailKey() const
        {
            return m_sourceControlThumbnailKey;
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework
