/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_MATH_SEGMENT_INTERSECTION_H
#define AZCORE_MATH_SEGMENT_INTERSECTION_H

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Obb.h>
#include <AzCore/Math/Plane.h>

/// \file isect_segment.h

namespace AZ
{
    namespace Intersect
    {
        //! LineToPointDistanceTime computes the time of the shortest distance from point 'p' to segment (s1,s2).
        //! To calculate the point of intersection:
        //! P = s1 + u (s2 - s1)
        //! @param s1 segment start point
        //! @param s2 segment end point
        //! @param p point to find the closest time to.
        //! @return time (on the segment) for the shortest distance from 'p' to (s1,s2)  [0.0f (s1),1.0f (s2)]
        inline float LineToPointDistanceTime(const Vector3& s1, const Vector3& s21, const Vector3& p)
        {
            // so u = (p.x - s1.x)*(s2.x - s1.x) + (p.y - s1.y)*(s2.y - s1.y) + (p.z-s1.z)*(s2.z-s1.z) /  |s2-s1|^2
            return s21.Dot(p - s1) / s21.Dot(s21);
        }

        //! LineToPointDistance computes the closest point to 'p' from a segment (s1,s2).
        //! @param s1 segment start point
        //! @param s2 segment end point
        //! @param p point to find the closest time to.
        //! @param u time (on the segment) for the shortest distance from 'p' to (s1,s2)  [0.0f (s1),1.0f (s2)]
        //! @return the closest point
        inline Vector3 LineToPointDistance(const Vector3& s1, const Vector3& s2, const Vector3& p, float& u)
        {
            const Vector3 s21 = s2 - s1;
            // we assume seg1 and seg2 are NOT coincident
            AZ_MATH_ASSERT(!s21.IsClose(Vector3(0.0f), 1e-4f), "OK we agreed that we will pass valid segments! (s1 != s2)");

            u = LineToPointDistanceTime(s1, s21, p);

            return s1 + u * s21;
        }

        //! Given segment pq and triangle abc (CCW), returns whether segment intersects
        //! triangle and if so, also returns the barycentric coordinates (u,v,w)
        //! of the intersection point.
        //! @param p segment start point
        //! @param q segment end point
        //! @param a triangle point 1
        //! @param b triangle point 2
        //! @param c triangle point 3
        //! @param normal at the intersection point.
        //! @param t time of intersection along the segment [0.0 (p), 1.0 (q)]
        //! @return 1 if the segment intersects the triangle otherwise 0
        int IntersectSegmentTriangleCCW(
            const Vector3& p, const Vector3& q, const Vector3& a, const Vector3& b, const Vector3& c,
            /*float &u, float &v, float &w,*/ Vector3& normal, float& t);

        //! Same as \ref IntersectSegmentTriangleCCW without respecting the triangle (a,b,c) vertex order (double sided).
        int IntersectSegmentTriangle(
            const Vector3& p, const Vector3& q, const Vector3& a, const Vector3& b, const Vector3& c,
            /*float &u, float &v, float &w,*/ Vector3& normal, float& t);

        //! Ray aabb intersection result types.
        enum RayAABBIsectTypes
        {
            ISECT_RAY_AABB_NONE = 0,    ///< no intersection
            ISECT_RAY_AABB_SA_INSIDE,   ///< the ray starts inside the aabb
            ISECT_RAY_AABB_ISECT,       ///< intersects along the PQ segment
        };

