/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/IntersectSegment.h>

namespace AZ
{
    AZ_MATH_INLINE Capsule::Capsule(const Vector3& firstHemisphereCenter, const Vector3& secondHemisphereCenter, float radius)
        : m_firstHemisphereCenter(firstHemisphereCenter)
        , m_secondHemisphereCenter(secondHemisphereCenter)
    {
        AZ_MATH_ASSERT(radius >= 0, "Capsule radius must be non-negative");
        m_radius = radius;
    }

    AZ_MATH_INLINE Capsule::Capsule(const LineSegment& lineSegment, float radius)
        : m_firstHemisphereCenter(lineSegment.GetStart())
        , m_secondHemisphereCenter(lineSegment.GetEnd())
    {
        AZ_MATH_ASSERT(radius >= 0, "Capsule radius must be non-negative");
        m_radius = radius;
    }

    AZ_MATH_INLINE const Vector3& Capsule::GetFirstHemisphereCenter() const
    {
        return m_firstHemisphereCenter;
    }

    AZ_MATH_INLINE const Vector3& Capsule::GetSecondHemisphereCenter() const
    {
        return m_secondHemisphereCenter;
    }

    AZ_MATH_INLINE Vector3 Capsule::GetCenter() const
    {
        return 0.5f * (m_firstHemisphereCenter + m_secondHemisphereCenter);
    }

    AZ_MATH_INLINE float Capsule::GetRadius() const
    {
        return m_radius;
    }

    AZ_MATH_INLINE float Capsule::GetCylinderHeight() const
    {
        return m_firstHemisphereCenter.GetDistance(m_secondHemisphereCenter);
    }

    AZ_MATH_INLINE float Capsule::GetTotalHeight() const
    {
        return GetCylinderHeight() + 2.0f * m_radius;
    }

    AZ_MATH_INLINE void Capsule::SetFirstHemisphereCenter(const Vector3& firstHemisphereCenter)
    {
        m_firstHemisphereCenter = firstHemisphereCenter;
    }

    AZ_MATH_INLINE void Capsule::SetSecondHemisphereCenter(const Vector3& secondHemisphereCenter)
    {
        m_secondHemisphereCenter = secondHemisphereCenter;
    }

    AZ_MATH_INLINE void Capsule::SetRadius(float radius)
    {
        AZ_MATH_ASSERT(radius >= 0, "Capsule radius must be non-negative");
        m_radius = radius;
    }

    AZ_MATH_INLINE bool Capsule::IsClose(const Capsule& rhs, float tolerance) const
    {
        return AZ::IsClose(m_radius, rhs.m_radius, tolerance) &&
            ((m_firstHemisphereCenter.IsClose(rhs.m_firstHemisphereCenter, tolerance) &&
              m_secondHemisphereCenter.IsClose(rhs.m_secondHemisphereCenter, tolerance)) ||
             (m_firstHemisphereCenter.IsClose(rhs.m_secondHemisphereCenter, tolerance) &&
              m_secondHemisphereCenter.IsClose(rhs.m_firstHemisphereCenter, tolerance)));
    }

    AZ_MATH_INLINE bool Capsule::Contains(const AZ::Vector3& point) const
    {
        return Intersect::PointSegmentDistanceSq(point, m_firstHemisphereCenter, m_secondHemisphereCenter) <= m_radius * m_radius;
    }
} // namespace AZ

