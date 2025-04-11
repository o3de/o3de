/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AssetBrowser/Thumbnails/ProductThumbnail.h>
#include <AssetBrowser/Thumbnails/SourceThumbnail.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <QtConcurrent/QtConcurrent>
#include <Source/ImageLoader/ImageLoaders.h>
#include <Source/Thumbnail/ImageThumbnail.h>

namespace ImageProcessingAtom
{
    namespace Thumbnails
    {
        static constexpr const int ImageThumbnailSize = 256;

        //////////////////////////////////////////////////////////////////////////
        // ImageThumbnail
        //////////////////////////////////////////////////////////////////////////
        ImageThumbnail::ImageThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey key)
            : Thumbnail(key)
        {
            auto sourceKey = azrtti_cast<const AzToolsFramework::AssetBrowser::SourceThumbnailKey*>(key.data());
            if (sourceKey)
            {
                bool foundIt = false;
                AZStd::vector<AZ::Data::AssetInfo> productAssetInfo;
                AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                    foundIt, &AzToolsFramework::AssetSystemRequestBus::Events::GetAssetsProducedBySourceUUID, sourceKey->GetSourceUuid(),
                    productAssetInfo);

                for (const auto& assetInfo : productAssetInfo)
                {
                    m_assetIds.insert(assetInfo.m_assetId);
                }
            }

            auto productKey = azrtti_cast<const AzToolsFramework::AssetBrowser::ProductThumbnailKey*>(key.data());
            if (productKey && productKey->GetAssetType() == AZ::RPI::StreamingImageAsset::RTTI_Type())
            {
                m_assetIds.insert(productKey->GetAssetId());
            }

            AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Handler::BusConnect(key);
            AzFramework::AssetCatalogEventBus::Handler::BusConnect();
        }

        ImageThumbnail::~ImageThumbnail()
        {
            AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Handler::BusDisconnect();
            AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
        }

        void ImageThumbnail::Load()
        {
            m_state = State::Loading;
            AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::QueueEvent(
                AZ::RPI::StreamingImageAsset::RTTI_Type(), &AzToolsFramework::Thumbnailer::ThumbnailerRendererRequests::RenderThumbnail,
                m_key, ImageThumbnailSize);
        }

        void ImageThumbnail::ThumbnailRendered(const QPixmap& thumbnailImage)
        {
            m_pixmap = thumbnailImage;
            m_state = State::Ready;
            QueueThumbnailUpdated();
        }

        void ImageThumbnail::ThumbnailFailedToRender()
        {
            m_state = State::Failed;
            QueueThumbnailUpdated();
        }

        void ImageThumbnail::OnCatalogAssetChanged([[maybe_unused]] const AZ::Data::AssetId& assetId)
        {
            if (m_assetIds.contains(assetId) && (m_state == State::Ready || m_state == State::Failed))
            {
                m_state = State::Unloaded;
                Load();
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // ImageThumbnailCache
        //////////////////////////////////////////////////////////////////////////
        ImageThumbnailCache::ImageThumbnailCache()
            : ThumbnailCache<ImageThumbnail>()
        {
        }

        ImageThumbnailCache::~ImageThumbnailCache() = default;

        int ImageThumbnailCache::GetPriority() const
        {
            // Image thumbnails override default source thumbnails, so carry higher priority
            return 1;
        }

        const char* ImageThumbnailCache::GetProviderName() const
        {
            return ProviderName;
        }

        bool ImageThumbnailCache::IsSupportedThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey key) const
        {
            auto sourceKey = azrtti_cast<const AzToolsFramework::AssetBrowser::SourceThumbnailKey*>(key.data());
            if (sourceKey)
            {
                bool foundIt = false;
                AZ::Data::AssetInfo assetInfo;
                AZStd::string watchFolder;
                AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                    foundIt, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourceUUID, sourceKey->GetSourceUuid(),
                    assetInfo, watchFolder);

                if (foundIt)
                {
                    AZStd::string ext;
                    AZ::StringFunc::Path::GetExtension(assetInfo.m_relativePath.c_str(), ext, false);
                    return IsExtensionSupported(ext.c_str());
                }
            }

            auto productKey = azrtti_cast<const AzToolsFramework::AssetBrowser::ProductThumbnailKey*>(key.data());
            return productKey && productKey->GetAssetType() == AZ::RPI::StreamingImageAsset::RTTI_Type();
        }
    } // namespace Thumbnails
} // namespace ImageProcessingAtom
