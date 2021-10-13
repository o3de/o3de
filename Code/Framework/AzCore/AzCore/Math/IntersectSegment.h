/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Obb.h>
#include <AzCore/Math/Plane.h>
#include <AzCore/Math/Vector3.h>

namespace AZ
{
    namespace Intersect
    {
        //! LineToPointDistanceTime computes the time of the shortest distance from point 'p' to segment (s1,s2).
        //! To calculate the point of intersection: P = s1 + u (s2 - s1)
        //! @param s1 Segment start point.
        //! @param s2 Segment end point.
        //! @param p Point to find the closest time to.
        //! @return Time (on the segment) for the shortest distance from 'p' to (s1,s2)  [0.0f (s1),1.0f (s2)]
        float LineToPointDistanceTime(const Vector3& s1, const Vector3& s21, const Vector3& p);

        //! LineToPointDistance computes the closest point to 'p' from a segment (s1,s2).
        //! @param s1 Segment start point
        //! @param s2 Segment end point
        //! @param p Point to find the closest time to.
        //! @param u Time (on the segment) for the shortest distance from 'p' to (s1,s2)  [0.0f (s1),1.0f (s2)]
        //! @return The closest point
        Vector3 LineToPointDistance(const Vector3& s1, const Vector3& s2, const Vector3& p, float& u);

        //! Given segment pq and triangle abc (CCW), returns whether segment intersects
        //! triangle and if so, also returns the barycentric coordinates (u,v,w)
        //! of the intersection point.
        //! @param p Segment start point.
        //! @param q Segment end point.
        //! @param a Triangle point 1.
        //! @param b Triangle point 2.
        //! @param c Triangle point 3.
        //! @param normal At the intersection point.
        //! @param t Time of intersection along the segment [0.0 (p), 1.0 (q)].
        //! @return true if the segments intersects the triangle otherwise false.
        bool IntersectSegmentTriangleCCW(
            const Vector3& p, const Vector3& q, const Vector3& a, const Vector3& b, const Vector3& c, Vector3& normal, float& t);

        //! Same as \ref IntersectSegmentTriangleCCW without respecting the triangle (a,b,c) vertex order (double sided).
        //! @param p Segment start point.
        //! @param q Segment end point.
        //! @param a Triangle point 1.
        //! @param b Triangle point 2.
        //! @param c Triangle point 3.
        //! @param normal At the intersection point.
        //! @param t Time of intersection along the segment [0.0 (p), 1.0 (q)].
        //! @return True if the segments intersects the triangle otherwise false.
        bool IntersectSegmentTriangle(
            const Vector3& p, const Vector3& q, const Vector3& a, const Vector3& b, const Vector3& c, Vector3& normal, float& t);

        //! Ray aabb intersection result types.
        enum RayAABBIsectTypes : AZ::s32
        {
            ISECT_RAY_AABB_NONE = 0, ///< no intersection
            ISECT_RAY_AABB_SA_INSIDE, ///< the ray starts inside the aabb
            ISECT_RAY_AABB_ISECT, ///< intersects along the PQ segment
        };

        //! Intersect ray R(t) = rayStart + t*d against AABB a. When intersecting,
        //! return intersection distance tmin and point q of intersection.
        //! @param rayStart Ray starting point
        //! @param dir Ray direction and length (dir = rayEnd - rayStart)
        //! @param dirRCP 1/dir (reciprocal direction - we cache this result very often so we don't need to compute it multiple times,
        //! otherwise just use dir.GetReciprocal())
        //! @param aabb Axis aligned bounding box to intersect against
        //! @param tStart Time on ray of the first intersection [0,1] or 0 if the ray starts inside the aabb - check the return value
        //! @param tEnd Time of the of the second intersection [0,1] (it can be > 1 if intersects after the rayEnd)
        //! @param startNormal Normal at the start point.
        //! @return \ref RayAABBIsectTypes
        RayAABBIsectTypes IntersectRayAABB(
            const Vector3& rayStart,
            const Vector3& dir,
            const Vector3& dirRCP,
            const Aabb& aabb,
            float& tStart,
            float& tEnd,
            Vector3& startNormal);

