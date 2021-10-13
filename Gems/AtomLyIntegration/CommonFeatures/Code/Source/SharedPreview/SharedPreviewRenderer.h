/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Thumbnails/Thumbnail.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>
#include <SharedPreview/SharedPreviewRendererContext.h>
#include <SharedPreview/SharedPreviewRendererData.h>

#include <AtomLyIntegration/CommonFeatures/Thumbnails/ThumbnailFeatureProcessorProviderBus.h>

// Disables warning messages triggered by the Qt library
// 4251: class needs to have dll-interface to be used by clients of class
// 4800: forcing value to bool 'true' or 'false' (performance warning)
AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option")
#include <QPixmap>
AZ_POP_DISABLE_WARNING

namespace AZ
{
    namespace LyIntegration
    {
        class SharedPreviewRendererState;

        //! Provides custom rendering of material and model thumbnails
        class SharedPreviewRenderer
            : public SharedPreviewRendererContext
            , private AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::MultiHandler
            , private SystemTickBus::Handler
            , private ThumbnailFeatureProcessorProviderBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(SharedPreviewRenderer, AZ::SystemAllocator, 0)

            SharedPreviewRenderer();
            ~SharedPreviewRenderer();

            //! SharedPreviewRendererContext overrides...
            void SetState(State step) override;
            State GetState() const override;
            AZStd::shared_ptr<SharedPreviewRendererData> GetData() const override;

        private:
            //! ThumbnailerRendererRequestsBus::Handler interface overrides...
            void RenderThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey thumbnailKey, int thumbnailSize) override;
            bool Installed() const override;

            //! SystemTickBus::Handler interface overrides...
            void OnSystemTick() override;

            //! Render::ThumbnailFeatureProcessorProviderBus::Handler interface overrides...
            const AZStd::vector<AZStd::string>& GetCustomFeatureProcessors() const override;

            AZStd::unordered_map<State, AZStd::shared_ptr<SharedPreviewRendererState>> m_steps;
            State m_currentState = State::None;
            AZStd::shared_ptr<SharedPreviewRendererData> m_data;
            AZStd::vector<AZStd::string> m_minimalFeatureProcessors;
        };
    } // namespace LyIntegration
} // namespace AZ
