/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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
            const Vector3 extents = 0.5f * aabb.GetExtents();

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
            const float maxDistSq = sphere.GetCenter().GetDistanceSq(aabb.GetMax());
            const float minDistSq = sphere.GetCenter().GetDistanceSq(aabb.GetMin());
            const float radiusSq = sphere.GetRadius() * sphere.GetRadius();
            return maxDistSq <= radiusSq && minDistSq <= radiusSq;
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
            return sphere1.GetCenter().GetDistanceSq(sphere2.GetCenter()) <= (radiusDiff * radiusDiff);
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
    }
}
