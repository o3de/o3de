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

namespace SandboxEditor
{
    static AZ::Quaternion CameraRotation(const float pitch, const float yaw)
    {
        return AZ::Quaternion::CreateRotationZ(yaw) * AZ::Quaternion::CreateRotationX(pitch);
    }

    void SetDefaultViewportCameraPosition(const AZ::Vector3& position)
    {
        auto viewportContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        if (auto viewportContext = viewportContextManager->GetDefaultViewportContext())
        {
            const auto& currentCameraTransform = viewportContext->GetCameraTransform();
            viewportContext->SetCameraTransform(
                AZ::Transform::CreateFromQuaternionAndTranslation(currentCameraTransform.GetRotation(), position));
        }
    }

    void SetDefaultViewportCameraRotation(const float pitch, const float yaw)
    {
        auto viewportContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        if (auto viewportContext = viewportContextManager->GetDefaultViewportContext())
        {
            const auto& currentCameraTransform = viewportContext->GetCameraTransform();
            viewportContext->SetCameraTransform(
                AZ::Transform::CreateFromQuaternionAndTranslation(CameraRotation(pitch, yaw), currentCameraTransform.GetTranslation()));
        }
    }

    void InterpolateDefaultViewportCameraToTransform(const AZ::Vector3& position, const float pitch, const float yaw)
    {
        auto viewportContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        if (auto viewportContext = viewportContextManager->GetDefaultViewportContext())
        {
            AtomToolsFramework::ModularViewportCameraControllerRequestBus::Event(
                viewportContext->GetId(), &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::InterpolateToTransform,
                AZ::Transform::CreateFromQuaternionAndTranslation(CameraRotation(pitch, yaw), position));
        }
    }

    AZ::Transform GetDefaultViewportCameraTransform()
    {
        auto viewportContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        if (auto viewportContext = viewportContextManager->GetDefaultViewportContext())
        {
            return viewportContext->GetCameraTransform();
        }

        AZ_WarningOnce("EditorViewport", false, "Default viewport camera not found");

        return AZ::Transform::CreateIdentity();
    }
} // namespace SandboxEditor
