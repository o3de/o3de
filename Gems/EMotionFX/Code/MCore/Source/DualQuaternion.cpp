/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        const float realLength = mReal.GetLength();
        const float dotProduct = mReal.Dot(mDual);
        const float dualFactor = realLength - 2.0f * dotProduct;

        mReal.Set(-mReal.GetX() * realLength, -mReal.GetY() * realLength, -mReal.GetZ() * realLength, mReal.GetW() * realLength);
        mDual.Set(-mDual.GetX() * dualFactor, -mDual.GetY() * dualFactor, -mDual.GetZ() * dualFactor, mDual.GetW() * dualFactor);

        return *this;
    }


    // calculate the inversed version
    DualQuaternion DualQuaternion::Inversed() const
    {
        const float realLength = mReal.GetLength();
        const float dotProduct = mReal.Dot(mDual);
        const float dualFactor = realLength - 2.0f * dotProduct;
        return DualQuaternion(AZ::Quaternion(-mReal.GetX() * realLength, -mReal.GetY() * realLength, -mReal.GetZ() * realLength, mReal.GetW() * realLength),
            AZ::Quaternion(-mDual.GetX() * dualFactor, -mDual.GetY() * dualFactor, -mDual.GetZ() * dualFactor, mDual.GetW() * dualFactor));
    }


    // convert the dual quaternion to a matrix
    AZ::Transform DualQuaternion::ToTransform() const
    {
        const float sqLen   = mReal.Dot(mReal);
        const float x       = mReal.GetX();
        const float y       = mReal.GetY();
        const float z       = mReal.GetZ();
        const float w       = mReal.GetW();
        const float t0      = mDual.GetW();
        const float t1      = mDual.GetX();
        const float t2      = mDual.GetY();
        const float t3      = mDual.GetZ();

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
        const float length = mReal.GetLength();
        const float invLength = 1.0f / length;

        mReal.Set(mReal.GetX() * invLength, mReal.GetY() * invLength, mReal.GetZ() * invLength, mReal.GetW() * invLength);
        mDual.Set(mDual.GetX() * invLength, mDual.GetY() * invLength, mDual.GetZ() * invLength, mDual.GetW() * invLength);
        mDual += mReal * (mReal.Dot(mDual) * -1.0f);
        return *this;
    }


    // convert back into rotation and translation
    void DualQuaternion::ToRotationTranslation(AZ::Quaternion* outRot, AZ::Vector3* outPos) const
    {
        const float invLength = 1.0f / mReal.GetLength();
        *outRot = mReal * invLength;
        outPos->Set(2.0f * (-mDual.GetW() * mReal.GetX() + mDual.GetX() * mReal.GetW() - mDual.GetY() * mReal.GetZ() + mDual.GetZ() * mReal.GetY()) * invLength,
            2.0f * (-mDual.GetW() * mReal.GetY() + mDual.GetX() * mReal.GetZ() + mDual.GetY() * mReal.GetW() - mDual.GetZ() * mReal.GetX()) * invLength,
            2.0f * (-mDual.GetW() * mReal.GetZ() - mDual.GetX() * mReal.GetY() + mDual.GetY() * mReal.GetX() + mDual.GetZ() * mReal.GetW()) * invLength);
    }


    // special case version for conversion into rotation and translation
    // only works with normalized dual quaternions
    void DualQuaternion::NormalizedToRotationTranslation(AZ::Quaternion* outRot, AZ::Vector3* outPos) const
    {
        *outRot = mReal;
        outPos->Set(2.0f * (-mDual.GetW() * mReal.GetX() + mDual.GetX() * mReal.GetW() - mDual.GetY() * mReal.GetZ() + mDual.GetZ() * mReal.GetY()),
            2.0f * (-mDual.GetW() * mReal.GetY() + mDual.GetX() * mReal.GetZ() + mDual.GetY() * mReal.GetW() - mDual.GetZ() * mReal.GetX()),
            2.0f * (-mDual.GetW() * mReal.GetZ() - mDual.GetX() * mReal.GetY() + mDual.GetY() * mReal.GetX() + mDual.GetZ() * mReal.GetW()));
    }


    // convert into a dual quaternion from a translation and rotation
    DualQuaternion DualQuaternion::ConvertFromRotationTranslation(const AZ::Quaternion& rotation, const AZ::Vector3& translation)
    {
        return DualQuaternion(rotation,  0.5f * (AZ::Quaternion(translation.GetX(), translation.GetY(), translation.GetZ(), 0.0f) * rotation));
    }
}   // namespace MCore
