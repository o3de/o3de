/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/Entries/ProductAssetBrowserEntry.h>
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>
#include <AzToolsFramework/AssetBrowser/Thumbnails/ProductThumbnail.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntryCache.h>

#include <QVariant>

using namespace AzFramework;
using namespace AzToolsFramework::AssetDatabase;

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        ProductAssetBrowserEntry::~ProductAssetBrowserEntry()
        {
            if (EntryCache* cache = EntryCache::GetInstance())
            {
                cache->m_productAssetIdMap.erase(m_assetId);
            }
        }

        QVariant ProductAssetBrowserEntry::data(int column) const
        {
            switch (static_cast<Column>(column))
            {
            case Column::ProductID:
            {
                return QVariant(m_productId);
            }
            case Column::JobID:
            {
                return QVariant(m_jobId);
            }
            case Column::SubID:
            {
                return QVariant(m_assetId.m_subId);
            }
            default:
            {
                return AssetBrowserEntry::data(column);
            }
            }
        }

        AssetBrowserEntry::AssetEntryType ProductAssetBrowserEntry::GetEntryType() const
        {
            return AssetEntryType::Product;
        }

        AZ::s64 ProductAssetBrowserEntry::GetProductID() const
        {
            return m_productId;
        }

        AZ::s64 ProductAssetBrowserEntry::GetJobID() const
        {
            return m_jobId;
        }

        const AZ::Data::AssetId& ProductAssetBrowserEntry::GetAssetId() const
        {
            return m_assetId;
        }

        const AZ::Data::AssetType& ProductAssetBrowserEntry::GetAssetType() const
        {
            return m_assetType;
        }
        
        const AZStd::string& ProductAssetBrowserEntry::GetAssetTypeString() const
        {
            return m_assetTypeString;
        }

        ProductAssetBrowserEntry* ProductAssetBrowserEntry::GetProductByAssetId(const AZ::Data::AssetId& assetId)
        {
            if (EntryCache* cache = EntryCache::GetInstance())
            {
                // use find to avoid automatically inserting non-found products into the map as nullptr.
                auto found = cache->m_productAssetIdMap.find(assetId);
                if (found != cache->m_productAssetIdMap.end())
                {
                    return found->second;
                }
            }
            return nullptr;
        }

        void ProductAssetBrowserEntry::SetThumbnailDirty()
        {
            if (EntryCache* cache = EntryCache::GetInstance())
            {
                // if source is displaying product's thumbnail, then it needs to also SetThumbnailDirty
                if (m_parentAssetEntry)
                {
                    cache->m_dirtyThumbnailsSet.insert(m_parentAssetEntry);
                }
                cache->m_dirtyThumbnailsSet.insert(this);
            }
        }

        SharedThumbnailKey AssetBrowser::ProductAssetBrowserEntry::GetThumbnailKey() const
        {
            return AssetBrowserEntry::GetThumbnailKey();
        }

        SharedThumbnailKey ProductAssetBrowserEntry::CreateThumbnailKey()
        {
            return MAKE_TKEY(AzToolsFramework::AssetBrowser::ProductThumbnailKey, m_assetId);
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework
