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
#include <PreviewRenderer/PreviewRendererState.h>

namespace AtomToolsFramework
{
    //! PreviewRendererCaptureState renders a thumbnail to a pixmap and notifies MaterialOrModelThumbnail once finished
    class PreviewRendererCaptureState final
        : public PreviewRendererState
        , public AZ::TickBus::Handler
        , public AZ::Render::FrameCaptureNotificationBus::Handler
    {
    public:
        PreviewRendererCaptureState(PreviewRenderer* renderer);
        ~PreviewRendererCaptureState();

    private:
        //! AZ::TickBus::Handler interface overrides...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        //! AZ::Render::FrameCaptureNotificationBus::Handler overrides...
        void OnCaptureFinished(AZ::Render::FrameCaptureResult result, const AZStd::string& info) override;

        //! This is necessary to suspend capture to allow a frame for Material and Mesh components to assign materials
        int m_ticksToCapture = 1;
    };
} // namespace AtomToolsFramework
