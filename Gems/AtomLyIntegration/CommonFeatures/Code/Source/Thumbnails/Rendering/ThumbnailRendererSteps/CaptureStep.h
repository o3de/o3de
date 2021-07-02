/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/Utils/FrameCaptureBus.h>
#include <AzCore/Component/TickBus.h>
#include <Thumbnails/Rendering/ThumbnailRendererSteps/ThumbnailRendererStep.h>

namespace AZ
{
    namespace LyIntegration
    {
        namespace Thumbnails
        {
            //! CaptureStep renders a thumbnail to a pixmap and notifies MaterialOrModelThumbnail once finished
            class CaptureStep
                : public ThumbnailRendererStep
                , private TickBus::Handler
                , private Render::FrameCaptureNotificationBus::Handler
            {
            public:
                CaptureStep(ThumbnailRendererContext* context);

                void Start() override;
                void Stop() override;

            private:
                //! Places the camera so that the entire model is visible
                void RepositionCamera() const;

                //! AZ::TickBus::Handler interface overrides...
                void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

                //! Render::FrameCaptureNotificationBus::Handler overrides...
                void OnCaptureFinished(Render::FrameCaptureResult result, const AZStd::string& info) override;
                
                static constexpr float DepthNear = 0.01f;
                static constexpr float StartingDistanceMultiplier = 1.75f;
                static constexpr float StartingRotationAngle = Constants::QuarterPi / 2.0f;

                //! This flag is needed to wait one frame after each frame capture to reset FrameCaptureSystemComponent
                bool m_readyToCapture = true;
                //! This is necessary to suspend capture to allow a frame for Material and Mesh components to assign materials
                int m_ticksToCapture = 0;
            };
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ

