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

#pragma once

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

    //! Get the default viewport camera transform.
    SANDBOX_API AZ::Transform GetDefaultViewportCameraTransform();
} // namespace SandboxEditor
