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

#include <QPixmap>
#include <QtConcurrent/QtConcurrent>

#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

#include <Atom/RPI.Public/Material/Material.h>

#include <Source/Material/Thumbnails/MaterialThumbnail.h>


namespace AZ
{
    namespace LyIntegration
    {
        using namespace AzToolsFramework::Thumbnailer;
        using namespace AzToolsFramework::AssetBrowser;

        const int MaterialThumbnailSize = 200;

        //////////////////////////////////////////////////////////////////////////
        // MaterialThumbnail
        //////////////////////////////////////////////////////////////////////////
        MaterialThumbnail::MaterialThumbnail(SharedThumbnailKey key, int thumbnailSize)
            : Thumbnail(key, thumbnailSize)
        {
            const SourceThumbnailKey* sourceThumbnailKey = azrtti_cast<const SourceThumbnailKey*>(m_key.data());
            if (!sourceThumbnailKey)
            {
                AZ_Error("MaterialThumbnail", sourceThumbnailKey, "Incorrect key type, excpected SourceThumbnailKey");
                m_state = State::Failed;
                return;
            }

            Data::AssetInfo assetInfo;
            AZStd::string watchFolder;
            bool hasResult = false;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(hasResult, &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath,
                sourceThumbnailKey->GetFileName().c_str(), assetInfo, watchFolder);

            if (!hasResult)
            {
                AZ_Error("MaterialThumbnail", hasResult, "AssetInfo for %s could not be found", sourceThumbnailKey->GetFileName().c_str());
                m_state = State::Failed;
                return;
            }

            m_assetType = AZ::AzTypeInfo<AZ::RPI::MaterialAsset>::Uuid();
            m_assetId = assetInfo.m_assetId;

            ThumbnailerRendererNotificationBus::Handler::BusConnect(m_assetId);
            AzFramework::AssetCatalogEventBus::Handler::BusConnect();
        }

        void MaterialThumbnail::LoadThread()
        {
            ThumbnailerRendererRequestBus::QueueEvent(m_assetType, &ThumbnailerRendererRequests::RenderThumbnail, m_assetId, m_thumbnailSize);
            // wait for response from thumbnail renderer
            m_renderWait.acquire();
        }

        MaterialThumbnail::~MaterialThumbnail()
        {
            ThumbnailerRendererNotificationBus::Handler::BusDisconnect(m_assetId);
            AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
        }

        void MaterialThumbnail::ThumbnailRendered(QPixmap& thumbnailImage)
        {
            m_pixmap = thumbnailImage;
            m_renderWait.release();
        }

        void MaterialThumbnail::ThumbnailFailedToRender()
        {
            m_state = State::Failed;
            m_renderWait.release();
        }

        void MaterialThumbnail::OnCatalogAssetChanged(const AZ::Data::AssetId& assetId)
        {
            if (m_assetId == assetId &&
                m_state == State::Ready)
            {
                m_state = State::Unloaded;
                Load();
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // MaterialThumbnailCache
        //////////////////////////////////////////////////////////////////////////
        MaterialThumbnailCache::MaterialThumbnailCache()
            : ThumbnailCache<MaterialThumbnail, SourceKeyHash, SourceKeyEqual>()
            , m_renderer(AZStd::make_unique<MaterialThumbnailRenderer>())
        {
        }

        MaterialThumbnailCache::~MaterialThumbnailCache() = default;

        int MaterialThumbnailCache::GetPriority() const
        {
            // Material thumbnails override default source thumbnails, so carry higher priority
            return 1;
        }

        const char* MaterialThumbnailCache::GetProviderName() const
        {
            return ProviderName;
        }

        bool MaterialThumbnailCache::IsSupportedThumbnail(SharedThumbnailKey key) const
        {
            auto sourceKey = azrtti_cast<const AzToolsFramework::AssetBrowser::SourceThumbnailKey*>(key.data());
            return sourceKey && sourceKey->GetExtension() == ".material";
        }
    } // namespace LyIntegration
} // namespace AZ

#include <Source/Material/Thumbnails/moc_MaterialThumbnail.cpp>
