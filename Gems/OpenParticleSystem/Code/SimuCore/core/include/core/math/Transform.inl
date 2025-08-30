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
    inline Transform::Transform(const Vector3& trans, const Quaternion& rot,  float s)
    {
        value = AZ::Transform::CreateFromQuaternionAndTranslation(rot.value, trans.value);
        value.SetUniformScale(s);
    }

    inline Transform Transform::operator*(const Transform& rhs) const
    {
        return value * rhs.value;
    }

    inline Transform& Transform::operator*=(const Transform& rhs)
    {
        *this = *this * rhs;
        return *this;
    }

    inline bool Transform::operator==(const Transform& rhs) const
    {
        return value == rhs.value;
    }

    inline bool Transform::operator!=(const Transform& rhs) const
    {
        return !operator==(rhs);
    }


    inline Matrix4 Transform::ToMatrix() const
    {
        return AZ::Matrix4x4::CreateFromTransform(value);
    }

    inline Transform Transform::Inverse() const
    {
        return value.GetInverse();
    }

    inline Vector3 Transform::TransformPoint(const Vector3& rhs) const
    {
        return value.TransformPoint(rhs.value);
    }

    inline Vector3 Transform::TransformVector(const Vector3& rhs) const
    {
        return value.TransformVector(rhs.value);
    }

    inline Vector3 Transform::GetTranslation() const
    {
        return value.GetTranslation();
    }

    inline Quaternion Transform::GetRotation() const
    {
        return value.GetRotation();
    }

    inline float Transform::GetUniformScale() const
    {
        return value.GetUniformScale();
    }

    inline void Transform::LookAt(const Vector3& start, const Vector3& target, const AZ::Transform::Axis axisUp)
    {
        value = AZ::Transform::CreateLookAt(start.value, target.value, axisUp);
    }
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