        //! Intersect ray against AABB.
        //! @param rayStart Ray starting point.
        //! @param dir Ray reciprocal direction.
        //! @param aabb Axis aligned bounding box to intersect against.
        //! @param start Length on ray of the first intersection.
        //! @param end Length of the of the second intersection.
        //! @return \ref RayAABBIsectTypes In this faster version than IntersectRayAABB we return only ISECT_RAY_AABB_NONE and
        //! ISECT_RAY_AABB_ISECT. You can check yourself for that case.
        RayAABBIsectTypes IntersectRayAABB2(const Vector3& rayStart, const Vector3& dirRCP, const Aabb& aabb, float& start, float& end);

        //! Clip a ray to an aabb. return true if ray was clipped. The ray
        //! can be inside so don't use the result if the ray intersect the box.
        //! @param aabb Bounds to test against.
        //! @param rayStart The start of the ray.
        //! @param rayEnd The end of the ray.
        //! @param[out] tClipStart The proportion where the ray enters the \ref Aabb.
        //! @param[out] tClipEnd The proportion where the ray exits the \ref Aabb.
        //! @return True if the ray was clipped, otherwise false.
        bool ClipRayWithAabb(const Aabb& aabb, Vector3& rayStart, Vector3& rayEnd, float& tClipStart, float& tClipEnd);

        //! Test segment and aabb where the segment is defined by midpoint
        //! midPoint = (p1-p0) * 0.5f and half vector halfVector = p1 - midPoint.
        //! the aabb is at the origin and defined by half extents only.
        //! @param midPoint Midpoint of a line segment.
        //! @param halfVector Half vector of an aabb.
        //! @param aabbExtends The extends of a bounded box.
        //! @return True if the segment and AABB intersect, otherwise false
        bool TestSegmentAABBOrigin(const Vector3& midPoint, const Vector3& halfVector, const Vector3& aabbExtends);

        //! Test if segment specified by points p0 and p1 intersects AABB. \ref TestSegmentAABBOrigin.
        //! @param p0 Segment start point.
        //! @param p1 Segment end point.
        //! @param aabb Bounded box to test against.
        //! @return True if the segment and AABB intersect, otherwise false.
        bool TestSegmentAABB(const Vector3& p0, const Vector3& p1, const Aabb& aabb);

        //! Ray sphere intersection result types.
        enum SphereIsectTypes : AZ::s32
        {
            ISECT_RAY_SPHERE_SA_INSIDE = -1, //!< The ray starts inside the cylinder
            ISECT_RAY_SPHERE_NONE, //!< No intersection
            ISECT_RAY_SPHERE_ISECT, //!< Along the PQ segment
        };

        //! IntersectRaySphereOrigin
        //! return time t>=0  but not limited, so if you check a segment make sure
        //! t <= segmentLen.
        //! @param rayStart ray start point.
        //! @param rayDirNormalized ray direction normalized.
        //! @param shereRadius Radius of sphere at origin.
        //! @param time of closest intersection [0,+INF] in relation to the normalized direction.
        //! @return \ref SphereIsectTypes.
        SphereIsectTypes IntersectRaySphereOrigin(
            const Vector3& rayStart, const Vector3& rayDirNormalized, const float sphereRadius, float& t);

        //! Intersect ray (rayStart,rayDirNormalized) and sphere (sphereCenter,sphereRadius) \ref IntersectRaySphereOrigin
        //! @param rayStart The start of the ray.
        //! @param rayDirNormalized The direction of the ray normalized.
        //! @param sphereCenter The center of the sphere.
        //! @param sphereRadius Radius of the sphere.
        //! @param[out] t Coefficient in the ray's explicit equation from which an 
        //! intersecting point is calculated as "rayOrigin + t1 * rayDir".
        //! @return SphereIsectTypes
        SphereIsectTypes IntersectRaySphere(
            const Vector3& rayStart, const Vector3& rayDirNormalized, const Vector3& sphereCenter, const float sphereRadius, float& t);

