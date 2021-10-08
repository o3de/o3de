/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TickBus.h>
#include <Thumbnails/Rendering/CommonPreviewRendererState.h>

namespace AZ
{
    namespace LyIntegration
    {
        namespace Thumbnails
        {
            //! CommonPreviewRendererLoadState pauses further rendering until all assets used for rendering a thumbnail have been loaded
            class CommonPreviewRendererLoadState
                : public CommonPreviewRendererState
                , private TickBus::Handler
            {
            public:
                CommonPreviewRendererLoadState(CommonPreviewRenderer* renderer);

                void Start() override;
                void Stop() override;

            private:
                //! AZ::TickBus::Handler interface overrides...
                void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

                static constexpr float TimeOutS = 5.0f;
                float m_timeRemainingS = TimeOutS;
            };
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ

