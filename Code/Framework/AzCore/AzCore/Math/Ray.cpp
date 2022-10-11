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
    Ray::Ray(const AZ::Vector3& origin, const AZ::Vector3& direction)
        : m_origin(origin)
        , m_direction(direction)
    {
        AZ_MATH_ASSERT(m_direction.IsNormalized(), "direction is not normalized");
    }

    Ray Ray::CreateFromLineSegment(const LineSegment& segment)
    {
        return Ray(segment.GetStart(), segment.GetDifference().GetNormalized());
    }

    const AZ::Vector3& Ray::GetOrigin() const
    {
        return m_origin;
    }

    const AZ::Vector3& Ray::GetDirection() const
    {
        return m_direction;
    }
} // namespace AZ
