/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/PreviewRenderer/PreviewRenderer.h>
#include <AzCore/Component/TickBus.h>
#include <AzToolsFramework/Thumbnails/Thumbnail.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>
#include <Thumbnails/Thumbnail.h>

namespace AZ
{
    namespace LyIntegration
    {
        namespace Thumbnails
        {
            //! Provides custom rendering of material and model thumbnails
            class CommonThumbnailRenderer
                : public AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::MultiHandler
                , public SystemTickBus::Handler
            {
            public:
                AZ_CLASS_ALLOCATOR(CommonThumbnailRenderer, AZ::SystemAllocator, 0);

                CommonThumbnailRenderer();
                ~CommonThumbnailRenderer();

            private:
                //! ThumbnailerRendererRequestsBus::Handler interface overrides...
                void RenderThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey thumbnailKey, int thumbnailSize) override;
                bool Installed() const override;

                //! SystemTickBus::Handler interface overrides...
                void OnSystemTick() override;

                AtomToolsFramework::PreviewRenderer m_previewRenderer;
            };
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ
