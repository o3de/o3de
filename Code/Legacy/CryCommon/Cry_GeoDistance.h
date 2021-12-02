/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Common distance-computations
#pragma once

#include <Cry_Geo.h>
#include <AzCore/Math/Vector3.h>

#ifdef max
#undef max
#endif

namespace Distance {
    //----------------------------------------------------------------------------------
    /// Returns squared distance from a point to a line segment and also the "t value" (from 0 to 1) of the
    /// closest point on the line segment
    //----------------------------------------------------------------------------------
    template<typename F>
    ILINE F Point_LinesegSq(const Vec3_tpl<F>& p, const Lineseg& lineseg, F& fT)
    {
        Vec3_tpl<F> diff = p - lineseg.start;
        Vec3_tpl<F> dir = lineseg.end - lineseg.start;
        fT = diff.Dot(dir);

        if (fT <= 0.0f)
        {
            fT = 0.0f;
        }
        else
        {
            F fSqrLen = dir.GetLengthSquared();
            if (fT >= fSqrLen)
            {
                fT = 1.0f;
                diff -= dir;
            }
            else
            {
                fT /= fSqrLen;
                diff -= fT * dir;
            }
        }

        return diff.GetLengthSquared();
    }

    /// Returns distance from a point to a line segment and also the "t value" (from 0 to 1) of the
    /// closest point on the line segment
    template<typename F>
    ILINE F Point_Lineseg(const Vec3_tpl<F>& p, const Lineseg& lineseg, F& fT)
    {
        return sqrt_tpl(Point_LinesegSq(p, lineseg, fT));
    }

    //! \brief Get the distance squared from a Point to a Cylinder.
    //! \param point AZ::Vector3 The point to test distance against the cylinder
    //! \param cylinderAxisEndA AZ::Vector3 One end of the cylinder axis (centered in the circle)
    //! \param cylinderAxisEndB AZ::Vector3 Other end of the cylinder axis (centered in the circle)
    //! \param radius float Radius of the cylinder
    //! \return float Closest distance squared from the point to the cylinder.
    AZ_INLINE float Point_CylinderSq(
        const AZ::Vector3& point,
        const AZ::Vector3& cylinderAxisEndA,
        const AZ::Vector3& cylinderAxisEndB,
        float radius
    )
    {
        // Use the cylinder axis' center point to determine distance by
        // splitting into Voronoi regions and using symmetry.
        // The regions are:
        // - Inside
        // - Beyond cylinder radius but between two disc ends.
        // - Within cylinder radius but beyond two disc ends.
        // - Beyond cylinder radius and beyond two disc ends.

        const AZ::Vector3 cylinderAxis = cylinderAxisEndB - cylinderAxisEndA;
        float halfLength = cylinderAxis.GetLength() * 0.5f;
        const AZ::Vector3 cylinderAxisUnit = cylinderAxis.GetNormalized();

        // get the center of the axis and the vector from center to the test point
        const AZ::Vector3 centerPoint = (cylinderAxis * 0.5) + cylinderAxisEndA;
        const AZ::Vector3 pointToCenter = point - centerPoint;

        // distance point is from center (projected onto axis)
        // the abs here takes advantage of symmetry.
        float x = fabsf(pointToCenter.Dot(cylinderAxisUnit));

        // squared distance from point to center (hypotenuse)
        float n2 = pointToCenter.GetLengthSq();

        // squared distance from point to center perpendicular to axis (pythagorean)
        float y2 = n2 - sqr(x);

        float distanceSquared = 0.f;

        if (x < halfLength) // point is between the two ends
        {
            if (y2 > sqr(radius))   // point is outside of radius
            {
                distanceSquared = sqr(sqrtf(y2) - radius);
            }
            // else point is inside cylinder, distance is zero.
        }
        else if (y2 < sqr(radius))
        {
            // point is within radius
            // point projects into a disc at either end, grab the "parallel" distance only
            distanceSquared = sqr(x - halfLength);
        }
        else
        {
            // point is outside of radius
            // point projects onto the edge of the disc, grab distance in two directions,
            // combine "parallel" and "perpendicular" distances.
            distanceSquared = sqr(sqrtf(y2) - radius) + sqr(x - halfLength);
        }

        return distanceSquared;
    }

} //namespace Distance
