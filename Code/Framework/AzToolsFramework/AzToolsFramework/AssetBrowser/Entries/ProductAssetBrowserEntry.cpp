/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/Entries/ProductAssetBrowserEntry.h>
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>
#include <AzToolsFramework/AssetBrowser/Thumbnails/AssetBrowserProductThumbnail.h>
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

        void ProductAssetBrowserEntry::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<ProductAssetBrowserEntry, AssetBrowserEntry>()
                    ->Field("m_productId", &ProductAssetBrowserEntry::m_productId)
                    ->Field("m_jobId", &ProductAssetBrowserEntry::m_jobId)
                    ->Field("m_assetId", &ProductAssetBrowserEntry::m_assetId)
                    ->Field("m_assetType", &ProductAssetBrowserEntry::m_assetType)
                    ->Version(1);
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

        void ProductAssetBrowserEntry::ThumbnailUpdated()
        {
            // if source is displaying product's thumbnail, then it needs to also listen to its ThumbnailUpdated
            if (m_parentAssetEntry)
            {
                if (EntryCache* cache = EntryCache::GetInstance())
                {
                    cache->m_dirtyThumbnailsSet.insert(m_parentAssetEntry);
                }
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
