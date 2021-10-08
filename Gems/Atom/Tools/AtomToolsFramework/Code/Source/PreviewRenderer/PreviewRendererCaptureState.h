/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/Utils/FrameCaptureBus.h>
#include <AzCore/Component/TickBus.h>
#include <Thumbnails/Rendering/CommonPreviewRendererState.h>

namespace AZ
{
    namespace LyIntegration
    {
        namespace Thumbnails
        {
            //! CommonPreviewRendererCaptureState renders a thumbnail to a pixmap and notifies MaterialOrModelThumbnail once finished
            class CommonPreviewRendererCaptureState
                : public CommonPreviewRendererState
                , private TickBus::Handler
                , private Render::FrameCaptureNotificationBus::Handler
            {
            public:
                CommonPreviewRendererCaptureState(CommonPreviewRenderer* renderer);

                void Start() override;
                void Stop() override;

            private:
                //! AZ::TickBus::Handler interface overrides...
                void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

                //! Render::FrameCaptureNotificationBus::Handler overrides...
                void OnCaptureFinished(Render::FrameCaptureResult result, const AZStd::string& info) override;

                //! This is necessary to suspend capture to allow a frame for Material and Mesh components to assign materials
                int m_ticksToCapture = 0;
            };
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ

