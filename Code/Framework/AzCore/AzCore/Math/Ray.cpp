/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Ray.h>
#include <AzCore/Math/

namespace O3DE
{
    Ray::Ray(const AZ::Vector3& origin, const AZ::Vector3& direction)
        : m_origin(origin)
        , m_direction(direction)
    {
        AZ_MATH_ASSERT(m_direction.IsNormalized(), "This normal is not normalized");
    }

    const AZ::Vector3& Ray::getOrigin() const
    {
        return m_origin;
    }
    
    const AZ::Vector3& Ray::getDirection() const
    {
        return m_direction;
    }

    bool Ray::operator==(const Ray& rhs) const
    {
        return m_start == rhs.m_start && m_direction == rhs.m_direction;
    }

    bool Ray::operator!=(const Ray& rhs) const
    {
        return m_start != rhs.m_start && m_direction != rhs.m_direction;
    }

    Ray& Ray::operator=(const Ray& rhs) const
    {
        m_origin = rhs.m_origin;
        m_direction = rhs.m_direction;
        return *this;
    }
} // namespace O3DE