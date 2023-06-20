/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Aabb.h>

namespace AZ
{
    AZ_MATH_INLINE Hemisphere::Hemisphere(const Vector3& center, float radius, const Vector3& normalizedDirection)
        : m_centerRadius(AZ::Vector4(center, radius))
        , m_direction(normalizedDirection)
    {
        AZ_MATH_ASSERT(normalizedDirection.IsNormalized(), "The direction is not normalized");
    }

    AZ_MATH_INLINE Hemisphere Hemisphere::CreateFromSphereAndDirection(const Sphere& sphere, const Vector3& normalizedDirection)
    {
        return Hemisphere(sphere.GetCenter(), sphere.GetRadius(), normalizedDirection);
    }

    AZ_MATH_INLINE Vector3 Hemisphere::GetCenter() const
    {
        return Vector3(m_centerRadius);
    }

    AZ_MATH_INLINE float Hemisphere::GetRadius() const
    {
        return m_centerRadius.GetW();
    }

    AZ_MATH_INLINE const Vector3& Hemisphere::GetDirection() const
    {
        return m_direction;
    }

    AZ_MATH_INLINE void Hemisphere::SetCenter(const Vector3& center)
    {
        m_centerRadius.Set(center, GetRadius());
    }

    AZ_MATH_INLINE void Hemisphere::SetRadius(float radius)
    {
        m_centerRadius.SetW(radius);
    }

    AZ_MATH_INLINE void Hemisphere::SetDirection(const Vector3& direction)
    {
        m_direction = direction;
    }

    AZ_MATH_INLINE bool Hemisphere::operator==(const Hemisphere& rhs) const
    {
        return (m_centerRadius == rhs.m_centerRadius) && (m_direction == rhs.m_direction);
    }

    AZ_MATH_INLINE bool Hemisphere::operator!=(const Hemisphere& rhs) const
    {
        return !(*this == rhs);
    }
}