        //! Intersect ray (rayStarty, rayDirNormalized) and disk (center, radius, normal)
        //! @param rayOrigin The origin of the ray to test.
        //! @param rayDir The direction of the ray to test. It has to be unit length.
        //! @param diskCenter Center point of the disk.
        //! @param diskRadius Radius of the disk.
        //! @param diskNormal A normal perpendicular to the disk.
        //! @param[out] t If returning 1 (indicating a hit), this contains distance from rayOrigin along the normalized rayDir
        //! that the hit occured at.
        //! @return False if not interesecting and true if intersecting
        bool IntersectRayDisk(
            const Vector3& rayOrigin,
            const Vector3& rayDir,
            const Vector3& diskCenter,
            const float diskRadius,
            const AZ::Vector3& diskNormal,
            float& t);

        //! If there is only one intersecting point, the coefficient is stored in \ref t1.
        //! @param rayOrigin The origin of the ray to test.
        //! @param rayDir The direction of the ray to test. It has to be unit length.
        //! @param cylinderEnd1 The center of the circle on one end of the cylinder.
        //! @param cylinderDir The direction pointing from \ref cylinderEnd1 to the other end of the cylinder. It has to be unit length.
        //! @param cylinderHeight The distance between two centers of the circles on two ends of the cylinder respectively.
        //! @param[out] t1 A possible coefficient in the ray's explicit equation from which an intersecting point is calculated as "rayOrigin + t1 * rayDir".
        //! @param[out] t2 A possible coefficient in the ray's explicit equation from which an intersecting point is calculated as "rayOrigin + t2 * rayDir".
        //! @return The number of intersecting points.
        int IntersectRayCappedCylinder(
            const Vector3& rayOrigin,
            const Vector3& rayDir,
            const Vector3& cylinderEnd1,
            const Vector3& cylinderDir,
            float cylinderHeight,
            float cylinderRadius,
            float& t1,
            float& t2);

        //! If there is only one intersecting point, the coefficient is stored in \ref t1.
        //! @param rayOrigin The origin of the ray to test.
        //! @param rayDir The direction of the ray to test. It has to be unit length.
        //! @param coneApex The apex of the cone.
        //! @param coneDir The unit-length direction from the apex to the base.
        //! @param coneHeight The height of the cone, from the apex to the base.
        //! @param coneBaseRadius The radius of the cone base circle.
        //! @param[out] t1 A possible coefficient in the ray's explicit equation from which an intersecting point is calculated as "rayOrigin + t1 * rayDir".
        //! @param[out] t2 A possible coefficient in the ray's explicit equation from which an intersecting point is calculated as "rayOrigin + t2 * rayDir".
        //! @return The number of intersecting points.
        int IntersectRayCone(
            const Vector3& rayOrigin,
            const Vector3& rayDir,
            const Vector3& coneApex,
            const Vector3& coneDir,
            float coneHeight,
            float coneBaseRadius,
            float& t1,
            float& t2);

        //! Test intersection between a ray and a plane in 3D.
        //! @param rayOrigin The origin of the ray to test intersection with.
        //! @param rayDir The direction of the ray to test intersection with.
        //! @param planePos A point on the plane to test intersection with.
        //! @param planeNormal The normal of the plane to test intersection with.
        //! @param[out] t The coefficient in the ray's explicit equation from which the intersecting point is calculated as "rayOrigin + t * rayDirection".
        //! @return The number of intersection point.
        int IntersectRayPlane(
            const Vector3& rayOrigin, const Vector3& rayDir, const Vector3& planePos, const Vector3& planeNormal, float& t);

        //! Test intersection between a ray and a two-sided quadrilateral defined by four points in 3D.
        //! The four points that define the quadrilateral could be passed in with either counter clock-wise
        //! winding or clock-wise winding.
        //! @param rayOrigin The origin of the ray to test intersection with.
        //! @param rayDir The direction of the ray to test intersection with.
        //! @param vertexA One of the four points that define the quadrilateral.
        //! @param vertexB One of the four points that define the quadrilateral.
        //! @param vertexC One of the four points that define the quadrilateral.
        //! @param vertexD One of the four points that define the quadrilateral.
        //! @param[out] t The coefficient in the ray's explicit equation from which the 
        //! intersecting point is calculated as "rayOrigin + t * rayDirection".
        //! @return The number of intersection point.
        int IntersectRayQuad(
            const Vector3& rayOrigin,
            const Vector3& rayDir,
            const Vector3& vertexA,
            const Vector3& vertexB,
            const Vector3& vertexC,
            const Vector3& vertexD,
            float& t);

