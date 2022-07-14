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
    //! PreviewRendererCaptureState renders a preview to an image
    class PreviewRendererCaptureState final
        : public PreviewRendererState
        , public AZ::SystemTickBus::Handler
        , public AZ::Render::FrameCaptureNotificationBus::Handler
    {
    public:
        PreviewRendererCaptureState(PreviewRenderer* renderer);
        ~PreviewRendererCaptureState();

    private:
        //! AZ::SystemTickBus::Handler interface overrides...
        void OnSystemTick() override;

        //! AZ::Render::FrameCaptureNotificationBus::Handler overrides...
        void OnFrameCaptureFinished(AZ::Render::FrameCaptureResult result, const AZStd::string& info) override;

        //! This is necessary to suspend capture until preview scene is ready
        int m_ticksToCapture = 1;
    };
} // namespace AtomToolsFramework
