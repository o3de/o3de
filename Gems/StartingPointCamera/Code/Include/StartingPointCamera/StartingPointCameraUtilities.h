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
#include "StartingPointCamera/StartingPointCameraConstants.h"
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Transform.h>

namespace Camera
{
    const char* GetNameFromUuid(const AZ::Uuid& uuid);

    //////////////////////////////////////////////////////////////////////////
    /// This methods will 0 out a vector component and re-normalize it
    //////////////////////////////////////////////////////////////////////////
    void MaskComponentFromNormalizedVector(AZ::Vector3& v, VectorComponentType vectorComponentType);

    //////////////////////////////////////////////////////////////////////////
    /// This will calculate the requested Euler angle from a given AZ::Quaternion
    //////////////////////////////////////////////////////////////////////////
    float GetEulerAngleFromTransform(const AZ::Transform& rotation, EulerAngleType eulerAngleType);

    //////////////////////////////////////////////////////////////////////////
    /// This will calculate an AZ::Transform based on an Euler angle
    //////////////////////////////////////////////////////////////////////////
    AZ::Transform CreateRotationFromEulerAngle(EulerAngleType rotationType, float radians);

    //////////////////////////////////////////////////////////////////////////
    /// Creates the Quaternion representing the rotation looking down the vector
    //////////////////////////////////////////////////////////////////////////
    AZ::Quaternion CreateQuaternionFromViewVector(const AZ::Vector3 lookVector);

} //namespace Camera
