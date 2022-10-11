/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>

namespace AZ
{
    class LineSegment;

    //! Ray defined by two Vector3, an orign and a normalized direction.
    class Ray
    {
    public:
        AZ_TYPE_INFO(Ray, "{0301a872-5bea-4563-8070-85ed243cb57c}");

        //! Default constructor, Creates a ray at the origin with a length of 0.
        Ray() = default;
        //! Creates a Ray with a starting origin and direction. 
        //! direction has to be normalized to be valid.
        Ray(const AZ::Vector3& origin, const AZ::Vector3& direction);

        //! Create a ray from a LineSegment where the the start of the line segment is
        //! the orign and the direction points toward the ending point. 
        static Ray CreateFromLineSegment(const LineSegment& segment);

        //! returns the origin.
        const AZ::Vector3& GetOrigin() const;
        //! returns the normalized direction of the Ray.
        const AZ::Vector3& GetDirection() const;

    private:
        AZ::Vector3 m_origin;
        AZ::Vector3 m_direction;
    };
} // namespace AZ
