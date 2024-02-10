/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/AssetBrowser/Thumbnails/ProductThumbnail.h>
#include <QPixmap>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        //////////////////////////////////////////////////////////////////////////
        // ProductThumbnailKey
        //////////////////////////////////////////////////////////////////////////
        ProductThumbnailKey::ProductThumbnailKey(const AZ::Data::AssetId& assetId)
            : ThumbnailKey()
            , m_assetId(assetId)
        {
            AZ::Data::AssetInfo info;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(info, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, m_assetId);
            m_assetType = info.m_assetType;
        }

        const AZ::Data::AssetId& ProductThumbnailKey::GetAssetId() const { return m_assetId; }

        const AZ::Data::AssetType& ProductThumbnailKey::GetAssetType() const { return m_assetType; }

        size_t ProductThumbnailKey::GetHash() const
        {
            return m_assetType.GetHash();
        }

        bool ProductThumbnailKey::Equals(const ThumbnailKey* other) const
        {
            if (!ThumbnailKey::Equals(other))
            {
                return false;
            }
            // products displayed in Asset Browser have icons based on asset type, so multiple different products with same asset type will have same thumbnail
            return m_assetId == azrtti_cast<const ProductThumbnailKey*>(other)->GetAssetId();
        }

        //////////////////////////////////////////////////////////////////////////
        // ProductThumbnail
        //////////////////////////////////////////////////////////////////////////
        static const char* DEFAULT_PRODUCT_ICON_PATH = "Icons/AssetBrowser/DefaultProduct_16.svg";

        ProductThumbnail::ProductThumbnail(Thumbnailer::SharedThumbnailKey key)
            : Thumbnail(key)
        {}

        void ProductThumbnail::Load() 
        {
            m_state = State::Loading;

            auto productKey = azrtti_cast<const ProductThumbnailKey*>(m_key.data());
            AZ_Assert(productKey, "Incorrect key type, excpected ProductThumbnailKey");

            QString iconPath;
            AZ::AssetTypeInfoBus::EventResult(iconPath, productKey->GetAssetType(), &AZ::AssetTypeInfo::GetBrowserIcon);
            if (!iconPath.isEmpty())
            {
                // is it an embedded resource or absolute path?
                bool isUsablePath = (iconPath.startsWith(":") || (!AzFramework::StringFunc::Path::IsRelative(iconPath.toUtf8().constData())));
                
                if (!isUsablePath)
                {
                    // getting here means it needs resolution.  Can we find the real path of the file?  This also searches in gems for sources.
                    bool foundIt = false;
                    AZStd::string watchFolder;
                    AZ::Data::AssetInfo assetInfo;
                    AssetSystemRequestBus::BroadcastResult(
                        foundIt, &AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, iconPath.toUtf8().constData(), assetInfo,
                        watchFolder);

                    if (foundIt)
                    {
                        // the absolute path is join(watchfolder, relativepath); // since its relative to the watch folder.
                        AZStd::string finalPath;
                        AzFramework::StringFunc::Path::Join(watchFolder.c_str(), assetInfo.m_relativePath.c_str(), finalPath);
                        iconPath = QString::fromUtf8(finalPath.c_str());
                    }
                }
            }
            else
            {
                // no pixmap specified - use default.
                iconPath = QString::fromUtf8(DEFAULT_PRODUCT_ICON_PATH);
            }

            m_pixmap.load(iconPath);
            m_state = m_pixmap.isNull() ? State::Failed : State::Ready;
            QueueThumbnailUpdated();
        }

        //////////////////////////////////////////////////////////////////////////
        // ProductThumbnailCache
        //////////////////////////////////////////////////////////////////////////
        ProductThumbnailCache::ProductThumbnailCache()
            : ThumbnailCache<ProductThumbnail>() {}

        ProductThumbnailCache::~ProductThumbnailCache() = default;

        const char* ProductThumbnailCache::GetProviderName() const
        {
            return ProviderName;
        }

        bool ProductThumbnailCache::IsSupportedThumbnail(Thumbnailer::SharedThumbnailKey key) const
        {
            return azrtti_istypeof<const ProductThumbnailKey*>(key.data());
        }

    } // namespace AssetBrowser
} // namespace AzToolsFramework

#include "AssetBrowser/Thumbnails/moc_ProductThumbnail.cpp"
