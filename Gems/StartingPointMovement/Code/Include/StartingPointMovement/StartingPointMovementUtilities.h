/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzCore/Math/Vector3.h>
#include "StartingPointMovement/StartingPointMovementConstants.h"
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Transform.h>

namespace Movement
{
    //////////////////////////////////////////////////////////////////////////
    /// This will calculate an AZ::Transform based on an axis of rotation and an angle
    //////////////////////////////////////////////////////////////////////////
    static AZ::Transform CreateRotationFromAxis(AxisOfRotation rotationType, float radians)
    {
        static const AZ::Transform(*s_createRotation[3]) (const float) =
        {
            &AZ::Transform::CreateRotationX,
            &AZ::Transform::CreateRotationY,
            &AZ::Transform::CreateRotationZ
        };
        return s_createRotation[rotationType](radians);
    }
} //namespace Movement
