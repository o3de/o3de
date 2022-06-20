/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// extended constructor
MCORE_INLINE DualQuaternion::DualQuaternion(const AZ::Quaternion& rotation, const AZ::Vector3& translation)
    : m_real(rotation)
{
    m_dual = 0.5f * (AZ::Quaternion(translation.GetX(), translation.GetY(), translation.GetZ(), 0.0f) * rotation);
}



// transform a 3D point with the dual quaternion
MCORE_INLINE AZ::Vector3 DualQuaternion::TransformPoint(const AZ::Vector3& point) const
{
    const AZ::Vector3 realVector(m_real.GetX(), m_real.GetY(), m_real.GetZ());
    const AZ::Vector3 dualVector(m_dual.GetX(), m_dual.GetY(), m_dual.GetZ());
    const AZ::Vector3 position      = point + 2.0f * (realVector.Cross(realVector.Cross(point) + (m_real.GetW() * point)));
    const AZ::Vector3 displacement  = 2.0f * (m_real.GetW() * dualVector - m_dual.GetW() * realVector + realVector.Cross(dualVector));
    return position + displacement;
}


// transform a vector with this dual quaternion
MCORE_INLINE AZ::Vector3 DualQuaternion::TransformVector(const AZ::Vector3& v) const
{
    const AZ::Vector3 realVector(m_real.GetX(), m_real.GetY(), m_real.GetZ());
    const AZ::Vector3 dualVector(m_dual.GetX(), m_dual.GetY(), m_dual.GetZ());
    return v + 2.0f * (realVector.Cross(realVector.Cross(v) + m_real.GetW() * v));
}