        //! Intersect ray R(t) = rayStart + t*d against AABB a. When intersecting,
        //! return intersection distance tmin and point q of intersection.
        //! @param rayStart ray starting point
        //! @param dir ray direction and length (dir = rayEnd - rayStart)
        //! @param dirRCP 1/dir (reciprocal direction - we cache this result very often so we don't need to compute it multiple times, otherwise just use dir.GetReciprocal())
        //! @param aabb Axis aligned bounding box to intersect against
        //! @param tStart time on ray of the first intersection [0,1] or 0 if the ray starts inside the aabb - check the return value
        //! @param tEnd time of the of the second intersection [0,1] (it can be > 1 if intersects after the rayEnd)
        //! @param startNormal normal at the start point.
        //! @return \ref RayAABBIsectTypes
        int IntersectRayAABB(
            const Vector3& rayStart, const Vector3& dir, const Vector3& dirRCP, const Aabb& aabb,
            float& tStart, float& tEnd, Vector3& startNormal /*, Vector3& inter*/);

        //! Intersect ray against AABB.
        //! @param rayStart ray starting point.
        //! @param dir ray reciprocal direction.
        //! @param aabb Axis aligned bounding box to intersect against.
        //! @param start length on ray of the first intersection.
        //! @param end length of the of the second intersection.
        //! @return \ref RayAABBIsectTypes In this faster version than IntersectRayAABB we return only ISECT_RAY_AABB_NONE and ISECT_RAY_AABB_ISECT.
        //! You can check yourself for that case.
        int IntersectRayAABB2(
            const Vector3& rayStart, const Vector3& dirRCP, const Aabb& aabb,
            float& start, float& end);

        //! Clip a ray to an aabb. return true if ray was clipped. The ray
        //! can be inside so don't use the result if the ray intersect the box.
        inline int ClipRayWithAabb(
            const Aabb& aabb, Vector3& rayStart, Vector3& rayEnd, float& tClipStart, float& tClipEnd)
        {
            Vector3 startNormal;
            float tStart, tEnd;
            Vector3 dirLen = rayEnd - rayStart;
            if (IntersectRayAABB(rayStart, dirLen, dirLen.GetReciprocal(), aabb, tStart, tEnd, startNormal) != ISECT_RAY_AABB_NONE)
            {
                // clip the ray with the box
                if (tStart > 0.0f)
                {
                    rayStart = rayStart + tStart * dirLen;
                    tClipStart = tStart;
                }
                if (tEnd < 1.0f)
                {
                    rayEnd = rayStart + tEnd * dirLen;
                    tClipEnd = tEnd;
                }

                return 1;
            }

            return 0;
        }

        //! Test segment and aabb where the segment is defined by midpoint
        //! midPoint = (p1-p0) * 0.5f and half vector halfVector = p1 - midPoint.
        //! the aabb is at the origin and defined by half extents only.
        //! @return 1 if the intersect, otherwise 0.
        int TestSegmentAABBOrigin(const Vector3& midPoint, const Vector3& halfVector, const Vector3& aabbExtends);

        //! Test if segment specified by points p0 and p1 intersects AABB. \ref TestSegmentAABBOrigin
        //! @return 1 if the segment and AABB intersect, otherwise 0.
        inline int TestSegmentAABB(const Vector3& p0, const Vector3& p1, const Aabb& aabb)
        {
            Vector3 e = aabb.GetExtents();
            Vector3 d = p1 - p0;
            Vector3 m = p0 + p1 - aabb.GetMin() - aabb.GetMax();

            return TestSegmentAABBOrigin(m, d, e);
        }

        //! Ray sphere intersection result types.
        enum SphereIsectTypes
        {
            ISECT_RAY_SPHERE_SA_INSIDE = -1, // the ray starts inside the cylinder
            ISECT_RAY_SPHERE_NONE,      // no intersection
            ISECT_RAY_SPHERE_ISECT,     // along the PQ segment
        };

