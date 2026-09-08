/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzFramework/Physics/Common/PhysicsTypes.h>

namespace PhysX
{
    //! Sweep the free editor camera through a physics scene, sliding along blocking surfaces.
    AZ::Vector3 ResolveCameraCollision(
        AzPhysics::SceneHandle sceneHandle, const AZ::Vector3& previousPosition, const AZ::Vector3& desiredPosition);
} // namespace PhysX
