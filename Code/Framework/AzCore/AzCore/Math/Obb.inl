/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AZ
{
    AZ_MATH_INLINE const Vector3& Obb::GetPosition() const
    {
        return m_position;
    }


    AZ_MATH_INLINE const Quaternion& Obb::GetRotation() const
    {
        return m_rotation;
    }


    AZ_MATH_INLINE void Obb::SetRotation(const Quaternion& rotation)
    {
        m_rotation = rotation;
    }


    AZ_MATH_INLINE const Vector3& Obb::GetHalfLengths() const
    {
        return m_halfLengths;
    }


    AZ_MATH_INLINE void Obb::SetHalfLengths(const Vector3& halfLengths)
    {
        m_halfLengths = halfLengths;
    }


    AZ_MATH_INLINE Vector3 Obb::GetAxisX() const
    {
        return m_rotation.TransformVector(Vector3::CreateAxisX());
    }


    AZ_MATH_INLINE Vector3 Obb::GetAxisY() const
    {
        return m_rotation.TransformVector(Vector3::CreateAxisY());
    }


    AZ_MATH_INLINE Vector3 Obb::GetAxisZ() const
    {
        return m_rotation.TransformVector(Vector3::CreateAxisZ());
    }


    AZ_MATH_INLINE Vector3 Obb::GetAxis(int32_t index) const
    {
        Vector3 axis = Vector3::CreateZero();
        axis.SetElement(index, 1.0f);
        return m_rotation.TransformVector(axis);
    }


    AZ_MATH_INLINE float Obb::GetHalfLengthX() const
    {
        return m_halfLengths.GetX();
    }


    AZ_MATH_INLINE float Obb::GetHalfLengthY() const
    {
        return m_halfLengths.GetY();
    }


    AZ_MATH_INLINE float Obb::GetHalfLengthZ() const
    {
        return m_halfLengths.GetZ();
    }


    AZ_MATH_INLINE float Obb::GetHalfLength(int32_t index) const
    {
        return m_halfLengths.GetElement(index);
    }


    AZ_MATH_INLINE void Obb::SetPosition(const Vector3& position)
    {
        m_position = position;
    }


    AZ_MATH_INLINE void Obb::SetHalfLengthX(float halfLength)
    {
        m_halfLengths.SetX(halfLength);
    }


    AZ_MATH_INLINE void Obb::SetHalfLengthY(float halfLength)
    {
        m_halfLengths.SetY(halfLength);
    }


    AZ_MATH_INLINE void Obb::SetHalfLengthZ(float halfLength)
    {
        m_halfLengths.SetZ(halfLength);
    }


    AZ_MATH_INLINE void Obb::SetHalfLength(int32_t index, float halfLength)
    {
        m_halfLengths.SetElement(index, halfLength);
    }


    AZ_MATH_INLINE bool Obb::Contains(const Vector3& point) const
    {
        const Vector3 local = m_rotation.GetInverseFast().TransformVector(point - m_position);
        return local.IsGreaterEqualThan(-m_halfLengths) && local.IsLessEqualThan(m_halfLengths);
    }


    AZ_MATH_INLINE float Obb::GetDistance(const Vector3& point) const
    {
        const Vector3 local = m_rotation.GetInverseFast().TransformVector(point - m_position);
        const Vector3 closest = local.GetClamp(-m_halfLengths, m_halfLengths);
        return local.GetDistance(closest);
    }


    AZ_MATH_INLINE float Obb::GetDistanceSq(const Vector3& point) const
    {
        const Vector3 local = m_rotation.GetInverseFast().TransformVector(point - m_position);
        const Vector3 closest = local.GetClamp(-m_halfLengths, m_halfLengths);
        return local.GetDistanceSq(closest);
    }
}
