/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Thumbnails/Rendering/CommonThumbnailRenderer.h>
#include <Thumbnails/Rendering/ThumbnailRendererSteps/FindThumbnailToRenderStep.h>

namespace AZ
{
    namespace LyIntegration
    {
        namespace Thumbnails
        {
            FindThumbnailToRenderStep::FindThumbnailToRenderStep(CommonThumbnailRenderer* renderer)
                : ThumbnailRendererStep(renderer)
            {
            }

            void FindThumbnailToRenderStep::Start()
            {
                TickBus::Handler::BusConnect();
            }

            void FindThumbnailToRenderStep::Stop()
            {
                TickBus::Handler::BusDisconnect();
            }

            void FindThumbnailToRenderStep::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] ScriptTimePoint time)
            {
                m_renderer->SelectThumbnail();
            }
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ
