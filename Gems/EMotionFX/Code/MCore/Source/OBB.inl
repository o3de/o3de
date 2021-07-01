/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// initialize the box
// this creates an invalid box (with negative extents) so the IsValid method will return false
MCORE_INLINE void OBB::Init()
{
    mCenter = AZ::Vector3::CreateZero();
    mExtents.Set(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    mRotation = AZ::Transform::CreateIdentity();
}


// check if the OBB is valid
MCORE_INLINE bool OBB::CheckIfIsValid() const
{
    if (mExtents.GetX() < 0.0f)
    {
        return false;
    }
    if (mExtents.GetY() < 0.0f)
    {
        return false;
    }
    if (mExtents.GetZ() < 0.0f)
    {
        return false;
    }
    return true;
}

