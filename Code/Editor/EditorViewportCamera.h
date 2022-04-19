/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Quaternion.h>
#include <SandboxAPI.h>

namespace AZ
{
    class Transform;
    class Vector3;
} // namespace AZ

namespace SandboxEditor
{
    //! Set the default viewport camera translation/position.
    SANDBOX_API void SetDefaultViewportCameraPosition(const AZ::Vector3& position);

    //! Set the default viewport camera orientation/rotation.
    //! @param pitch Amount of pitch in radians.
    //! @param yaw Amount of yaw in radians.
    SANDBOX_API void SetDefaultViewportCameraRotation(float pitch, float yaw);

    //! Set the camera to interpolate to the given position and orientation.
    //! @param position The new position of the camera.
    //! @param pitch Amount of pitch in radians.
    //! @param yaw Amount of yaw in radians.
    SANDBOX_API void InterpolateDefaultViewportCameraToTransform(const AZ::Vector3& position, float pitch, float yaw);

    //! Get the default viewport camera transform.
    SANDBOX_API AZ::Transform GetDefaultViewportCameraTransform();

    //! Returns a quaternion representing a pitch/yaw rotation for a camera.
    //! @param pitch Amount of pitch in radians.
    //! @param yaw Amount of yaw in radians.
    inline AZ::Quaternion CameraRotation(const float pitch, const float yaw)
    {
        return AZ::Quaternion::CreateRotationZ(yaw) * AZ::Quaternion::CreateRotationX(pitch);
    }
} // namespace SandboxEditor
