/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <AzFramework/Viewport/CameraState.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

namespace AtomToolsFramework
{
    //! A concrete implementation of the ViewportInteractionRequestBus.
    //! Primarily concerned with picking (screen to world and world to screen transformations).
    class ViewportInteractionImpl
        : public AzToolsFramework::ViewportInteraction::ViewportInteractionRequestBus::Handler
        , private AZ::RPI::ViewportContextIdNotificationBus::Handler
    {
    public:
        explicit ViewportInteractionImpl(AZ::RPI::ViewPtr viewPtr);

        void Connect(AzFramework::ViewportId viewportId);
        void Disconnect();

        // ViewportInteractionRequestBus overrides ...
        AzFramework::CameraState GetCameraState() override;
        AzFramework::ScreenPoint ViewportWorldToScreen(const AZ::Vector3& worldPosition) override;
        AZ::Vector3 ViewportScreenToWorld(const AzFramework::ScreenPoint& screenPosition) override;
        AzToolsFramework::ViewportInteraction::ProjectedViewportRay ViewportScreenToWorldRay(
            const AzFramework::ScreenPoint& screenPosition) override;
        float DeviceScalingFactor() override;

        AZStd::function<AzFramework::ScreenSize()> m_screenSizeFn; //! Callback to determine the screen size.
        AZStd::function<float()> m_deviceScalingFactorFn; //! Callback to determine the device scaling factor.

    private:
        // ViewportContextIdNotificationBus overrides ...
        void OnViewportDefaultViewChanged(AZ::RPI::ViewPtr view) override;

        AZ::RPI::ViewPtr m_viewPtr;
    };
} // namespace AtomToolsFramework
