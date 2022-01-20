/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzFramework/Viewport/ScreenGeometry.h>

namespace AZ
{
    class Frustum;
    class Matrix3x4;
    class Matrix4x4;
    struct ViewFrustumAttributes;
} // namespace AZ

namespace AzFramework
{
    struct CameraState;
    struct ViewportInfo;

    //! Returns a position in screen space (in the range [0-viewportSize.x, 0-viewportSize.y]) from normalized device
    //! coordinates (in the range 0.0-1.0).
    inline ScreenPoint ScreenPointFromNdc(const AZ::Vector2& screenNdc, const AZ::Vector2& viewportSize)
    {
        return ScreenPoint(
            aznumeric_cast<int>(AZStd::lround(screenNdc.GetX() * viewportSize.GetX())),
            aznumeric_cast<int>(AZStd::lround((1.0f - screenNdc.GetY()) * viewportSize.GetY())));
    }

    //! Returns a position in normalized device coordinates (in the range [0.0-1.0, 0.0-1.0]) from a
    //! screen space position (in the range [0-viewportSize.x, 0-viewportSize.y]).
    inline AZ::Vector2 NdcFromScreenPoint(const ScreenPoint& screenPoint, const AZ::Vector2& viewportSize)
    {
        return AZ::Vector2(aznumeric_cast<float>(screenPoint.m_x), viewportSize.GetY() - aznumeric_cast<float>(screenPoint.m_y)) /
            viewportSize;
    }

    //! Projects a position in world space to screen space normalized device coordinates for the given camera.
    AZ::Vector3 WorldToScreenNdc(const AZ::Vector3& worldPosition, const AZ::Matrix3x4& cameraView, const AZ::Matrix4x4& cameraProjection);

    //! Projects a position in world space to screen space for the given camera.
    ScreenPoint WorldToScreen(const AZ::Vector3& worldPosition, const CameraState& cameraState);

    //! Overload of WorldToScreen that accepts camera values that can be precomputed if this function
    //! is called many times in a loop.
    ScreenPoint WorldToScreen(
        const AZ::Vector3& worldPosition,
        const AZ::Matrix3x4& cameraView,
        const AZ::Matrix4x4& cameraProjection,
        const AZ::Vector2& viewportSize);

    //! Unprojects a position in screen space pixel coordinates to world space.
    //! Note: The position returned will be on the near clip plane of the camera in world space.
    AZ::Vector3 ScreenToWorld(const ScreenPoint& screenPosition, const CameraState& cameraState);

    //! Overload of ScreenToWorld that accepts camera values that can be precomputed if this function
    //! is called many times in a loop.
    AZ::Vector3 ScreenToWorld(
        const ScreenPoint& screenPosition,
        const AZ::Matrix3x4& inverseCameraView,
        const AZ::Matrix4x4& inverseCameraProjection,
        const AZ::Vector2& viewportSize);

    //! Unprojects a position in screen space normalized device coordinates to world space.
    //! Note: The position returned will be on the near clip plane of the camera in world space.
    AZ::Vector3 ScreenNdcToWorld(
        const AZ::Vector2& ndcPosition, const AZ::Matrix3x4& inverseCameraView, const AZ::Matrix4x4& inverseCameraProjection);

    //! Returns the camera projection for the current camera state.
    AZ::Matrix4x4 CameraProjection(const CameraState& cameraState);

    //! Returns the inverse of the camera projection for the current camera state.
    AZ::Matrix4x4 InverseCameraProjection(const CameraState& cameraState);

    //! Returns the camera view for the current camera state.
    //! @note This is the 'v' in the MVP transform going from world space to view space (viewFromWorld).
    AZ::Matrix3x4 CameraView(const CameraState& cameraState);

    //! Returns the inverse of the camera view for the current camera state.
    //! @note This is the same as the CameraTransform but corrected for Z up.
    AZ::Matrix3x4 InverseCameraView(const CameraState& cameraState);

    //! Returns the camera transform for the current camera state.
    //! @note This is the inverse of 'v' in the MVP transform going from view space to world space (worldFromView).
    AZ::Matrix3x4 CameraTransform(const CameraState& cameraState);

    //! Takes a camera view (the world to camera space transform) and returns the
    //! corresponding camera transform (the world position and orientation of the camera).
    //! @note The parameter is the viewFromWorld transform (the 'v' in MVP) going from world space
    //! to view space. The return value is worldFromView transform going from view space to world space.
    AZ::Matrix3x4 CameraTransformFromCameraView(const AZ::Matrix3x4& cameraView);

    //! Takes a camera transform (the world position and orientation of the camera) and
    //! returns the corresponding camera view (to be used to transform from world to camera space).
    //! @note The parameter is the worldFromView transform going from view space to world space. The
    //! return value is viewFromWorld transform (the 'v' in MVP) going from view space to world space.
    AZ::Matrix3x4 CameraViewFromCameraTransform(const AZ::Matrix3x4& cameraTransform);

    //! Returns a frustum representing the camera transform and view volume in world space.
    AZ::Frustum FrustumFromCameraState(const CameraState& cameraState);

    //! Returns a structure representing frustum attributes for the current camera state.
    AZ::ViewFrustumAttributes ViewFrustumAttributesFromCameraState(const CameraState& cameraState);

    //! Returns the aspect ratio of the viewport dimensions.
    //! @note Ensure a valid viewport size is provided to this function.
    inline float AspectRatio(const AZ::Vector2& viewportSize)
    {
        AZ_Assert(viewportSize.GetY() > 0.0f, "AspectRatio called with invalid viewport size");
        return viewportSize.GetX() / viewportSize.GetY();
    }
} // namespace AzFramework