        //! IntersectRaySphereOrigin
        //! return time t>=0  but not limited, so if you check a segment make sure
        //! t <= segmentLen
        //! @param rayStart ray start point
        //! @param rayDirNormalized ray direction normalized.
        //! @param shereRadius sphere radius
        //! @param time of closest intersection [0,+INF] in relation to the normalized direction.
        //! @return \ref SphereIsectTypes
        AZ_INLINE int IntersectRaySphereOrigin(
            const Vector3& rayStart, const Vector3& rayDirNormalized,
            const float sphereRadius, float& t)
        {
            Vector3 m = rayStart;
            float b = m.Dot(rayDirNormalized);
            float c = m.Dot(m) - sphereRadius * sphereRadius;

            // Exit if r's origin outside s (c > 0)and r pointing away from s (b > 0)
            if (c > 0.0f && b > 0.0f)
            {
                return ISECT_RAY_SPHERE_NONE;
            }
            float discr = b * b - c;
            // A negative discriminant corresponds to ray missing sphere
            if (discr < 0.0f)
            {
                return ISECT_RAY_SPHERE_NONE;
            }

            // Ray now found to intersect sphere, compute smallest t value of intersection
            t = -b - Sqrt(discr);

            // If t is negative, ray started inside sphere so clamp t to zero
            if (t < 0.0f)
            {
                //  t = 0.0f;
                return ISECT_RAY_SPHERE_SA_INSIDE; // no hit if inside
            }
            //q = p + t * d;
            return ISECT_RAY_SPHERE_ISECT;
        }

        //! Intersect ray (rayStart,rayDirNormalized) and sphere (sphereCenter,sphereRadius) \ref IntersectRaySphereOrigin
        inline int IntersectRaySphere(
            const Vector3& rayStart, const Vector3& rayDirNormalized, const Vector3& sphereCenter, const float sphereRadius, float& t)
        {
            return IntersectRaySphereOrigin(rayStart - sphereCenter, rayDirNormalized, sphereRadius, t);
        }

        //! @param      rayOrigin      The origin of the ray to test.
        //! @param      rayDir         The direction of the ray to test. It has to be unit length.
        //! @param      diskCenter     Center point of the disk
        //! @param      diskRadius     Radius of the disk
        //! @param      diskNormal     A normal perpendicular to the disk
        //! @param[out] t              If returning 1 (indicating a hit), this contains distance from rayOrigin along the normalized rayDir that the hit occured at.
        //! @return                    The number of intersecting points.
        int IntersectRayDisk(
            const Vector3& rayOrigin, const Vector3& rayDir, const Vector3& diskCenter, const float diskRadius, const AZ::Vector3& diskNormal, float& t);

        //! If there is only one intersecting point, the coefficient is stored in \ref t1.
        //! @param      rayOrigin      The origin of the ray to test.
        //! @param      rayDir         The direction of the ray to test. It has to be unit length.
        //! @param      cylinderEnd1   The center of the circle on one end of the cylinder.
        //! @param      cylinderDir    The direction pointing from \ref cylinderEnd1 to the other end of the cylinder. It has to be unit length.
        //! @param      cylinderHeight The distance between two centers of the circles on two ends of the cylinder respectively.
        //! @param[out] t1             A possible coefficient in the ray's explicit equation from which an intersecting point is calculated as "rayOrigin + t1 * rayDir".
        //! @param[out] t2             A possible coefficient in the ray's explicit equation from which an intersecting point is calculated as "rayOrigin + t2 * rayDir".
        //! @return                    The number of intersecting points.
        int IntersectRayCappedCylinder(
            const Vector3& rayOrigin, const Vector3& rayDir, 
            const Vector3& cylinderEnd1, const Vector3& cylinderDir, float cylinderHeight, float cylinderRadius,
            float& t1, float& t2);

        //! If there is only one intersecting point, the coefficient is stored in \ref t1.
        //! @param      rayOrigin      The origin of the ray to test.
        //! @param      rayDir         The direction of the ray to test. It has to be unit length.
        //! @param      coneApex       The apex of the cone.
        //! @param      coneDir        The unit-length direction from the apex to the base. 
        //! @param      coneHeight     The height of the cone, from the apex to the base.
        //! @param      coneBaseRadius The radius of the cone base circle.
        //! @param[out] t1             A possible coefficient in the ray's explicit equation from which an intersecting point is calculated as "rayOrigin + t1 * rayDir".
        //! @param[out] t2             A possible coefficient in the ray's explicit equation from which an intersecting point is calculated as "rayOrigin + t2 * rayDir".
        //! @return                    The number of intersecting points.
        int IntersectRayCone(
            const Vector3& rayOrigin, const Vector3& rayDir,
            const Vector3& coneApex, const Vector3& coneDir, float coneHeight, float coneBaseRaidus,
            float& t1, float& t2);

