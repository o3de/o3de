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
    class Ray;

    //! LineSegment defined by two Vector3, an start and a end.
    class LineSegment
    {
    public:
        AZ_TYPE_INFO(LineSegment, "{7557da1e-cc20-11ec-9d64-0242ac120002}");

        //! Default constructor, Creates an empty LineSegment at the origin with a length of 0.
        LineSegment() = default;
        //! Create a LineSegment with a start and an end. 
        LineSegment(const AZ::Vector3& start, const AZ::Vector3& end);

        //! Create a LineSegment from a ray where origin is the start and 
        //! the length defines the end of the LineSegment given the direction of the Ray.
        static LineSegment CreateFromRayAndLength(const Ray& segment, float length);

        const AZ::Vector3& GetStart() const;
        const AZ::Vector3& GetEnd() const;

        //! The difference between the end the the start.
         //! @return Direction and mangitude of the LineSegment.
        AZ::Vector3 GetDifference() const;

        //! Returns the point along the segment from start to end -range [0, 1].
        //! @param t fraction/proportion/percentage along the LineSegment.
        //! @return The Position give the fraction t.
        AZ::Vector3 GetPoint(float t) const;

    private:
        AZ::Vector3 m_start;
        AZ::Vector3 m_end;
    };

} // namespace AZ
