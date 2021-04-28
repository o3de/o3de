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
        const int ImageThumbnailSize = 200;

        //////////////////////////////////////////////////////////////////////////
        // ImageThumbnail
        //////////////////////////////////////////////////////////////////////////
        ImageThumbnail::ImageThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey key, int thumbnailSize)
            : Thumbnail(key, thumbnailSize)
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

        void ImageThumbnail::LoadThread()
        {
            AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::QueueEvent(
                AZ::RPI::StreamingImageAsset::RTTI_Type(), &AzToolsFramework::Thumbnailer::ThumbnailerRendererRequests::RenderThumbnail,
                m_key,
                m_thumbnailSize);
            // wait for response from thumbnail renderer
            m_renderWait.acquire();
        }

        void ImageThumbnail::ThumbnailRendered(QPixmap& thumbnailImage)
        {
            m_pixmap = thumbnailImage;
            m_renderWait.release();
        }

        void ImageThumbnail::ThumbnailFailedToRender()
        {
            m_state = State::Failed;
            m_renderWait.release();
        }

        void ImageThumbnail::OnCatalogAssetChanged([[maybe_unused]] const AZ::Data::AssetId& assetId)
        {
            if (m_state == State::Ready && m_assetIds.find(assetId) != m_assetIds.end())
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