        //!  Test intersection between a ray and an oriented box in 3D.
        //! @param rayOrigin The origin of the ray to test intersection with.
        //! @param rayDir The direction of the ray to test intersection with.
        //! @param boxCenter The position of the center of the box.
        //! @param boxAxis1 An axis along one dimension of the oriented box.
        //! @param boxAxis2 An axis along one dimension of the oriented box.
        //! @param boxAxis3 An axis along one dimension of the oriented box.
        //! @param boxHalfExtent1 The half extent of the box on the dimension of \ref boxAxis1.
        //! @param boxHalfExtent2 The half extent of the box on the dimension of \ref boxAxis2.
        //! @param boxHalfExtent3 The half extent of the box on the dimension of \ref boxAxis3.
        //! @param[out] t The coefficient in the ray's explicit equation from which the intersecting point is calculated as "rayOrigin + t * rayDirection".
        //! @return true if there is an intersection, false otherwise.
        bool IntersectRayBox(
            const Vector3& rayOrigin,
            const Vector3& rayDir,
            const Vector3& boxCenter,
            const Vector3& boxAxis1,
            const Vector3& boxAxis2,
            const Vector3& boxAxis3,
            float boxHalfExtent1,
            float boxHalfExtent2,
            float boxHalfExtent3,
            float& t);

        //! Test intersection between a ray and an OBB.
        //! @param rayOrigin The origin of the ray to test intersection with.
        //! @param rayDir The direction of the ray to test intersection with.
        //! @param obb The OBB to test for intersection with the ray.
        //! @param[out] t The coefficient in the ray's explicit equation from which the intersecting point is calculated as "rayOrigin + t * rayDirection".
        //! @return True if there is an intersection, false otherwise.
        bool IntersectRayObb(const Vector3& rayOrigin, const Vector3& rayDir, const Obb& obb, float& t);

        //! Ray cylinder intersection types.
        enum CylinderIsectTypes : AZ::s32
        {
            RR_ISECT_RAY_CYL_SA_INSIDE = -1, //!< the ray starts inside the cylinder
            RR_ISECT_RAY_CYL_NONE, //!< no intersection
            RR_ISECT_RAY_CYL_PQ, //!< along the PQ segment
            RR_ISECT_RAY_CYL_P_SIDE, //!< on the P side
            RR_ISECT_RAY_CYL_Q_SIDE, //!< on the Q side
        };

        //! Reference: Real-Time Collision Detection - 5.3.7 Intersecting Ray or Segment Against Cylinder
        //! Intersect segment S(t)=sa+t(dir), 0<=t<=1 against cylinder specified by p, q and r.
        //! @param sa The initial point.
        //! @param dir Magnitude and direction for sa.
        //! @param p Center point of side 1 cylinder.
        //! @param q Center point of side 2 cylinder.
        //! @param r Radius of cylinder.
        //! @param[out] t Proporition along line segment.
        //! @return CylinderIsectTypes
        CylinderIsectTypes IntersectSegmentCylinder(
            const Vector3& sa, const Vector3& dir, const Vector3& p, const Vector3& q, const float r, float& t);

        //! Capsule ray intersect types.
        enum CapsuleIsectTypes
        {
            ISECT_RAY_CAPSULE_SA_INSIDE = -1, //!< The ray starts inside the cylinder
            ISECT_RAY_CAPSULE_NONE, //!< No intersection
            ISECT_RAY_CAPSULE_PQ, //!< Along the PQ segment
            ISECT_RAY_CAPSULE_P_SIDE, //!< On the P side
            ISECT_RAY_CAPSULE_Q_SIDE, //!< On the Q side
        };

        //! This is a quick implementation of segment capsule based on segment cylinder \ref IntersectSegmentCylinder
        //! segment sphere intersection. We can optimize it a lot once we fix the ray
        //! cylinder intersection.
        //! @param sa The beginning of the line segment.
        //! @param dir The direction and length of the segment.
        //! @param p Center point of side 1 capsule.
        //! @param q Center point of side 1 capsule.
        //! @param r The radius of the capsule.
        //! @param[out] t Proporition along line segment.
        //! @return CapsuleIsectTypes
        CapsuleIsectTypes IntersectSegmentCapsule(
            const Vector3& sa, const Vector3& dir, const Vector3& p, const Vector3& q, const float r, float& t);

