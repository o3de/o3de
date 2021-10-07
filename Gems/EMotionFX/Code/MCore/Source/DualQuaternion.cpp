/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include "DualQuaternion.h"
#include <AzCore/Math/Matrix3x3.h>


namespace MCore
{
    // calculate the inverse
    DualQuaternion& DualQuaternion::Inverse()
    {
        const float realLength = m_real.GetLength();
        const float dotProduct = m_real.Dot(m_dual);
        const float dualFactor = realLength - 2.0f * dotProduct;

        m_real.Set(-m_real.GetX() * realLength, -m_real.GetY() * realLength, -m_real.GetZ() * realLength, m_real.GetW() * realLength);
        m_dual.Set(-m_dual.GetX() * dualFactor, -m_dual.GetY() * dualFactor, -m_dual.GetZ() * dualFactor, m_dual.GetW() * dualFactor);

        return *this;
    }


    // calculate the inversed version
    DualQuaternion DualQuaternion::Inversed() const
    {
        const float realLength = m_real.GetLength();
        const float dotProduct = m_real.Dot(m_dual);
        const float dualFactor = realLength - 2.0f * dotProduct;
        return DualQuaternion(AZ::Quaternion(-m_real.GetX() * realLength, -m_real.GetY() * realLength, -m_real.GetZ() * realLength, m_real.GetW() * realLength),
            AZ::Quaternion(-m_dual.GetX() * dualFactor, -m_dual.GetY() * dualFactor, -m_dual.GetZ() * dualFactor, m_dual.GetW() * dualFactor));
    }


    // convert the dual quaternion to a matrix
    AZ::Transform DualQuaternion::ToTransform() const
    {
        const float sqLen   = m_real.Dot(m_real);
        const float x       = m_real.GetX();
        const float y       = m_real.GetY();
        const float z       = m_real.GetZ();
        const float w       = m_real.GetW();
        const float t0      = m_dual.GetW();
        const float t1      = m_dual.GetX();
        const float t2      = m_dual.GetY();
        const float t3      = m_dual.GetZ();

        AZ::Matrix3x3 matrix3x3;
        matrix3x3.SetElement(0, 0, w * w + x * x - y * y - z * z);
        matrix3x3.SetElement(0, 1, 2.0f * x * y - 2.0f * w * z);
        matrix3x3.SetElement(0, 2, 2.0f * x * z + 2.0f * w * y);
        matrix3x3.SetElement(1, 0, 2.0f * x * y + 2.0f * w * z);
        matrix3x3.SetElement(1, 1, w * w + y * y - x * x - z * z);
        matrix3x3.SetElement(1, 2, 2.0f * y * z - 2.0f * w * x);
        matrix3x3.SetElement(2, 0, 2.0f * x * z - 2.0f * w * y);
        matrix3x3.SetElement(2, 1, 2.0f * y * z + 2.0f * w * x);
        matrix3x3.SetElement(2, 2, w * w + z * z - x * x - y * y);

        AZ::Vector3 translation;
        translation.SetElement(0, -2.0f * t0 * x + 2.0f * w * t1 - 2.0f * t2 * z + 2.0f * y * t3);
        translation.SetElement(1, -2.0f * t0 * y + 2.0f * t1 * z - 2.0f * x * t3 + 2.0f * w * t2);
        translation.SetElement(2, -2.0f * t0 * z + 2.0f * x * t2 + 2.0f * w * t3 - 2.0f * t1 * y);

        const float invSqLen = 1.0f / sqLen;

        return AZ::Transform::CreateFromMatrix3x3AndTranslation(invSqLen * matrix3x3, invSqLen * translation);
    }


    // construct the dual quaternion from a given non-scaled matrix
    DualQuaternion DualQuaternion::ConvertFromTransform(const AZ::Transform& transform)
    {
        const AZ::Vector3 pos = transform.GetTranslation();
        const AZ::Quaternion rot = transform.GetRotation();
        return DualQuaternion(rot, pos);
    }


    // normalizes the dual quaternion
    DualQuaternion& DualQuaternion::Normalize()
    {
        const float length = m_real.GetLength();
        const float invLength = 1.0f / length;

        m_real.Set(m_real.GetX() * invLength, m_real.GetY() * invLength, m_real.GetZ() * invLength, m_real.GetW() * invLength);
        m_dual.Set(m_dual.GetX() * invLength, m_dual.GetY() * invLength, m_dual.GetZ() * invLength, m_dual.GetW() * invLength);
        m_dual += m_real * (m_real.Dot(m_dual) * -1.0f);
        return *this;
    }


    // convert back into rotation and translation
    void DualQuaternion::ToRotationTranslation(AZ::Quaternion* outRot, AZ::Vector3* outPos) const
    {
        const float invLength = 1.0f / m_real.GetLength();
        *outRot = m_real * invLength;
        outPos->Set(2.0f * (-m_dual.GetW() * m_real.GetX() + m_dual.GetX() * m_real.GetW() - m_dual.GetY() * m_real.GetZ() + m_dual.GetZ() * m_real.GetY()) * invLength,
            2.0f * (-m_dual.GetW() * m_real.GetY() + m_dual.GetX() * m_real.GetZ() + m_dual.GetY() * m_real.GetW() - m_dual.GetZ() * m_real.GetX()) * invLength,
            2.0f * (-m_dual.GetW() * m_real.GetZ() - m_dual.GetX() * m_real.GetY() + m_dual.GetY() * m_real.GetX() + m_dual.GetZ() * m_real.GetW()) * invLength);
    }


    // special case version for conversion into rotation and translation
    // only works with normalized dual quaternions
    void DualQuaternion::NormalizedToRotationTranslation(AZ::Quaternion* outRot, AZ::Vector3* outPos) const
    {
        *outRot = m_real;
        outPos->Set(2.0f * (-m_dual.GetW() * m_real.GetX() + m_dual.GetX() * m_real.GetW() - m_dual.GetY() * m_real.GetZ() + m_dual.GetZ() * m_real.GetY()),
            2.0f * (-m_dual.GetW() * m_real.GetY() + m_dual.GetX() * m_real.GetZ() + m_dual.GetY() * m_real.GetW() - m_dual.GetZ() * m_real.GetX()),
            2.0f * (-m_dual.GetW() * m_real.GetZ() - m_dual.GetX() * m_real.GetY() + m_dual.GetY() * m_real.GetX() + m_dual.GetZ() * m_real.GetW()));
    }


    // convert into a dual quaternion from a translation and rotation
    DualQuaternion DualQuaternion::ConvertFromRotationTranslation(const AZ::Quaternion& rotation, const AZ::Vector3& translation)
    {
        return DualQuaternion(rotation,  0.5f * (AZ::Quaternion(translation.GetX(), translation.GetY(), translation.GetZ(), 0.0f) * rotation));
    }
}   // namespace MCore
