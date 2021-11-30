/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Viewport/ViewportInteractionImpl.h>
#include <AzFramework/Viewport/ViewportScreen.h>

namespace AtomToolsFramework
{
    ViewportInteractionImpl::ViewportInteractionImpl(AZ::RPI::ViewPtr viewPtr)
        : m_viewPtr(AZStd::move(viewPtr))
    {
    }

    void ViewportInteractionImpl::Connect(const AzFramework::ViewportId viewportId)
    {
        AzToolsFramework::ViewportInteraction::ViewportInteractionRequestBus::Handler::BusConnect(viewportId);
        AZ::RPI::ViewportContextIdNotificationBus::Handler::BusConnect(viewportId);
    }

    void ViewportInteractionImpl::Disconnect()
    {
        AZ::RPI::ViewportContextIdNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::ViewportInteraction::ViewportInteractionRequestBus::Handler::BusDisconnect();
    }

    AzFramework::CameraState ViewportInteractionImpl::GetCameraState()
    {
        // build camera state from atom camera transforms
        AzFramework::CameraState cameraState =
            AzFramework::CreateDefaultCamera(m_viewPtr->GetCameraTransform(), AzFramework::Vector2FromScreenSize(m_screenSizeFn()));
        AzFramework::SetCameraClippingVolumeFromPerspectiveFovMatrixRH(cameraState, m_viewPtr->GetViewToClipMatrix());
        return cameraState;
    }

    AzFramework::ScreenPoint ViewportInteractionImpl::ViewportWorldToScreen(const AZ::Vector3& worldPosition)
    {
        return AzFramework::WorldToScreen(worldPosition, GetCameraState());
    }

    AZ::Vector3 ViewportInteractionImpl::ViewportScreenToWorld(const AzFramework::ScreenPoint& screenPosition)
    {
        return AzFramework::ScreenToWorld(screenPosition, GetCameraState());
    }

    AzToolsFramework::ViewportInteraction::ProjectedViewportRay ViewportInteractionImpl::ViewportScreenToWorldRay(
        const AzFramework::ScreenPoint& screenPosition)
    {
        return AzToolsFramework::ViewportInteraction::ViewportScreenToWorldRay(GetCameraState(), screenPosition);
    }

    float ViewportInteractionImpl::DeviceScalingFactor()
    {
        return m_deviceScalingFactorFn();
    }

    void ViewportInteractionImpl::OnViewportDefaultViewChanged(AZ::RPI::ViewPtr view)
    {
        m_viewPtr = AZStd::move(view);
    }
} // namespace AtomToolsFramework
