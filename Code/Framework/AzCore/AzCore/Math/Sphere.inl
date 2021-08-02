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
    AZ_MATH_INLINE Sphere::Sphere(const Vector3& center, float radius)
    {
        m_center = center;
        m_radius = radius;
    }


    AZ_MATH_INLINE Sphere Sphere::CreateUnitSphere()
    {
        return Sphere(AZ::Vector3::CreateZero(), 1.0f);
    }


    AZ_MATH_INLINE Sphere Sphere::CreateFromAabb(const Aabb& aabb)
    {
        const AZ::Vector3 halfExtent = 0.5f * (aabb.GetMax() - aabb.GetMin());
        const AZ::Vector3 center = aabb.GetMin() + halfExtent;
        const float radius = halfExtent.GetMaxElement();
        return Sphere(center, radius);
    }


    AZ_MATH_INLINE const Vector3& Sphere::GetCenter() const
    {
        return m_center;
    }


    AZ_MATH_INLINE float Sphere::GetRadius() const
    {
        return m_radius;
    }


    AZ_MATH_INLINE void Sphere::SetCenter(const Vector3& center)
    {
        m_center = center;
    }


    AZ_MATH_INLINE void Sphere::SetRadius(float radius)
    {
        m_radius = radius;
    }


    AZ_MATH_INLINE void Sphere::Set(const Sphere& sphere)
    {
        m_center = sphere.m_center;
        m_radius = sphere.m_radius;
    }


    AZ_MATH_INLINE Sphere& Sphere::operator=(const Sphere& rhs)
    {
        Set(rhs);
        return *this;
    }


    AZ_MATH_INLINE bool Sphere::operator==(const Sphere& rhs) const
    {
        return (m_center == rhs.m_center) && (m_radius == rhs.m_radius);
    }


    AZ_MATH_INLINE bool Sphere::operator!=(const Sphere& rhs) const
    {
        return !(*this == rhs);
    }
}
