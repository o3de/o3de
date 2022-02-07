/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>

namespace AZ
{
    class SerializeContext;
} // namespace AZ

namespace AzFramework
{
    //! Represents the camera state populated by the viewport camera.
    struct CameraState
    {
        //! @cond
        AZ_TYPE_INFO(CameraState, "{D309D934-044C-4BA8-91F1-EA3A45177A52}")
        CameraState() = default;
        //! @endcond

        static void Reflect(AZ::SerializeContext& context);

        //! Return the vertical fov of the camera when the view is in perspective.
        float VerticalFovRadian() const { return m_fovOrZoom; }
        //! Return the zoom amount of the camera when the view is in orthographic.
        float Zoom() const { return m_fovOrZoom; }

        AZ::Vector3 m_position = AZ::Vector3::CreateZero(); //!< World position of the camera.
        AZ::Vector3 m_forward = AZ::Vector3::CreateAxisY(); //!< Forward look direction of the camera (world space).
        AZ::Vector3 m_side = AZ::Vector3::CreateAxisX(); //!< Side vector of camera (orthogonal to forward and up).
        AZ::Vector3 m_up = AZ::Vector3::CreateAxisZ(); //!< Up vector of the camera (cameras frame - world space).
        AZ::Vector2 m_viewportSize = AZ::Vector2::CreateZero(); //!< Dimensions of the viewport.
        float m_nearClip = 0.01f; //!< Near clip plane of the camera.
        float m_farClip = 100.0f; //!< Far clip plane of the camera.
        float m_fovOrZoom = 0.0f; //!< Vertical fov or zoom of camera depending on if it is using orthographic projection or not.
        bool m_orthographic = false; //!< Is the camera using orthographic projection or not.
    };

    //! Create a camera at the given transform, specifying the near and far clip planes as well as the fov with a specific viewport size.
    CameraState CreateCamera(
        const AZ::Transform& transform, float nearPlane, float farPlane, float verticalFovRad, const AZ::Vector2& viewportSize);

    //! Create a camera at the given transform with a specific viewport size.
    //! @note The near/far clip planes and fov are sensible default values - please
    //! use SetCameraClippingVolume to override them.
    CameraState CreateDefaultCamera(const AZ::Transform& transform, const AZ::Vector2& viewportSize);

    //! Create a camera at the given position (no orientation) with a specific viewport size.
    //! @note The near/far clip planes and fov are sensible default values - please
    //! use SetCameraClippingVolume to override them.
    CameraState CreateIdentityDefaultCamera(const AZ::Vector3& position, const AZ::Vector2& viewportSize);

    //! Create a camera transformed by the given view to world matrix with a specific viewport size.
    //! @note The near/far clip planes and fov are sensible default values - please
    //! use SetCameraClippingVolume to override them.
    CameraState CreateCameraFromWorldFromViewMatrix(const AZ::Matrix4x4& worldFromView, const AZ::Vector2& viewportSize);

    //! Override the default near/far clipping planes and fov of the camera.
    void SetCameraClippingVolume(CameraState& cameraState, float nearPlane, float farPlane, float verticalFovRad);

    //! Override the default near/far clipping planes and fov of the camera by inferring them the specified right handed transform into clip space.
    void SetCameraClippingVolumeFromPerspectiveFovMatrixRH(CameraState& cameraState, const AZ::Matrix4x4& clipFromView);

    //! Retrieve the field of view (Fov) from the perspective projection matrix (view space to clip space).
    float RetrieveFov(const AZ::Matrix4x4& clipFromView);

    //! Set the transform for an existing camera.
    void SetCameraTransform(CameraState& cameraState, const AZ::Transform& transform);

} // namespace AzFramework
