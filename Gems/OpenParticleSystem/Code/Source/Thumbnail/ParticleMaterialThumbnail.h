/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once


#if !defined(Q_MOC_RUN)
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzToolsFramework/Thumbnails/Thumbnail.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#endif

namespace OpenParticleSystem
{
    namespace Thumbnails
    {
        class ParticleMaterialThumbnail
            : public AzToolsFramework::Thumbnailer::Thumbnail
            , public AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Handler
        {
            Q_OBJECT
        public:
            ParticleMaterialThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey key);
            ~ParticleMaterialThumbnail() override;

            // AzToolsFramework::ThumbnailerRendererNotificationBus::Handler overrides
            void ThumbnailRendered(const QPixmap& thumbnailImage) override;
            void ThumbnailFailedToRender() override;

            // Thumbnail overrides
            void Load() override;

        private:
            AZStd::binary_semaphore m_renderWait;
        };

        // Cache configuration for particle material thumbnails
        class ParticleMaterialThumbnailCache
            : public AzToolsFramework::Thumbnailer::ThumbnailCache<ParticleMaterialThumbnail>
        {
        public:
            ParticleMaterialThumbnailCache();
            ~ParticleMaterialThumbnailCache() override;

            int GetPriority() const override;
            const char* GetProviderName() const override;

            static constexpr const char* ProviderName = "Particle Material Thumbnails";

        protected:
            bool IsSupportedThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey key) const override;

        private:
            bool IsSupportedMaterial(const AZ::Data::Asset<AZ::RPI::MaterialAsset>& materialAsset) const;
        };
    } // namespace Thumbnails
} // namespace OpenParticleSystem
