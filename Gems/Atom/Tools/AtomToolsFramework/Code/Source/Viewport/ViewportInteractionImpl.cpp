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
    }

    void ViewportInteractionImpl::Disconnect()
    {
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
        const AzFramework::CameraState cameraState = GetCameraState();
        const AZ::Vector3 rayOrigin = AzFramework::ScreenToWorld(screenPosition, cameraState);
        const AZ::Vector3 rayDirection = (rayOrigin - cameraState.m_position).GetNormalized();
        return AzToolsFramework::ViewportInteraction::ProjectedViewportRay{ rayOrigin, rayDirection };
    }

    float ViewportInteractionImpl::DeviceScalingFactor()
    {
        return m_deviceScalingFactorFn();
    }
} // namespace AtomToolsFramework
