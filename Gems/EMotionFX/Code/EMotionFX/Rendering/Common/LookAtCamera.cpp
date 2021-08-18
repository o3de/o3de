/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        m_target = target;
        m_up     = up;
    }


    // update the camera
    void LookAtCamera::Update(float timeDelta)
    {
        MCORE_UNUSED(timeDelta);

        MCore::LookAtRH(m_viewMatrix, m_position, m_target, m_up);

        // update our base camera at the very end
        Camera::Update();
    }


    // reset the camera attributes
    void LookAtCamera::Reset(float flightTime)
    {
        MCORE_UNUSED(flightTime);

        // reset the base class attributes
        Camera::Reset();
        m_up.Set(0.0f, 0.0f, 1.0f);
    }
} // namespace MCommon