        //! Test intersection between a ray and a plane in 3D.
        //! @param rayOrigin    The origin of the ray to test intersection with.
        //! @param rayDir       The direction of the ray to test intersection with.
        //! @param planePos     A point on the plane to test intersection with.
        //! @param planeNormal  The normal of the plane to test intersection with.
        //! @param t[out]       The coefficient in the ray's explicit equation from which the intersecting point is calculated as "rayOrigin + t * rayDirection".
        //! @return             The number of intersection point.
        int IntersectRayPlane(
            const Vector3& rayOrigin, const Vector3& rayDir, const Vector3& planePos,
            const Vector3& planeNormal, float& t);

        //! Test intersection between a ray and a two-sided quadrilateral defined by four points in 3D.
        //! The four points that define the quadrilateral could be passed in with either counter clock-wise 
        //! winding or clock-wise winding.
        //! @param rayOrigin  The origin of the ray to test intersection with.
        //! @param rayDir     The direction of the ray to test intersection with.
        //! @param vertexA    One of the four points that define the quadrilateral.
        //! @param vertexB    One of the four points that define the quadrilateral.
        //! @param vertexC    One of the four points that define the quadrilateral.
        //! @param vertexD    One of the four points that define the quadrilateral.
        //! @param t[out]     The coefficient in the ray's explicit equation from which the intersecting point is calculated as "rayOrigin + t * rayDirection".
        //! @return           The number of intersection point.
        int IntersectRayQuad(
            const Vector3& rayOrigin, const Vector3& rayDir, const Vector3& vertexA,
            const Vector3& vertexB, const Vector3& vertexC, const Vector3& vertexD, float& t);

        //! Test intersection between a ray and an oriented box in 3D.
        //! @param rayOrigin       The origin of the ray to test intersection with.
        //! @param rayDir          The direction of the ray to test intersection with.
        //! @param boxCenter       The position of the center of the box.
        //! @param boxAxis1        An axis along one dimension of the oriented box.
        //! @param boxAxis2        An axis along one dimension of the oriented box.
        //! @param boxAxis3        An axis along one dimension of the oriented box.
        //! @param boxHalfExtent1  The half extent of the box on the dimension of \ref boxAxis1.
        //! @param boxHalfExtent2  The half extent of the box on the dimension of \ref boxAxis2.
        //! @param boxHalfExtent3  The half extent of the box on the dimension of \ref boxAxis3.
        //! @param t[out]     The coefficient in the ray's explicit equation from which the intersecting point is calculated as "rayOrigin + t * rayDirection".
        //! @return           1 if there is an intersection, 0 otherwise.
        int IntersectRayBox(
            const Vector3& rayOrigin, const Vector3& rayDir, const Vector3& boxCenter, const Vector3& boxAxis1,
            const Vector3& boxAxis2, const Vector3& boxAxis3, float boxHalfExtent1, float boxHalfExtent2, float boxHalfExtent3,
            float& t);

        //! Test intersection between a ray and an OBB.
        //! @param rayOrigin The origin of the ray to test intersection with.
        //! @param rayDir The direction of the ray to test intersection with.
        //! @param obb The OBB to test for intersection with the ray.
        //! @param t[out] The coefficient in the ray's explicit equation from which the intersecting point is calculated as "rayOrigin + t * rayDirection".
        //! @return 1 if there is an intersection, 0 otherwise.
        int IntersectRayObb(const Vector3& rayOrigin, const Vector3& rayDir, const Obb& obb, float& t);

