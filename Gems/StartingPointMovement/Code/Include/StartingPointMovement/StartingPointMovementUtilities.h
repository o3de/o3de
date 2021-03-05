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
