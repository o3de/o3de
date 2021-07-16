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
#include <Thumbnails/Rendering/ThumbnailRendererContext.h>
#include <Thumbnails/Rendering/ThumbnailRendererData.h>

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
        namespace Thumbnails
        {
            class ThumbnailRendererStep;

            //! Provides custom rendering of material and model thumbnails
            class CommonThumbnailRenderer
                : public ThumbnailRendererContext
                , private AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::MultiHandler
                , private SystemTickBus::Handler
                , private ThumbnailFeatureProcessorProviderBus::Handler
            {
            public:
                AZ_CLASS_ALLOCATOR(CommonThumbnailRenderer, AZ::SystemAllocator, 0)

                CommonThumbnailRenderer();
                ~CommonThumbnailRenderer();

                //! ThumbnailRendererContext overrides...
                void SetStep(Step step) override;
                Step GetStep() const override;
                AZStd::shared_ptr<ThumbnailRendererData> GetData() const override;

            private:
                //! ThumbnailerRendererRequestsBus::Handler interface overrides...
                void RenderThumbnail(AzToolsFramework::Thumbnailer::SharedThumbnailKey thumbnailKey, int thumbnailSize) override;
                bool Installed() const override;

                //! SystemTickBus::Handler interface overrides...
                void OnSystemTick() override;

                //! Render::ThumbnailFeatureProcessorProviderBus::Handler interface overrides...
                const AZStd::vector<AZStd::string>& GetCustomFeatureProcessors() const override;

                AZStd::unordered_map<Step, AZStd::shared_ptr<ThumbnailRendererStep>> m_steps;
                Step m_currentStep = Step::None;
                AZStd::shared_ptr<ThumbnailRendererData> m_data;
                AZStd::vector<AZStd::string> m_minimalFeatureProcessors;
            };
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ
