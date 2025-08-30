/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifdef _MSC_VER
#pragma warning(push)
#endif

namespace SimuCore {
    inline Quaternion::Quaternion() 
    : value(0,0,0,1)
    {
        
    }

    inline Quaternion::Quaternion(float qx, float qy, float qz, float qw)
    : value(qx, qy, qz, qw)
    {
    }

    inline Quaternion::Quaternion(const Vector3& axis, float angle)
    {
        value = AZ::Quaternion::CreateFromAxisAngle(axis.value, angle);
    }

    inline Quaternion::Quaternion(const Quaternion& q)
    {
        value = q.value;
    }

    inline Quaternion& Quaternion::operator=(const Quaternion& q)
    {
        value = q.value;
        return *this;
    }

    inline Quaternion& Quaternion::operator*=(const Quaternion& q)
    {
        value *= q.value;
        return *this;
    }

    inline Quaternion& Quaternion::operator+=(const Quaternion& q)
    {
        value += q.value;
        return *this;
    }

    inline Quaternion Quaternion::operator*(const float scale) const
    {
        return value * scale;
    }

    inline Quaternion Quaternion::operator+(const Quaternion& q) const
    {
        return value + q.value;
    }

    inline Quaternion Quaternion::operator-(const Quaternion& q) const
    {
        return value - q.value;
    }

    inline Quaternion Quaternion::operator-() const
    {
        return value * -1.0f;
    }

    inline bool Quaternion::operator==(const Quaternion& q) const
    {
        return value == q.value;
    }

    inline bool Quaternion::operator!=(const Quaternion& q) const
    {
        return !(*this == q);
    }

    inline Vector3 Quaternion::operator*(const Vector3& v) const
    {
        Vector3 qv(value.GetX(), value.GetY(), value.GetZ());
        Vector3 v1(Cross(qv, v));
        Vector3 v2(Cross(qv, v1));
        return v + ((v1 * value.GetW()) + v2) * 2.f;
    }
    
    inline Quaternion& Quaternion::Normalize()
    {
        value = value.GetNormalized();
        return *this;
    }

    inline void Quaternion::FromMatrix3(const Matrix3& m)
    {
        value = AZ::Quaternion::CreateFromMatrix3x3(m.value);
        (void)Normalize();
    }

    inline Matrix3 Quaternion::ToMatrix3() const
    {
        return AZ::Matrix3x3::CreateFromQuaternion(value);
    }

    inline Matrix4 Quaternion::ToMatrix() const
    {
        return AZ::Matrix4x4::CreateFromQuaternion(value);
    }

    inline Quaternion Quaternion::GetConjugate() const
    {
        return value.GetConjugate();
    }

    inline Quaternion Quaternion::Inverse() const
    {
        return value.GetInverseFull();
    }

    inline void Quaternion::Slerp(const Quaternion& right, float factor)
    {
        value = value.Slerp(right.value, factor);
        (void)Normalize();
    }

    inline Vector3 Quaternion::RotateVector3(const Vector3& v) const
    {
        Quaternion p(v.value.GetX(), v.value.GetY(), v.value.GetZ(), 0.0f);
        p *= this->Inverse();
        Quaternion pp = Quaternion(*this);
        pp *= p;
        return Vector3(pp.value.GetX(), pp.value.GetY(), pp.value.GetZ());
    }

    inline Vector3 Quaternion::RotateVector3(const Quaternion& q, const Vector3& v)
    {
        Quaternion p(v.value.GetX(), v.value.GetY(), v.value.GetZ(), 0.0f);
        p *= q.Inverse();
        Quaternion pp = Quaternion(q);
        pp *= p;
        return Vector3(pp.value.GetX(), pp.value.GetY(), pp.value.GetZ());
    }
}


#ifdef _MSC_VER
#pragma warning(pop)
#endif
