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

#include "LookAtCamera.h"
#include <MCore/Source/AzCoreConversions.h>


namespace MCommon
{
    // constructor
    LookAtCamera::LookAtCamera()
        : Camera()
    {
        Reset();
    }


    // destructor
    LookAtCamera::~LookAtCamera()
    {
    }


    // look at target
    void LookAtCamera::LookAt(const AZ::Vector3& target, const AZ::Vector3& up)
    {
        mTarget = target;
        mUp     = up;
    }


    // update the camera
    void LookAtCamera::Update(float timeDelta)
    {
        MCORE_UNUSED(timeDelta);

        MCore::LookAtRH(mViewMatrix, mPosition, mTarget, mUp);

        // update our base camera at the very end
        Camera::Update();
    }


    // reset the camera attributes
    void LookAtCamera::Reset(float flightTime)
    {
        MCORE_UNUSED(flightTime);

        // reset the base class attributes
        Camera::Reset();
        mUp.Set(0.0f, 0.0f, 1.0f);
    }
} // namespace MCommon
