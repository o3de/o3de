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
    namespace ShapeIntersection
    {
        AZ_MATH_INLINE bool IntersectThreePlanes(const Plane& p1, const Plane& p2, const Plane& p3, Vector3& outP)
        {
            // From Foundations of Game Development Vol 1 by Eric Lengyel (Ch. 3)
            Vector3 n1CrossN2 = p1.GetNormal().Cross(p2.GetNormal());
            float det = n1CrossN2.Dot(p3.GetNormal());
            if (GetAbs(det) > std::numeric_limits<float>::min())
            {
                Vector3 n3CrossN2 = p3.GetNormal().Cross(p2.GetNormal());
                Vector3 n1CrossN3 = p1.GetNormal().Cross(p3.GetNormal());

                outP = (n3CrossN2 * p1.GetDistance() + n1CrossN3 * p2.GetDistance() - n1CrossN2 * p3.GetDistance()) / det;
                return true;
            }

            return false;
        }

        AZ_MATH_INLINE IntersectResult Classify(const Plane& plane, const Sphere& sphere)
        {
            float distance = plane.GetPointDist(sphere.GetCenter());
            float radius = sphere.GetRadius();
            if (distance < -radius)
            {
                return IntersectResult::Exterior;
            }
            else if (distance > radius)
            {
                return IntersectResult::Interior;
            }
            else
            {
                return IntersectResult::Overlaps;
            }
        }

        AZ_MATH_INLINE IntersectResult Classify(const Plane& plane, const Obb& obb)
        {
            // Compute the projection interval radius onto the plane normal, then compute the distance to the plane and see if it's outside the interval.
            // d = distance of box center to the plane
            // r = projection interval radius of the obb (projected onto the plane normal)
            float d = plane.GetPointDist(obb.GetPosition());
            float r = obb.GetHalfLengthX() * GetAbs(plane.GetNormal().Dot(obb.GetAxisX()))
                    + obb.GetHalfLengthY() * GetAbs(plane.GetNormal().Dot(obb.GetAxisY()))
                    + obb.GetHalfLengthZ() * GetAbs(plane.GetNormal().Dot(obb.GetAxisZ()));
            if (d < -r)
            {
                return IntersectResult::Exterior;
            }
            else if (d > r)
            {
                return IntersectResult::Interior;
            }
            else
            {
                return IntersectResult::Overlaps;
            }
        }

        AZ_MATH_INLINE IntersectResult Classify(const Frustum& frustum, const Sphere& sphere)
        {
            return frustum.IntersectSphere(sphere);
        }

        AZ_MATH_INLINE bool Overlaps(const Aabb& aabb1, const Aabb& aabb2)
        {
            return aabb1.Overlaps(aabb2);
        }

        AZ_MATH_INLINE bool Overlaps(const Aabb& aabb, const Sphere& sphere)
        {
            return Overlaps(sphere, aabb);
        }

        AZ_MATH_INLINE bool Overlaps(const Sphere& sphere, const Aabb& aabb)
        {
            float distSq = aabb.GetDistanceSq(sphere.GetCenter());
            float radiusSq = sphere.GetRadius() * sphere.GetRadius();
            return distSq <= radiusSq;
        }

        AZ_MATH_INLINE bool Overlaps(const Sphere& sphere, const Frustum& frustum)
        {
            return Overlaps(frustum, sphere);
        }

        AZ_MATH_INLINE bool Overlaps(const Sphere& sphere, const Plane& plane)
        {
            const float dist = plane.GetPointDist(sphere.GetCenter());
            return dist * dist <= sphere.GetRadius() * sphere.GetRadius();
        }

        AZ_MATH_INLINE bool Overlaps(const Sphere& sphere1, const Sphere& sphere2)
        {
            const float radiusSum = sphere1.GetRadius() + sphere2.GetRadius();
            return sphere1.GetCenter().GetDistanceSq(sphere2.GetCenter()) <= (radiusSum * radiusSum);
        }

        AZ_MATH_INLINE bool Overlaps(const Sphere& sphere, const Obb& obb)
        {
            const float radius = sphere.GetRadius();
            return obb.GetDistanceSq(sphere.GetCenter()) < radius * radius;
        }

        AZ_MATH_INLINE bool Overlaps(const Sphere& sphere, const Capsule& capsule)
        {
            return Overlaps(capsule, sphere);
        }

        AZ_MATH_INLINE bool Overlaps(const Hemisphere& hemisphere, const Sphere& sphere)
        {
            float sphereDistanceToPlane = hemisphere.GetDirection().Dot(sphere.GetCenter() - hemisphere.GetCenter());

            if (sphereDistanceToPlane >= 0.0f)
            {
                // Sphere is in front of hemisphere, so treat the hemisphere as a sphere
                return Overlaps(Sphere(hemisphere.GetCenter(), hemisphere.GetRadius()), sphere);
            }
            else if (sphereDistanceToPlane > -sphere.GetRadius())
            {
                // Sphere is behind hemisphere, project the sphere onto the plane, then check radius of circle.
                Vector3 projectedSphereCenter = sphere.GetCenter() + hemisphere.GetDirection() * -sphereDistanceToPlane;
                float circleRadius = AZStd::sqrt(sphere.GetRadius() * sphere.GetRadius() - sphereDistanceToPlane * sphereDistanceToPlane);
                const float radiusSum = hemisphere.GetRadius() + circleRadius;
                return hemisphere.GetCenter().GetDistanceSq(projectedSphereCenter) < (radiusSum * radiusSum);
            }
            return false; // too far behind hemisphere to intersect
        }

        AZ_MATH_INLINE bool Overlaps(const Hemisphere& hemisphere, const Aabb& aabb)
        {
            float distSq = aabb.GetDistanceSq(hemisphere.GetCenter());
            float radiusSq = hemisphere.GetRadius() * hemisphere.GetRadius();
            if (distSq > radiusSq)
            {
                return false;
            }

            if (aabb.Contains(hemisphere.GetCenter()))
            {
                return true;
            }

            Vector3 nearestPointToPlane = aabb.GetSupport(-hemisphere.GetDirection());
            bool abovePlane = hemisphere.GetDirection().Dot(hemisphere.GetCenter() - nearestPointToPlane) > 0.0f;
            return !abovePlane; // This has false positives but is reasonably tight.
        }

        AZ_MATH_INLINE bool Overlaps(const Frustum& frustum, const Sphere& sphere)
        {
            for (Frustum::PlaneId planeId = Frustum::PlaneId::Near; planeId < Frustum::PlaneId::MAX; ++planeId)
            {
                if (frustum.GetPlane(planeId).GetPointDist(sphere.GetCenter()) + sphere.GetRadius() < 0.0f)
                {
                    return false;
                }
            }
            return true;
        }

        AZ_MATH_INLINE bool Overlaps(const Frustum& frustum, const Aabb& aabb)
        {
            //For an AABB, extents.Dot(planeAbs) computes the projection interval radius of the AABB onto the plane normal.
            //So for each plane, we can test compare the center-to-plane distance to this interval to see which side of the plane the AABB is on.
            //The AABB is not overlapping if it is fully behind any of the planes, otherwise it is overlapping.
            const Vector3 center = aabb.GetCenter();

            //If the AABB contains FLT_MAX at either (or both) extremes, it would be easy to overflow here by using "0.5f * GetExtents()"
            //or "0.5f * (GetMax() - GetMin())".  By separating into two separate multiplies before the subtraction, we can ensure
            //that we don't overflow.
            const Vector3 extents = (0.5f * aabb.GetMax()) - (0.5f * aabb.GetMin());

            for (Frustum::PlaneId planeId = Frustum::PlaneId::Near; planeId < Frustum::PlaneId::MAX; ++planeId)
            {
                const Plane plane = frustum.GetPlane(planeId);
                if (plane.GetPointDist(center) + extents.Dot(plane.GetNormal().GetAbs()) <= 0.0f)
                {
                    return false;
                }
            }
            return true;
        }

        AZ_MATH_INLINE bool Overlaps(const Frustum& frustum, const Obb& obb)
        {
            for (Frustum::PlaneId planeId = Frustum::PlaneId::Near; planeId < Frustum::PlaneId::MAX; ++planeId)
            {
                if (Classify(frustum.GetPlane(planeId), obb) == IntersectResult::Exterior)
                {
                    return false;
                }
            }

            return true;
        }

        AZ_MATH_INLINE bool Overlaps(const Capsule& capsule1, const Capsule& capsule2)
        {
            Vector3 closestPointSegment1;
            Vector3 closestPointSegment2;
            float segment1Proportion;
            float segment2Proportion;
            Intersect::ClosestSegmentSegment(
                capsule1.GetFirstHemisphereCenter(), capsule1.GetSecondHemisphereCenter(), capsule2.GetFirstHemisphereCenter(),
                capsule2.GetSecondHemisphereCenter(), segment1Proportion, segment2Proportion, closestPointSegment1, closestPointSegment2);
            const float radiusSum = capsule1.GetRadius() + capsule2.GetRadius();
            return closestPointSegment1.GetDistanceSq(closestPointSegment2) <= radiusSum * radiusSum;
        }

        AZ_MATH_INLINE bool Overlaps(const Capsule& capsule, const Sphere& sphere)
        {
            float proportion;
            Vector3 closestPointOnCapsuleAxis;
            Intersect::ClosestPointSegment(sphere.GetCenter(), capsule.GetFirstHemisphereCenter(), capsule.GetSecondHemisphereCenter(), proportion, closestPointOnCapsuleAxis);
            const float radiusSum = sphere.GetRadius() + capsule.GetRadius();
            return closestPointOnCapsuleAxis.GetDistanceSq(sphere.GetCenter()) <= radiusSum * radiusSum;
        }

        AZ_MATH_INLINE bool Overlaps(const Aabb& aabb, const Capsule& capsule)
        {
            return Overlaps(capsule, aabb);
        }

        AZ_MATH_INLINE bool Overlaps(const Capsule& capsule, const Aabb& aabb)
        {
            // First attempt a cheap rejection by comparing to the aabb's sphere.
            Vector3 aabbSphereCenter;
            float aabbSphereRadius;
            aabb.GetAsSphere(aabbSphereCenter, aabbSphereRadius);
            Sphere aabbSphere(aabbSphereCenter, aabbSphereRadius);
            if (!Overlaps(capsule, aabbSphere))
            {
                return false;
            }

            // Now do the more expensive test. The idea is to start with the end points of the capsule then
            // - Clamp the points with the aabb
            // - Find the closest points on the line segment to the clamped points.
            // - If the distance between the clamped point and the line segment point is less than the radius, then we know it intersects.
            // - Generate new clamped points from the points on the line segment for next iteration.
            // - If the two clamped points are equal to each other, or either of the new clamped points is equivalent to the previous clamped points,
            //   then we know we've already found the closest point possible on the aabb, so fail because previous distance check failed.
            // - Loop with new clamped points.

            float capsuleRadiusSq = capsule.GetRadius() * capsule.GetRadius();
            const Vector3& capsuleStart = capsule.GetFirstHemisphereCenter();
            const Vector3& capsuleEnd = capsule.GetSecondHemisphereCenter();
            const Vector3 capsuleSegment = capsuleEnd - capsuleStart;
            if (capsuleSegment.IsClose(AZ::Vector3::CreateZero()))
            {
                // capsule is nearly a sphere, and already failed sphere check above.
                return false;
            }
            const float segmentLengthSquared = capsuleSegment.Dot(capsuleSegment);
            const float rcpSegmentLengthSquared = 1.0f / segmentLengthSquared;

            Vector3 clampedPoint1 = capsuleStart.GetClamp(aabb.GetMin(), aabb.GetMax());
            Vector3 clampedPoint2 = capsuleEnd.GetClamp(aabb.GetMin(), aabb.GetMax());

            // Simplified from Intersect::ClosestPointSegment with certain parts pre-calculated, no need to return proportion.
            auto getClosestPointOnCapsule = [&](const Vector3& point) -> Vector3
            {
                float proportion = (point - capsuleStart).Dot(capsuleSegment);
                if (proportion <= 0.0f)
                {
                    return capsuleStart;
                }
                if (proportion >= segmentLengthSquared)
                {
                    return capsuleEnd;
                }
                return capsuleStart + (proportion * capsuleSegment * rcpSegmentLengthSquared);
            };

            constexpr uint32_t MaxIterations = 16;
            for (uint32_t i = 0; i < MaxIterations; ++i)
            {
                // Check point 1
                Vector3 closestPointOnCapsuleAxis1 = getClosestPointOnCapsule(clampedPoint1);
                if (clampedPoint1.GetDistanceSq(closestPointOnCapsuleAxis1) < capsuleRadiusSq)
                {
                    return true;
                }

                // Check point 2
                Vector3 closestPointOnCapsuleAxis2 = getClosestPointOnCapsule(clampedPoint2);
                if (clampedPoint2.GetDistanceSq(closestPointOnCapsuleAxis2) < capsuleRadiusSq)
                {
                    return true;
                }

                // If the points are the same, and previous tests failed, then this is the best point, but it's too far away.
                if (clampedPoint1.IsClose(clampedPoint2))
                {
                    return false;
                }

                // Choose better points.
                Vector3 newclampedPoint1 = closestPointOnCapsuleAxis1.GetClamp(aabb.GetMin(), aabb.GetMax());
                Vector3 newclampedPoint2 = closestPointOnCapsuleAxis2.GetClamp(aabb.GetMin(), aabb.GetMax());

                if (newclampedPoint1.IsClose(clampedPoint1) || newclampedPoint2.IsClose(clampedPoint2))
                {
                    // Capsule is parallel to AABB or beyond the end points and failing above tests, so it must be outside the capsule.
                    return false;
                }

                clampedPoint1 = newclampedPoint1;
                clampedPoint2 = newclampedPoint2;
            }

            return true; // prefer false positive
        }

        AZ_MATH_INLINE bool Contains(const Aabb& aabb1, const Aabb& aabb2)
        {
            return aabb1.Contains(aabb2);
        }

        AZ_MATH_INLINE bool Contains(const Aabb& aabb, const Sphere& sphere)
        {
            // Convert the sphere to an aabb
            return Contains(aabb, AZ::Aabb::CreateCenterRadius(sphere.GetCenter(), sphere.GetRadius()));
        }

        AZ_MATH_INLINE bool Contains(const Sphere& sphere, const Aabb& aabb)
        {
            const float radiusSq = sphere.GetRadius() * sphere.GetRadius();
            return aabb.GetMaxDistanceSq(sphere.GetCenter()) <= radiusSq;
        }

        AZ_MATH_INLINE bool Contains(const Sphere& sphere, const Vector3& point)
        {
            const float distSq = sphere.GetCenter().GetDistanceSq(point);
            const float radiusSq = sphere.GetRadius() * sphere.GetRadius();
            return distSq <= radiusSq;
        }

        AZ_MATH_INLINE bool Contains(const Sphere& sphere1, const Sphere& sphere2)
        {
            const float radiusDiff = sphere1.GetRadius() - sphere2.GetRadius();
            return sphere1.GetCenter().GetDistanceSq(sphere2.GetCenter()) <= (radiusDiff * radiusDiff) * AZ::GetSign(radiusDiff);
        }

        AZ_MATH_INLINE bool Contains(const Hemisphere& hemisphere, const Aabb& aabb)
        {
            const float radiusSq = hemisphere.GetRadius() * hemisphere.GetRadius();
            if (aabb.GetMaxDistanceSq(hemisphere.GetCenter()) <= radiusSq)
            {
                // points are inside sphere, check to make sure it's on the right side of the hemisphere plane
                Vector3 nearestPointToPlane = aabb.GetSupport(hemisphere.GetDirection());
                return hemisphere.GetDirection().Dot(nearestPointToPlane - hemisphere.GetCenter()) >= 0.0f;
            }
            return false;
        }

        AZ_MATH_INLINE bool Contains(const Frustum& frustum, const Aabb& aabb)
        {
            // For an AABB, extents.Dot(planeAbs) computes the projection interval radius of the AABB onto the plane normal.
            // So for each plane, we can test compare the center-to-plane distance to this interval to see which side of the plane the AABB is on.
            // The AABB is contained if it is fully in front of all of the planes.
            const Vector3 center = aabb.GetCenter();
            const Vector3 extents = 0.5f * aabb.GetExtents();

            for (Frustum::PlaneId planeId = Frustum::PlaneId::Near; planeId < Frustum::PlaneId::MAX; ++planeId)
            {
                const Plane plane = frustum.GetPlane(planeId);
                if (plane.GetPointDist(center) - extents.Dot(plane.GetNormal().GetAbs()) < 0.0f)
                {
                    return false;
                }
            }
            return true;
        }

        AZ_MATH_INLINE bool Contains(const Frustum& frustum, const Sphere& sphere)
        {
            for (Frustum::PlaneId planeId = Frustum::PlaneId::Near; planeId < Frustum::PlaneId::MAX; ++planeId)
            {
                if (frustum.GetPlane(planeId).GetPointDist(sphere.GetCenter()) - sphere.GetRadius() < 0.0f)
                {
                    return false;
                }
            }
            return true;
        }

        AZ_MATH_INLINE bool Contains(const Frustum& frustum, const Vector3& point)
        {
            for (Frustum::PlaneId planeId = Frustum::PlaneId::Near; planeId < Frustum::PlaneId::MAX; ++planeId)
            {
                if (frustum.GetPlane(planeId).GetPointDist(point) < 0.0f)
                {
                    return false;
                }
            }
            return true;
        }

        AZ_MATH_INLINE bool Contains(const Capsule& capsule, const Sphere& sphere)
        {
            float proportion;
            Vector3 closestPointOnCapsuleAxis;
            Intersect::ClosestPointSegment(sphere.GetCenter(), capsule.GetFirstHemisphereCenter(), capsule.GetSecondHemisphereCenter(), proportion, closestPointOnCapsuleAxis);
            return Contains(Sphere(closestPointOnCapsuleAxis, capsule.GetRadius()), sphere);
        }

        AZ_MATH_INLINE bool Contains(const Capsule& capsule, const Aabb& aabb)
        {
            AZ::Vector3 aabbSphereCenter;
            float aabbSphereRadius;
            aabb.GetAsSphere(aabbSphereCenter, aabbSphereRadius);
            AZ::Sphere aabbSphere(aabbSphereCenter, aabbSphereRadius);

            if (Contains(capsule, aabbSphere))
            {
                return true;
            }
            else if (!Overlaps(capsule, aabbSphere))
            {
                return false;
            }

            // Unable to determine with fast sphere based checks, so check each point in the aabb.
            for (const AZ::Vector3& aabbPoint :
                {
                    aabb.GetMin(),
                    aabb.GetMax(),
                    AZ::Vector3(aabb.GetMin().GetX(), aabb.GetMin().GetY(), aabb.GetMax().GetZ()),
                    AZ::Vector3(aabb.GetMin().GetX(), aabb.GetMax().GetY(), aabb.GetMin().GetZ()),
                    AZ::Vector3(aabb.GetMin().GetX(), aabb.GetMax().GetY(), aabb.GetMax().GetZ()),
                    AZ::Vector3(aabb.GetMax().GetX(), aabb.GetMin().GetY(), aabb.GetMin().GetZ()),
                    AZ::Vector3(aabb.GetMax().GetX(), aabb.GetMin().GetY(), aabb.GetMax().GetZ()),
                    AZ::Vector3(aabb.GetMax().GetX(), aabb.GetMax().GetY(), aabb.GetMin().GetZ()),
                })
            {
                if (!capsule.Contains(aabbPoint))
                {
                    return false;
                }
            }
            return true;
        }
    }
}
