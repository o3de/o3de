/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>
#include <AzFramework/Viewport/ViewportId.h>
#include <SandboxAPI.h>

namespace SandboxEditor
{
    //! Sets the specified viewport camera translation/position.
    //! @param viewportId The id of the viewport to modify.
    //! @param position The new position of the camera in world space.
    SANDBOX_API void SetViewportCameraPosition(AzFramework::ViewportId viewportId, const AZ::Vector3& position);

    //! Sets the default viewport camera translation/position.
    //! @param position The new position of the camera in world space.
    SANDBOX_API void SetDefaultViewportCameraPosition(const AZ::Vector3& position);

    //! Sets the specified viewport camera orientation/rotation.
    //! @param pitch Amount of pitch in radians.
    //! @param yaw Amount of yaw in radians.
    SANDBOX_API void SetViewportCameraRotation(AzFramework::ViewportId viewportId, float pitch, float yaw);

    //! Sets the default viewport camera orientation/rotation.
    //! @param pitch Amount of pitch in radians.
    //! @param yaw Amount of yaw in radians.
    SANDBOX_API void SetDefaultViewportCameraRotation(float pitch, float yaw);

    //! Sets the specified viewport camera translation/position and orientation/rotation.
    //! @param position The new position of the camera in world space.
    //! @param pitch Amount of pitch in radians.
    //! @param yaw Amount of yaw in radians.
    SANDBOX_API void SetViewportCameraTransform(AzFramework::ViewportId viewportId, const AZ::Vector3& position, float pitch, float yaw);

    //! Sets the default viewport camera translation/position and orientation/rotation.
    //! @param position The new position of the camera in world space.
    //! @param pitch Amount of pitch in radians.
    //! @param yaw Amount of yaw in radians.
    SANDBOX_API void SetDefaultViewportCameraTransform(const AZ::Vector3& position, float pitch, float yaw);

    //! Sets the specified viewport camera transform.
    //! @param transform The transform of the camera in world space.
    SANDBOX_API void SetViewportCameraTransform(AzFramework::ViewportId viewportId, const AZ::Transform& transform);

    //! Sets the default viewport camera transform.
    //! @param transform The transform of the camera in world space.
    SANDBOX_API void SetDefaultViewportCameraTransform(const AZ::Transform& transform);

    //! Sets the specified viewport camera to interpolate to the given position and orientation.
    //! @param position The new position of the camera in world space.
    //! @param pitch Amount of pitch in radians.
    //! @param yaw Amount of yaw in radians.
    //! @param duration Time (in seconds) the interpolation should last.
    SANDBOX_API void InterpolateViewportCameraToTransform(
        AzFramework::ViewportId viewportId, const AZ::Vector3& position, float pitch, float yaw, float duration);

    //! Sets the default viewport camera to interpolate to the given position and orientation.
    //! @param position The new position of the camera in world space.
    //! @param pitch Amount of pitch in radians.
    //! @param yaw Amount of yaw in radians.
    //! @param duration Time (in seconds) the interpolation should last.
    SANDBOX_API void InterpolateDefaultViewportCameraToTransform(const AZ::Vector3& position, float pitch, float yaw, float duration);

    //! Sets the specified viewport camera to interpolate to the given transform.
    //! @param transform The transform of the camera in world space.
    //! @param duration Time (in seconds) the interpolation should last.
    SANDBOX_API void InterpolateViewportCameraToTransform(
        AzFramework::ViewportId viewportId, const AZ::Transform& transform, float duration);

    //! Sets the default viewport camera to interpolate to the given transform.
    //! @param transform The transform of the camera in world space.
    //! @param duration Time (in seconds) the interpolation should last.
    SANDBOX_API void InterpolateDefaultViewportCameraToTransform(const AZ::Transform& transform, float duration);

    //! Gets the specified viewport camera transform in world space.
    SANDBOX_API AZ::Transform GetViewportCameraTransform(AzFramework::ViewportId viewportId);

    //! Gets the default viewport camera transform in world space.
    SANDBOX_API AZ::Transform GetDefaultViewportCameraTransform();

    //! Will call either Set or Interpolate camera transform depending on user setting.
    //! @param position The new position of the camera in world space.
    //! @param pitch Amount of pitch in radians.
    //! @param yaw Amount of yaw in radians.
    SANDBOX_API void HandleViewportCameraTransitionFromSetting(
        AzFramework::ViewportId viewportId, const AZ::Vector3& position, float pitch, float yaw);

    //! Will call either Set or Interpolate camera transform depending on user setting.
    //! @param position The new position of the camera in world space.
    //! @param pitch Amount of pitch in radians.
    //! @param yaw Amount of yaw in radians.
    SANDBOX_API void HandleDefaultViewportCameraTransitionFromSetting(const AZ::Vector3& position, float pitch, float yaw);

    //! Will call either Set or Interpolate camera transform depending on user setting.
    //! @param transform The transform of the camera in world space.
    SANDBOX_API void HandleViewportCameraTransitionFromSetting(
        AzFramework::ViewportId viewportId, const AZ::Transform& transform);

    //! Will call either Set or Interpolate camera transform depending on user setting.
    //! @param transform The transform of the camera in world space.
    SANDBOX_API void HandleDefaultViewportCameraTransitionFromSetting(const AZ::Transform& transform);

    //! Returns a transform that will aim to have the entity fill the screen (determined by the current
    //! camera transform, field of view and position and radius of the entity).
    //! @param cameraTransform The current transform of the camera.
    //! @param fovRadians The field of view of the camera (in radians).
    //! @param center The entity position in world space.
    //! @param radius The radius of the bounding sphere of the entity (usually calculated from the entity Aabb).
    //! @return Returns the new transform for the camera to fill the screen with the entity. If the cameraTransform
    //! and center match, an empty optional is returned.
    SANDBOX_API AZStd::optional<AZ::Transform> CalculateGoToEntityTransform(
        const AZ::Transform& cameraTransform, float fovRadians, const AZ::Vector3& center, float radius);

    //! Returns a quaternion representing a pitch/yaw rotation for a camera.
    //! @param pitch Amount of pitch in radians.
    //! @param yaw Amount of yaw in radians.
    inline AZ::Quaternion CameraRotation(const float pitch, const float yaw)
    {
        return AZ::Quaternion::CreateRotationZ(yaw) * AZ::Quaternion::CreateRotationX(pitch);
    }

    //! Returns a transform representing a position and pitch/yaw rotation.
    //! @param position Position for the transform.
    //! @param pitch Amount of pitch in radians.
    //! @param yaw Amount of yaw in radians.
    inline AZ::Transform TransformFromPositionPitchYaw(const AZ::Vector3& position, const float pitch, const float yaw)
    {
        return AZ::Transform::CreateFromQuaternionAndTranslation(CameraRotation(pitch, yaw), position);
    }
} // namespace SandboxEditor
