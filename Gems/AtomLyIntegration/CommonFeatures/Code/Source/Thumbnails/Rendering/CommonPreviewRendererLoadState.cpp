/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Thumbnails/ThumbnailerBus.h"
#include <Thumbnails/Rendering/CommonThumbnailRenderer.h>
#include <Thumbnails/Rendering/ThumbnailRendererSteps/CaptureStep.h>
#include <Thumbnails/Rendering/ThumbnailRendererSteps/WaitForAssetsToLoadStep.h>

namespace AZ
{
    namespace LyIntegration
    {
        namespace Thumbnails
        {
            WaitForAssetsToLoadStep::WaitForAssetsToLoadStep(CommonThumbnailRenderer* renderer)
                : ThumbnailRendererStep(renderer)
            {
            }

            void WaitForAssetsToLoadStep::Start()
            {
                m_renderer->LoadAssets();
                m_timeRemainingS = TimeOutS;
                TickBus::Handler::BusConnect();
            }

            void WaitForAssetsToLoadStep::Stop()
            {
                TickBus::Handler::BusDisconnect();
            }

            void WaitForAssetsToLoadStep::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
            {
                m_timeRemainingS -= deltaTime;
                if (m_timeRemainingS > 0.0f)
                {
                    m_renderer->UpdateLoadAssets();
                }
                else
                {
                    m_renderer->CancelLoadAssets();
                }
            }
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ
