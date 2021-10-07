/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Thumbnails/Rendering/CommonPreviewRenderer.h>
#include <Thumbnails/Rendering/CommonPreviewRendererIdleState.h>

namespace AZ
{
    namespace LyIntegration
    {
        namespace Thumbnails
        {
            CommonPreviewRendererIdleState::CommonPreviewRendererIdleState(CommonPreviewRenderer* renderer)
                : CommonPreviewRendererState(renderer)
            {
            }

            void CommonPreviewRendererIdleState::Start()
            {
                TickBus::Handler::BusConnect();
            }

            void CommonPreviewRendererIdleState::Stop()
            {
                TickBus::Handler::BusDisconnect();
            }

            void CommonPreviewRendererIdleState::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] ScriptTimePoint time)
            {
                m_renderer->SelectThumbnail();
            }
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ
