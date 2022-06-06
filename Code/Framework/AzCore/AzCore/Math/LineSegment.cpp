/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/LineSegment.h>
#include <AzCore/Math/Ray.h>

namespace AZ
{
    LineSegment::LineSegment(const AZ::Vector3& start, const AZ::Vector3& end)
        : m_start(start)
        , m_end(end)
    {
    }

    LineSegment LineSegment::CreateFromRayAndLength(const Ray& ray, float length)
    {
        return LineSegment(ray.GetOrigin(), ray.GetOrigin() + (ray.GetDirection() * length));
    }

    const AZ::Vector3& LineSegment::GetStart() const
    {
        return m_start;
    }

    const AZ::Vector3& LineSegment::GetEnd() const
    {
        return m_end;
    }

    AZ::Vector3 LineSegment::GetDifference() const
    {
        return m_end - m_start;
    }

    AZ::Vector3 LineSegment::GetPoint(float t) const
    {
        return m_start.Lerp(m_end, t);
    }

} // namespace AZ
