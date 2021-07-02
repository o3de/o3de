/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// float
template <>
MCORE_INLINE bool Compare<float>::CheckIfIsClose(const float& a, const float& b, float threshold)
{
    return (Math::Abs(a - b) <= threshold);
}


// Vector2
template <>
MCORE_INLINE bool Compare<AZ::Vector2>::CheckIfIsClose(const AZ::Vector2& a, const AZ::Vector2& b, float threshold)
{
    return ((a - b).GetLength() <= threshold);
}


// Vector3
template <>
MCORE_INLINE bool Compare<AZ::Vector3>::CheckIfIsClose(const AZ::Vector3& a, const AZ::Vector3& b, float threshold)
{
    return ((a - b).GetLength() <= threshold);
}


// Vector4
template <>
MCORE_INLINE bool Compare<AZ::Vector4>::CheckIfIsClose(const AZ::Vector4& a, const AZ::Vector4& b, float threshold)
{
    return ((a - b).GetLength() <= threshold);
}


// Quaternion
template <>
MCORE_INLINE bool Compare<AZ::Quaternion>::CheckIfIsClose(const AZ::Quaternion& a, const AZ::Quaternion& b, float threshold)
{
    /*
    if (a.Dot(b) < 0.0f)
        return Compare<Vector4>::CheckIfIsClose( Vector4(a.x, a.y, a.z, a.w), Vector4(-b.x, -b.y, -b.z, -b.w), threshold);
    else
        return Compare<Vector4>::CheckIfIsClose( Vector4(a.x, a.y, a.z, a.w), Vector4(b.x, b.y, b.z, b.w), threshold);
        */

    AZ::Vector3 axisA, axisB;
    float   angleA, angleB;

    // convert to an axis and angle representation
    MCore::ToAxisAngle(a, axisA, angleA);
    MCore::ToAxisAngle(b, axisB, angleB);

    // compare the axis and angles
    if (Math::Abs(angleA  - angleB) > threshold)
    {
        return false;
    }
    if (Math::Abs(axisA.GetX() - axisB.GetX()) > threshold)
    {
        return false;
    }
    if (Math::Abs(axisA.GetY() - axisB.GetY()) > threshold)
    {
        return false;
    }
    if (Math::Abs(axisA.GetZ() - axisB.GetZ()) > threshold)
    {
        return false;
    }

    // they are the same!
    return true;
}
