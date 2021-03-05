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

// multiply a vector by a quaternion
MCORE_INLINE AZ::Vector3 Quaternion::operator * (const AZ::Vector3& p) const
{
    Quaternion v(p.GetX(), p.GetY(), p.GetZ(), 0.0f);
    v = *this* v* this->Conjugated();
    return AZ::Vector3(v.x, v.y, v.z);
}



// returns the ratio of two quaternions
MCORE_INLINE Quaternion Quaternion::operator / (const Quaternion& q) const
{
    Quaternion t((*this) * -q);
    Quaternion s((-q) * (-q));
    t *= (1.0f / s.w);
    return t;
}



// calculates the length of the quaternion
MCORE_INLINE float Quaternion::Length() const
{
    const float sqLen = SquareLength();
    return Math::SafeSqrt(sqLen);
}


// normalizes the quaternion using approximation
MCORE_INLINE Quaternion& Quaternion::Normalize()
{
    // calculate 1.0 / length
    //  const float ooLen = 1.0f / Math::FastSqrt(x*x + y*y + z*z + w*w);
    //  const float ooLen = Math::FastInvSqrt(x*x + y*y + z*z + w*w);
    const float squareValue = x * x + y * y + z * z + w * w;
    const float ooLen = Math::InvSqrt(squareValue);

    x *= ooLen;
    y *= ooLen;
    z *= ooLen;
    w *= ooLen;

    return *this;
}


// get the right axis
MCORE_INLINE AZ::Vector3 Quaternion::CalcRightAxis() const
{
    return AZ::Vector3(1.0f -  2.0f * y * y - 2.0f * z * z,
        2.0f * x * y + 2.0f * z * w,
        2.0f * x * z - 2.0f * y * w);
}


// get the forward axis
MCORE_INLINE AZ::Vector3 Quaternion::CalcForwardAxis() const
{
    return AZ::Vector3(2.0f * x * y - 2.0f * z * w,
        1.0f - 2.0f * x * x - 2.0f * z * z,
        2.0f * y * z + 2.0f * x * w);
}


// get the up axis
MCORE_INLINE AZ::Vector3 Quaternion::CalcUpAxis() const
{
    return AZ::Vector3(2.0f * x * z + 2.0f * y * w,
        2.0f * y * z - 2.0f * x * w,
        1.0f - 2.0f * x * x - 2.0f * y * y);
}


// multiply by a quaternion
MCORE_INLINE const Quaternion& Quaternion::operator*=(const Quaternion& q)
{
    const float vx = w * q.x + x * q.w + y * q.z - z * q.y;
    const float vy = w * q.y + y * q.w + z * q.x - x * q.z;
    const float vz = w * q.z + z * q.w + x * q.y - y * q.x;
    const float vw = w * q.w - x * q.x - y * q.y - z * q.z;
    x = vx;
    y = vy;
    z = vz;
    w = vw;
    return *this;
}