        //! Intersect segment S(t)=A+t(B-A), 0<=t<=1 against convex polyhedron specified
        //! by the n halfspaces defined by the planes p[]. On exit tfirst and tlast
        //! define the intersection, if any.
        //! @param sa The beggining of the line segment.
        //! @param dir The direction and length of the segment.
        //! @param p Planes that compose a convex ponvex polyhedron.
        //! @param numPlanes number of planes.
        //! @param[out] tfirst Proportion along the line segment where the line enters.
        //! @param[out] tlast Proportion along the line segment where the line exits.
        //! @param[out] iFirstPlane The plane where the line enters.
        //! @param[out] iLastPlane The plane where the line exits.
        //! @return True if intersects else false.
        bool IntersectSegmentPolyhedron(
            const Vector3& sa,
            const Vector3& dir,
            const Plane p[],
            int numPlanes,
            float& tfirst,
            float& tlast,
            int& iFirstPlane,
            int& iLastPlane);

        //! Calculate the line segment closestPointSegment1<->closestPointSegment2 that is the shortest route between
        //! two segments segment1Start<->segment1End and segment2Start<->segment2End. Also calculate the values of segment1Proportion and
        //! segment2Proportion where closestPointSegment1 = segment1Start + (segment1Proportion * (segment1End - segment1Start))
        //! closestPointSegment2 = segment2Start + (segment2Proportion * (segment2End - segment2Start))
        //! If segments are parallel returns a solution.
        //! @param segment1Start Start of segment 1.
        //! @param segment1End End of segment 1.
        //! @param segment2Start Start of segment 2.
        //! @param segment2End End of segment 2.
        //! @param[out] segment1Proportion The proporition along segment 1 [0..1]
        //! @param[out] segment2Proportion The proporition along segment 2 [0..1]
        //! @param[out] closestPointSegment1 Closest point on segment 1.
        //! @param[out] closestPointSegment2 Closest point on segment 2.
        //! @param epsilon The minimum square distance where a line segment can be treated as a single point.
        void ClosestSegmentSegment(
            const Vector3& segment1Start,
            const Vector3& segment1End,
            const Vector3& segment2Start,
            const Vector3& segment2End,
            float& segment1Proportion,
            float& segment2Proportion,
            Vector3& closestPointSegment1,
            Vector3& closestPointSegment2,
            float epsilon = 1e-4f);

        //! Calculate the line segment closestPointSegment1<->closestPointSegment2 that is the shortest route between
        //! two segments segment1Start<->segment1End and segment2Start<->segment2End.
        //! If segments are parallel returns a solution.
        //! @param segment1Start Start of segment 1.
        //! @param segment1End End of segment 1.
        //! @param segment2Start Start of segment 2.
        //! @param segment2End End of segment 2.
        //! @param[out] closestPointSegment1 Closest point on segment 1.
        //! @param[out] closestPointSegment2 Closest point on segment 2.
        //! @param epsilon The minimum square distance where a line segment can be treated as a single point.
        void ClosestSegmentSegment(
            const Vector3& segment1Start,
            const Vector3& segment1End,
            const Vector3& segment2Start,
            const Vector3& segment2End,
            Vector3& closestPointSegment1,
            Vector3& closestPointSegment2,
            float epsilon = 1e-4f);

        //! Calculate the point (closestPointOnSegment) that is the closest point on
        //! segment segmentStart/segmentEnd to point. Also calculate the value of proportion where
        //! closestPointOnSegment = segmentStart + (proportion * (segmentEnd - segmentStart))
        //! @param point The point to test
        //! @param segmentStart The start of the segment
        //! @param segmentEnd The end of the segment
        //! @param[out] proportion The proportion of the segment  L(t) = (end - start) * t
        //! @param[out] closestPointOnSegment The point along the line segment
        void ClosestPointSegment(
            const Vector3& point,
            const Vector3& segmentStart,
            const Vector3& segmentEnd,
            float& proportion,
            Vector3& closestPointOnSegment);
    } // namespace Intersect
} // namespace AZ

#include <AzCore/Math/IntersectSegment.inl>
