/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
                AZ::Transform::CreateFromQuaternionAndTranslation(CameraRotation(pitch, yaw), position), 0.0f);
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
