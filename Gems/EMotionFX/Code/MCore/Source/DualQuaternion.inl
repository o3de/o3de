/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// extended constructor
MCORE_INLINE DualQuaternion::DualQuaternion(const AZ::Quaternion& rotation, const AZ::Vector3& translation)
    : mReal(rotation)
{
    mDual = 0.5f * (AZ::Quaternion(translation.GetX(), translation.GetY(), translation.GetZ(), 0.0f) * rotation);
}



// transform a 3D point with the dual quaternion
MCORE_INLINE AZ::Vector3 DualQuaternion::TransformPoint(const AZ::Vector3& point) const
{
    const AZ::Vector3 realVector(mReal.GetX(), mReal.GetY(), mReal.GetZ());
    const AZ::Vector3 dualVector(mDual.GetX(), mDual.GetY(), mDual.GetZ());
    const AZ::Vector3 position      = point + 2.0f * (realVector.Cross(realVector.Cross(point) + (mReal.GetW() * point)));
    const AZ::Vector3 displacement  = 2.0f * (mReal.GetW() * dualVector - mDual.GetW() * realVector + realVector.Cross(dualVector));
    return position + displacement;
}


// transform a vector with this dual quaternion
MCORE_INLINE AZ::Vector3 DualQuaternion::TransformVector(const AZ::Vector3& v) const
{
    const AZ::Vector3 realVector(mReal.GetX(), mReal.GetY(), mReal.GetZ());
    const AZ::Vector3 dualVector(mDual.GetX(), mDual.GetY(), mDual.GetZ());
    return v + 2.0f * (realVector.Cross(realVector.Cross(v) + mReal.GetW() * v));
}

