/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ParticleMaterialThumbnail.h>

#include <AssetBrowser/Thumbnails/ProductThumbnail.h>
#include <AssetBrowser/Thumbnails/SourceThumbnail.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <QtConcurrent/QtConcurrent>
#include <Atom/RPI.Edit/Common/AssetUtils.h>

namespace OpenParticleSystem
{
    namespace Thumbnails
    {
        static constexpr int particlePriority = 5;
        ParticleMaterialThumbnail::ParticleMaterialThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey key)
            : Thumbnail(key)
        {
            AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Handler::BusConnect(key);
        }

        ParticleMaterialThumbnail::~ParticleMaterialThumbnail()
        {
            AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Handler::BusDisconnect();
        }

        void ParticleMaterialThumbnail::ThumbnailRendered(const QPixmap& thumbnailImage)
        {
            m_pixmap = thumbnailImage;
            m_state = State::Ready;
            m_renderWait.release();
        }

        void ParticleMaterialThumbnail::ThumbnailFailedToRender()
        {
            m_state = State::Failed;
            m_renderWait.release();
        }

        void ParticleMaterialThumbnail::Load()
        {
            if (m_state == State::Unloaded)
            {
                m_state = State::Loading;
                // LoadThread
                m_state = State::Loading;
                m_pixmap.load(":/AssetEditor/default_document.svg");
                m_state = m_pixmap.isNull() ? State::Failed : State::Ready;
                QueueThumbnailUpdated();
            }
        }

        ParticleMaterialThumbnailCache::ParticleMaterialThumbnailCache()
            : ThumbnailCache<ParticleMaterialThumbnail>()
        {
        }

        ParticleMaterialThumbnailCache::~ParticleMaterialThumbnailCache() = default;

        int ParticleMaterialThumbnailCache::GetPriority() const
        {
            // particle material thumbnails override default material thumbnails, so carry higher priority
            return particlePriority;
        }

        const char* ParticleMaterialThumbnailCache::GetProviderName() const
        {
            return ProviderName;
        }

        bool ParticleMaterialThumbnailCache::IsSupportedMaterial(const AZ::Data::Asset<AZ::RPI::MaterialAsset>& materialAsset) const
        {
            AZStd::string materialTypeAssetHint = materialAsset->GetMaterialTypeAsset().GetHint();
            AZStd::to_lower(materialTypeAssetHint.begin(),materialTypeAssetHint.end());
            return materialTypeAssetHint.find("particle") != AZStd::string::npos;
        }

        bool ParticleMaterialThumbnailCache::IsSupportedThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey key) const
        {
            if (auto sourceKey = azrtti_cast<const AzToolsFramework::AssetBrowser::SourceThumbnailKey*>(key.get()))
            {
                bool found = false;
                AZStd::vector<AZ::Data::AssetInfo> productAssetInfos;
                AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                    found, &AzToolsFramework::AssetSystemRequestBus::Events::GetAssetsProducedBySourceUUID, sourceKey->GetSourceUuid(),
                    productAssetInfos);

                for (const auto& assetInfo : productAssetInfos)
                {
                    if (assetInfo.m_assetType != AZ::RPI::MaterialAsset::RTTI_Type())
                    {
                        continue;
                    }
                    if (auto materialAsset = AZ::RPI::AssetUtils::LoadAsset<AZ::RPI::MaterialAsset>(assetInfo.m_assetId))
                    {
                        return IsSupportedMaterial(materialAsset.GetValue());
                    }
                }
            }

            if (auto productKey = azrtti_cast<const AzToolsFramework::AssetBrowser::ProductThumbnailKey*>(key.get()))
            {
                if (productKey->GetAssetType() != AZ::RPI::MaterialAsset::RTTI_Type())
                {
                    return false;
                }
                auto assetId = productKey->GetAssetId();
                if (auto materialAsset = AZ::RPI::AssetUtils::LoadAsset<AZ::RPI::MaterialAsset>(assetId))
                {
                    return IsSupportedMaterial(materialAsset.GetValue());
                }
            }
            return false;
        }
    } // namespace Thumbnails
} // namespace OpenParticleSystem
