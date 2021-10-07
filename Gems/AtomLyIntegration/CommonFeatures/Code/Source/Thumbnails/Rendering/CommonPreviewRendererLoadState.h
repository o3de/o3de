/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Thumbnails/Rendering/ThumbnailRendererSteps/ThumbnailRendererStep.h>

namespace AZ
{
    namespace LyIntegration
    {
        namespace Thumbnails
        {
            //! WaitForAssetsToLoadStep pauses further rendering until all assets used for rendering a thumbnail have been loaded
            class WaitForAssetsToLoadStep
                : public ThumbnailRendererStep
                , private TickBus::Handler
            {
            public:
                WaitForAssetsToLoadStep(CommonThumbnailRenderer* renderer);

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

