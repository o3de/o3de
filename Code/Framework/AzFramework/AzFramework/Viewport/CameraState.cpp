/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CameraState.h"

#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AzFramework
{
    void SetCameraClippingVolume(
        AzFramework::CameraState& cameraState, const float nearPlane, const float farPlane, const float verticalFovRad)
    {
        cameraState.m_nearClip = nearPlane;
        cameraState.m_farClip = farPlane;
        cameraState.m_fovOrZoom = verticalFovRad;
    }

    void SetCameraTransform(CameraState& cameraState, const AZ::Transform& transform)
    {
        cameraState.m_side = transform.GetBasisX();
        cameraState.m_forward = transform.GetBasisY();
        cameraState.m_up = transform.GetBasisZ();
        cameraState.m_position = transform.GetTranslation();
    }

    static void SetDefaultCameraClippingVolume(AzFramework::CameraState& cameraState)
    {
        SetCameraClippingVolume(cameraState, 0.1f, 1000.0f, AZ::DegToRad(60.0f));
    }

    CameraState CreateCamera(
        const AZ::Transform& transform,
        const float nearPlane,
        const float farPlane,
        const float verticalFovRad,
        const AZ::Vector2& viewportSize)
    {
        AzFramework::CameraState cameraState;

        SetCameraTransform(cameraState, transform);
        SetCameraClippingVolume(cameraState, nearPlane, farPlane, verticalFovRad);
        cameraState.m_viewportSize = viewportSize;

        return cameraState;
    }

    AzFramework::CameraState CreateDefaultCamera(const AZ::Transform& transform, const AZ::Vector2& viewportSize)
    {
        AzFramework::CameraState cameraState;

        SetCameraTransform(cameraState, transform);
        SetDefaultCameraClippingVolume(cameraState);
        cameraState.m_viewportSize = viewportSize;

        return cameraState;
    }

    AzFramework::CameraState CreateIdentityDefaultCamera(const AZ::Vector3& position, const AZ::Vector2& viewportSize)
    {
        return CreateDefaultCamera(AZ::Transform::CreateTranslation(position), viewportSize);
    }

    AzFramework::CameraState CreateCameraFromWorldFromViewMatrix(const AZ::Matrix4x4& worldFromView, const AZ::Vector2& viewportSize)
    {
        AzFramework::CameraState cameraState;
        cameraState.m_viewportSize = viewportSize;

        cameraState.m_side = worldFromView.GetBasisXAsVector3();
        cameraState.m_forward = worldFromView.GetBasisYAsVector3();
        cameraState.m_up = worldFromView.GetBasisZAsVector3();
        cameraState.m_position = worldFromView.GetTranslation();

        SetDefaultCameraClippingVolume(cameraState);

        return cameraState;
    }

    void SetCameraClippingVolumeFromPerspectiveFovMatrixRH(CameraState& cameraState, const AZ::Matrix4x4& clipFromView)
    {
        const float m22 = clipFromView(2, 2);
        const float m23 = clipFromView(2, 3);
        cameraState.m_nearClip = m23 / m22;
        cameraState.m_farClip = m23 / (m22 + 1.f);
        // Check to see if the perspective matrix has reversed depth
        if (cameraState.m_nearClip > cameraState.m_farClip)
        {
            AZStd::swap(cameraState.m_nearClip, cameraState.m_farClip);
        }
        cameraState.m_fovOrZoom = RetrieveFov(clipFromView);
    }

    float RetrieveFov(const AZ::Matrix4x4& clipFromView)
    {
        return 2.0f * (AZ::Constants::HalfPi - AZStd::atan(clipFromView(1, 1)));
    }

    void CameraState::Reflect(AZ::SerializeContext& serializeContext)
    {
        serializeContext.Class<CameraState>()
            ->Field("Position", &CameraState::m_position)
            ->Field("Forward", &CameraState::m_forward)
            ->Field("Side", &CameraState::m_side)
            ->Field("Up", &CameraState::m_up)
            ->Field("ViewportSize", &CameraState::m_viewportSize)
            ->Field("NearClip", &CameraState::m_nearClip)
            ->Field("FarClip", &CameraState::m_farClip)
            ->Field("FovZoom", &CameraState::m_fovOrZoom)
            ->Field("Ortho", &CameraState::m_orthographic);
    }
} // namespace AzFramework