        //! Ray cylinder intersection types.
        enum CylinderIsectTypes
        {
            RR_ISECT_RAY_CYL_SA_INSIDE = -1, // the ray starts inside the cylinder
            RR_ISECT_RAY_CYL_NONE,  // no intersection
            RR_ISECT_RAY_CYL_PQ,        // along the PQ segment
            RR_ISECT_RAY_CYL_P_SIDE,    // on the P side
            RR_ISECT_RAY_CYL_Q_SIDE,    // on the Q side
        };

        //! Intersect segment S(t)=sa+t(dir), 0<=t<=1 against cylinder specified by p, q and r.
        int IntersectSegmentCylinder(
            const Vector3& sa, const Vector3& dir, const Vector3& p, const Vector3& q,
            const float r, float& t);

        //! Capsule ray intersect types.
        enum CapsuleIsectTypes
        {
            ISECT_RAY_CAPSULE_SA_INSIDE = -1, // the ray starts inside the cylinder
            ISECT_RAY_CAPSULE_NONE, // no intersection
            ISECT_RAY_CAPSULE_PQ,       // along the PQ segment
            ISECT_RAY_CAPSULE_P_SIDE,   // on the P side
            ISECT_RAY_CAPSULE_Q_SIDE,   // on the Q side
        };

        //! This is a quick implementation of segment capsule based on segment cylinder \ref IntersectSegmentCylinder
        //! segment sphere intersection. We can optimize it a lot once we fix the ray
        //! cylinder intersection.
        int IntersectSegmentCapsule(
            const Vector3& sa, const Vector3& dir, const Vector3& p,
            const Vector3& q, const float r, float& t);

        //! Intersect segment S(t)=A+t(B-A), 0<=t<=1 against convex polyhedron specified
        //! by the n halfspaces defined by the planes p[]. On exit tfirst and tlast
        //! define the intersection, if any.
        int IntersectSegmentPolyhedron(
            const Vector3& sa, const Vector3& sBA, const Plane p[], int numPlanes,
            float& tfirst, float& tlast, int& iFirstPlane, int& iLastPlane);

        //! Calculate the line segment closestPointSegment1<->closestPointSegment2 that is the shortest route between
        //! two segments segment1Start<->segment1End and segment2Start<->segment2End. Also calculate the values of segment1Proportion and segment2Proportion where
        //! closestPointSegment1 = segment1Start + (segment1Proportion * (segment1End - segment1Start))
        //! closestPointSegment2 = segment2Start + (segment2Proportion * (segment2End - segment2Start))
        //! If segments are parallel returns a solution.
        void ClosestSegmentSegment(
            const Vector3& segment1Start, const Vector3& segment1End, 
            const Vector3& segment2Start, const Vector3& segment2End, 
            float& segment1Proportion, float& segment2Proportion, 
            Vector3& closestPointSegment1, Vector3& closestPointSegment2,
            float epsilon = 1e-4f);

        //! Calculate the line segment closestPointSegment1<->closestPointSegment2 that is the shortest route between
        //! two segments segment1Start<->segment1End and segment2Start<->segment2End.
        //! If segments are parallel returns a solution.
        void ClosestSegmentSegment(
            const Vector3& segment1Start, const Vector3& segment1End,
            const Vector3& segment2Start, const Vector3& segment2End,
            Vector3& closestPointSegment1, Vector3& closestPointSegment2,
            float epsilon = 1e-4f);

        //! Calculate the point (closestPointOnSegment) that is the closest point on
        //! segment segmentStart/segmentEnd to point. Also calculate the value of proportion where
        //! closestPointOnSegment = segmentStart + (proportion * (segmentEnd - segmentStart))
        void ClosestPointSegment(
            const Vector3& point, const Vector3& segmentStart, const Vector3& segmentEnd,
            float& proportion, Vector3& closestPointOnSegment);
    }
}

#endif // AZCORE_MATH_SEGMENT_INTERSECTION_H
#pragma once
