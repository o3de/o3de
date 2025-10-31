/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/Math/IntersectionTestHelpers.h>
#include <AzCore/Math/MathUtils.h>

namespace UnitTest::IntersectTest
{
    template<bool oneSided>
    bool MollerTrumboreIntersectSegmentTriangleGeneric(
        const AZ::Vector3& p, const AZ::Vector3& rayEnd,
        const AZ::Vector3& a, const AZ::Vector3& b, const AZ::Vector3& c,
        AZ::Vector3& normal, float& t)
    {
        AZ::Vector3 ab = b - a;
        AZ::Vector3 ac = c - a;
        AZ::Vector3 ap = p - a;
        AZ::Vector3 qp = rayEnd - p;

        AZ::Vector3 h = qp.Cross(ac);
        float d = ab.Dot(h);

        if (d < AZ::Constants::FloatEpsilon)
        {
            if constexpr(oneSided)
            {
                return false;
            }
            else if (d > -AZ::Constants::FloatEpsilon)
            {
                return false;
            }
        }

        float f = 1.0f / d;
        float u = f * ap.Dot(h);
        if (u < 0.0f || u > 1.0f)
        {
            return false;
        }
        AZ::Vector3 q = ap.Cross(ab);
        float v = f * qp.Dot(q);
        if (v < 0.0f || u + v > 1.0f)
        {
            return false;
        }

        t = f * ac.Dot(q);
        if ((t > AZ::Constants::FloatEpsilon) && (t <= 1.0f))
        {
            normal = ab.Cross(ac).GetNormalized();
            if constexpr(!oneSided)
            {
                if (d < 0.0f)
                {
                    normal = -normal;
                }
            }
            return true;
        }

        return false;
    }

    bool MollerTrumboreIntersectSegmentTriangleCCW(
        const AZ::Vector3& p, const AZ::Vector3& rayEnd,
        const AZ::Vector3& a, const AZ::Vector3& b, const AZ::Vector3& c,
        AZ::Vector3& triNormal, float& t)
    {
        return MollerTrumboreIntersectSegmentTriangleGeneric<true>(p, rayEnd, a, b, c, triNormal, t);
    }
            
    bool MollerTrumboreIntersectSegmentTriangle(
        const AZ::Vector3& p, const AZ::Vector3& rayEnd,
        const AZ::Vector3& a, const AZ::Vector3& b, const AZ::Vector3& c,
        AZ::Vector3& triNormal, float& t)
    {
        return MollerTrumboreIntersectSegmentTriangleGeneric<false>(p, rayEnd, a, b, c, triNormal, t);
    }

    bool ArenbergIntersectSegmentTriangleCCW(
        const AZ::Vector3& p, const AZ::Vector3& q,
        const AZ::Vector3& a, const AZ::Vector3& b, const AZ::Vector3& c,
        AZ::Vector3& normal, float& t)
    {
        AZ::Vector3 ab = b - a;
        AZ::Vector3 ac = c - a;
        AZ::Vector3 qp = p - q;

        // Compute triangle normal. Can be pre-calculated/cached if
        // intersecting multiple segments against the same triangle.
        // Note that the normal isn't normalized yet, most of the calculations below just need the direction of the normal.
        // It will only get normalized after we've detected a successful hit.
        normal = ab.Cross(ac); // Right hand CCW

        // Compute denominator d.
        // This is a projection of our ray (pq) onto the Normal.
        // If d == 0, the ray is perpendicular to the Normal, or parallel to the triangle, so it doesn't intersect.
        // We're actually projecting qp, not pq, so if our ray is going the same direction as the Normal (positive),
        // then we're really pointing negative which means pointing into the side of the triangle with the surface.
        // If we're negative, then we're really pointing positive, which means pointing into the side of the triangle without the
        // surface, so exit early.
        // The value of d contains the length of our ray segment in triangle space, since it's the length along the normal.
        float d = qp.Dot(normal);
        if (d <= 0.0f)
        {
            return false;
        }

        // Compute t, which is the length of ray segment that's above the triangle in triangle space.
        // If t < 0, then the entire ray segment appears below the triangle, so return early.
        // If t > d, then the entire ray segment appears above the triangle, so return early.
        // If 0 <= t <= d, then some of pq appears above the triangle, and some appears below it.
        // Compute intersection t value of pq with plane of triangle.
        // If this were a ray instead of a segment, 0 <= t would be a sufficient check.
        AZ::Vector3 ap = p - a;
        t = ap.Dot(normal);
        if (t < 0.0f || t > d)
        {
            return false;
        }

        // Compute barycentric coordinate components and test if within bounds
        AZ::Vector3 e = qp.Cross(ap);
        float v = ac.Dot(e);
        if (v < 0.0f || v > d)
        {
            return false;
        }
        float w = -ab.Dot(e);
        if (w < 0.0f || ((v + w) > d))
        {
            return false;
        }

        // t/d is the fractional percentage of our ray segment that appears above the triangle in triangle space.
        // However, because it's a percentage, it can also be used back in world space as well to determine how much of our ray
        // segment appears above the triangle, which consequently gives us the collision point.
        float ood = 1.0f / d;
        t *= ood;

        normal.Normalize();

        return true;
    }

    bool ArenbergIntersectSegmentTriangle(
        const AZ::Vector3& p, const AZ::Vector3& q,
        const AZ::Vector3& a, const AZ::Vector3& b, const AZ::Vector3& c,
        AZ::Vector3& normal, float& t)
    {
        AZ::Vector3 ab = b - a;
        AZ::Vector3 ac = c - a;
        AZ::Vector3 qp = p - q;
        AZ::Vector3 ap = p - a;

        // Compute triangle normal. Can be pre-calculated or cached if
        // intersecting multiple segments against the same triangle
        normal = ab.Cross(ac); // Right hand CCW

        // Compute denominator d. If d <= 0, segment is parallel to or points
        // away from triangle, so exit early
        float d = qp.Dot(normal);
        AZ::Vector3 e;
        if (d > AZ::Constants::FloatEpsilon)
        {
            // the normal is on the right side
            e = qp.Cross(ap);
        }
        else
        {
            normal = -normal;

            // so either have a parallel ray or our normal is flipped
            if (d >= -AZ::Constants::FloatEpsilon)
            {
                return false; // parallel
            }
            d = -d;
            e = ap.Cross(qp);
        }

        // Compute intersection t value of pq with plane of triangle. A ray
        // intersects iff 0 <= t. Segment intersects iff 0 <= t <= 1. Delay
        // dividing by d until intersection has been found to pierce triangle
        t = ap.Dot(normal);

        // range segment check t[0,1] (it this case [0,d])
        if (t < 0.0f || t > d)
        {
            return false;
        }

        // Compute barycentric coordinate components and test if within bounds
        float v = ac.Dot(e);
        if (v < 0.0f || v > d)
        {
            return false;
        }
        float w = -ab.Dot(e);
        if (w < 0.0f || v + w > d)
        {
            return false;
        }

        // Segment/ray intersects the triangle. Perform delayed division and
        // compute the last barycentric coordinate component
        float ood = 1.0f / d;
        t *= ood;

        normal.Normalize();

        return true;
    }

}
