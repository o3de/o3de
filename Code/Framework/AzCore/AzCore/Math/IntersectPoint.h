/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Plane.h>

namespace AZ
{
    
    //! Intersect
    //! 
    //! This namespace contains, primitive intersections and overlapping tests.
    //! reference RealTimeCollisionDetection, CollisionDetection in interactive 3D environments, etc.
    namespace Intersect
    {

        //! Compute barycentric coordinates (u,v,w) for a point P with respect
        //! to triangle (a,b,c).
        AZ_MATH_INLINE Vector3
        Barycentric(const Vector3& a, const Vector3& b, const Vector3& c, const Vector3& p);

        //! Test if point p lies inside the triangle (a,b,c).
        AZ_MATH_INLINE bool
        TestPointTriangle(const Vector3& p, const Vector3& a, const Vector3& b, const Vector3& c);

        //! Test if point p lies inside the counter clock wise triangle (a,b,c).
        AZ_MATH_INLINE bool
        TestPointTriangleCCW(const Vector3& p, const Vector3& a, const Vector3& b, const Vector3& c);

        //! Find the closest point to 'p' on a plane 'plane'.
        //! @param[out] ptOnPlane closest point on the plane.
        //! @return Distance from the 'p' to 'ptOnPlane'.
        AZ_MATH_INLINE float
        ClosestPointPlane(const Vector3& p, const Plane& plane, Vector3& ptOnPlane);

        //! Find the closest point to 'p' on a triangle (a,b,c).
        //! @return closest point to 'p' on the triangle (a,b,c).
        inline Vector3 ClosestPointTriangle(const Vector3& p, const Vector3& a, const Vector3& b, const Vector3& c);
 
        //! This method checks if a point is inside a sphere or outside it.
        //! @param centerPosition Position of the center of the sphere.
        //! @param radiusSquared square of sphere radius.
        //! @param testPoint Point to be tested.
        //! @return boolean value indicating if the point is inside or not.
        AZ_INLINE bool PointSphere(const Vector3& centerPosition,
            float radiusSquared, const Vector3& testPoint);

        //! This method checks if a point is inside a cylinder or outside it.
        //! @param baseCenterPoint Vector to the base of the cylinder.
        //! @param axisVector Non normalized vector from the base of the cylinder to the other end.
        //! @param axisLengthSquared square of length of "axisVector".
        //! @param radiusSquared square of cylinder radius.
        //! @param testPoint Point to be tested.
        //! @return boolean value indicating if the point is inside or not.
        AZ_INLINE bool PointCylinder(const AZ::Vector3& baseCenterPoint, const AZ::Vector3& axisVector,
            float axisLengthSquared, float radiusSquared, const AZ::Vector3& testPoint);

    } // namespace Intersect
} // namespace AZ
#include <AzCore/Math/IntersectPoint.inl>
