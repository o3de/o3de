/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorViewportCamera.h>

#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <AtomToolsFramework/Viewport/ModularViewportCameraControllerRequestBus.h>

#include <EditorViewportSettings.h>

namespace SandboxEditor
{
    void SetViewportCameraPosition(const AzFramework::ViewportId viewportId, const AZ::Vector3& position)
    {
        const auto& currentCameraTransform = GetViewportCameraTransform(viewportId);
        SetViewportCameraTransform(
            viewportId, AZ::Transform::CreateFromQuaternionAndTranslation(currentCameraTransform.GetRotation(), position));
    }

    void SetDefaultViewportCameraPosition(const AZ::Vector3& position)
    {
        auto viewportContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        if (auto viewportContext = viewportContextManager->GetDefaultViewportContext())
        {
            SetViewportCameraPosition(viewportContext->GetId(), position);
        }
    }

    void SetViewportCameraRotation(const AzFramework::ViewportId viewportId, const float pitch, const float yaw)
    {
        const auto& currentCameraTransform = GetViewportCameraTransform(viewportId);
        SetViewportCameraTransform(viewportId, TransformFromPositionPitchYaw(currentCameraTransform.GetTranslation(), pitch, yaw));
    }

    void SetDefaultViewportCameraRotation(const float pitch, const float yaw)
    {
        auto viewportContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        if (auto viewportContext = viewportContextManager->GetDefaultViewportContext())
        {
            SetViewportCameraRotation(viewportContext->GetId(), pitch, yaw);
        }
    }

    void SetViewportCameraTransform(const AzFramework::ViewportId viewportId, const AZ::Transform& transform)
    {
        AzFramework::ViewportRequestBus::Event(viewportId, &AzFramework::ViewportRequestBus::Events::SetCameraTransform, transform);
    }

    void SetViewportCameraTransform(
        const AzFramework::ViewportId viewportId, const AZ::Vector3& position, const float pitch, const float yaw)
    {
        SetViewportCameraTransform(viewportId, TransformFromPositionPitchYaw(position, pitch, yaw));
    }

    void SetDefaultViewportCameraTransform(const AZ::Transform& transform)
    {
        auto viewportContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        if (auto viewportContext = viewportContextManager->GetDefaultViewportContext())
        {
            SetViewportCameraTransform(viewportContext->GetId(), transform);
        }
    }

    void SetDefaultViewportCameraTransform(const AZ::Vector3& position, const float pitch, const float yaw)
    {
        SetDefaultViewportCameraTransform(TransformFromPositionPitchYaw(position, pitch, yaw));
    }

    void InterpolateViewportCameraToTransform(
        const AzFramework::ViewportId viewportId, const AZ::Vector3& position, const float pitch, const float yaw, const float duration)
    {
        InterpolateViewportCameraToTransform(viewportId, TransformFromPositionPitchYaw(position, pitch, yaw), duration);
    }

    void InterpolateDefaultViewportCameraToTransform(const AZ::Vector3& position, const float pitch, const float yaw, const float duration)
    {
        InterpolateDefaultViewportCameraToTransform(TransformFromPositionPitchYaw(position, pitch, yaw), duration);
    }

    void InterpolateViewportCameraToTransform(
        const AzFramework::ViewportId viewportId, const AZ::Transform& transform, const float duration)
    {
        AtomToolsFramework::ModularViewportCameraControllerRequestBus::Event(
            viewportId,
            &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::InterpolateToTransform,
            transform,
            duration);
    }

    void InterpolateDefaultViewportCameraToTransform(const AZ::Transform& transform, const float duration)
    {
        auto viewportContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        if (auto viewportContext = viewportContextManager->GetDefaultViewportContext())
        {
            InterpolateViewportCameraToTransform(viewportContext->GetId(), transform, duration);
        }
    }

    void HandleViewportCameraTransitionFromSetting(
        const AzFramework::ViewportId viewportId, const AZ::Vector3& position, const float pitch, const float yaw)
    {
        HandleViewportCameraTransitionFromSetting(viewportId, TransformFromPositionPitchYaw(position, pitch, yaw));
    }

    void HandleDefaultViewportCameraTransitionFromSetting(
        const AZ::Vector3& position, const float pitch, const float yaw)
    {
        HandleDefaultViewportCameraTransitionFromSetting(TransformFromPositionPitchYaw(position, pitch, yaw));
    }

    void HandleViewportCameraTransitionFromSetting(
        const AzFramework::ViewportId viewportId, const AZ::Transform& transform)
    {
        if (CameraGoToPositionInstantlyEnabled())
        {
            SetViewportCameraTransform(viewportId, transform);
        }
        else
        {
            InterpolateViewportCameraToTransform(viewportId, transform, CameraGoToPositionDuration());
        }
    }

    void HandleDefaultViewportCameraTransitionFromSetting(const AZ::Transform& transform)
    {
        auto viewportContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        if (auto viewportContext = viewportContextManager->GetDefaultViewportContext())
        {
            HandleViewportCameraTransitionFromSetting(viewportContext->GetId(), transform);
        }
    }

    AZ::Transform GetViewportCameraTransform(const AzFramework::ViewportId viewportId)
    {
        AZ::Transform cameraTransform = AZ::Transform::CreateIdentity();
        AzFramework::ViewportRequestBus::EventResult(
            cameraTransform, viewportId, &AzFramework::ViewportRequestBus::Events::GetCameraTransform);
        return cameraTransform;
    }

    AZ::Transform GetDefaultViewportCameraTransform()
    {
        auto viewportContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        if (auto viewportContext = viewportContextManager->GetDefaultViewportContext())
        {
            return GetViewportCameraTransform(viewportContext->GetId());
        }

        AZ_WarningOnce("EditorViewport", false, "Default viewport camera not found");

        return AZ::Transform::CreateIdentity();
    }
} // namespace SandboxEditor
