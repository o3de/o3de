/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Thumbnails/Rendering/CommonPreviewRenderer.h>
#include <Thumbnails/Rendering/CommonPreviewRendererCaptureState.h>

namespace AZ
{
    namespace LyIntegration
    {
        namespace Thumbnails
        {
            CommonPreviewRendererCaptureState::CommonPreviewRendererCaptureState(CommonPreviewRenderer* renderer)
                : CommonPreviewRendererState(renderer)
            {
            }

            void CommonPreviewRendererCaptureState::Start()
            {
                m_ticksToCapture = 1;
                m_renderer->UpdateScene();
                TickBus::Handler::BusConnect();
            }

            void CommonPreviewRendererCaptureState::Stop()
            {
                m_renderer->EndCapture();
                TickBus::Handler::BusDisconnect();
                Render::FrameCaptureNotificationBus::Handler::BusDisconnect();
            }

            void CommonPreviewRendererCaptureState::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] ScriptTimePoint time)
            {
                if (m_ticksToCapture-- <= 0)
                {
                    // Reset the capture flag if the capture request was successful. Otherwise try capture it again next tick.
                    if (m_renderer->StartCapture())
                    {
                        Render::FrameCaptureNotificationBus::Handler::BusConnect();
                        TickBus::Handler::BusDisconnect();
                    }
                }
            }

            void CommonPreviewRendererCaptureState::OnCaptureFinished(
                [[maybe_unused]] Render::FrameCaptureResult result, [[maybe_unused]] const AZStd::string& info)
            {
                m_renderer->CompleteThumbnail();
            }
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ
