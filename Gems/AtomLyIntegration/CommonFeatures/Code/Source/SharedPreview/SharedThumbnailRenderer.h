/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>
#include <AzCore/Component/TickBus.h>
#include <AzToolsFramework/Thumbnails/Thumbnail.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>
#include <Thumbnails/Thumbnail.h>

namespace AZ
{
    namespace LyIntegration
    {
        //! Provides custom thumbnail rendering of supported asset types
        class SharedThumbnailRenderer final
            : public AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::MultiHandler
            , public SystemTickBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(SharedThumbnailRenderer, AZ::SystemAllocator);

            SharedThumbnailRenderer();
            ~SharedThumbnailRenderer();

        private:
            struct ThumbnailConfig
            {
                bool IsValid() const;
                Data::Asset<RPI::ModelAsset> m_modelAsset;
                Data::Asset<RPI::MaterialAsset> m_materialAsset;
                Data::Asset<RPI::AnyAsset> m_lightingAsset;
            };

            ThumbnailConfig GetThumbnailConfig(AzToolsFramework::Thumbnailer::SharedThumbnailKey thumbnailKey);

            //! ThumbnailerRendererRequestBus::Handler interface overrides...
            void RenderThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey thumbnailKey, int thumbnailSize) override;
            bool Installed() const override;

            //! SystemTickBus::Handler interface overrides...
            void OnSystemTick() override;

            // Default assets to be kept loaded and used for rendering if not overridden
            Data::Asset<RPI::AnyAsset> m_defaultLightingPresetAsset;
            Data::Asset<RPI::ModelAsset> m_defaultModelAsset;
            Data::Asset<RPI::MaterialAsset> m_defaultMaterialAsset;
            Data::Asset<RPI::MaterialAsset> m_reflectionMaterialAsset;
        };
    } // namespace LyIntegration
} // namespace AZ
