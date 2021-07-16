/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_MATH_POINT_TESTS_H
#define AZCORE_MATH_POINT_TESTS_H

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Plane.h>

/// \file isect_point.h

namespace AZ
{
    /**
    * Intersect
    *
    * This namespace contains, primitive intersections and overlapping tests.
    * reference RealTimeCollisionDetection, CollisionDetection in interactive 3D environments, etc.
    */
    namespace Intersect
    {
        /**
         * Compute barycentric coordinates (u,v,w) for a point P with respect
         * to triangle (a,b,c)
         */
        AZ_MATH_INLINE Vector3
        Barycentric(const Vector3& a, const Vector3& b, const Vector3& c, const Vector3& p)
        {
            Vector3 v0 = b - a;
            Vector3 v1 = c - a;
            Vector3 v2 = p - a;

            float d00 = v0.Dot(v0);
            float d01 = v0.Dot(v1);
            float d11 = v1.Dot(v1);
            float d20 = v2.Dot(v0);
            float d21 = v2.Dot(v1);

            float denom = d00 * d11 - d01 * d01;
            float denomRCP = 1.0f / denom;
            float v = (d11 * d20 - d01 * d21) * denomRCP;
            float w = (d00 * d21 - d01 * d20) * denomRCP;
            float u = 1.0f - v - w;
            return Vector3(u, v, w);
        }

        /// Test if point p lies inside the triangle (a,b,c).
        AZ_MATH_INLINE int
        TestPointTriangle(const Vector3& p, const Vector3& a, const Vector3& b, const Vector3& c)
        {
            Vector3 uvw = Barycentric(a, b, c, p);
            return uvw.IsGreaterEqualThan(Vector3::CreateZero());
        }

        /// Test if point p lies inside the counter clock wise triangle (a,b,c).
        AZ_MATH_INLINE int
        TestPointTriangleCCW(const Vector3& p, const Vector3& a, const Vector3& b, const Vector3& c)
        {
            // Translate the triangle so it lies in the origin p.
            Vector3 pA = a - p;
            Vector3 pB = b - p;
            Vector3 pC = c - p;
            float ab = pA.Dot(pB);
            float ac = pA.Dot(pC);
            float bc = pB.Dot(pC);
            float cc = pC.Dot(pC);

            // Make sure plane normals for pab and pac point in the same direction.
            if (bc * ac - cc * ab < 0.0f)
            {
                return 0;
            }

            float bb = pB.Dot(pB);

            // Make sure plane normals for pab and pca point in the same direction.
            if (ab * bc - ac * bb < 0.0f)
            {
                return 0;
            }

            // Otherwise p must be in the triangle.
            return 1;
        }

        /**
         * Find the closest point to 'p' on a plane 'plane'.
         * \return Distance from the 'p' to 'ptOnPlane'
         * \param ptOnPlane closest point on the plane.
         */
        AZ_MATH_INLINE float
        ClosestPointPlane(const Vector3& p, const Plane& plane, Vector3& ptOnPlane)
        {
            float dist = plane.GetPointDist(p);
            // \todo add Vector3 from Vector4 quick function.
            ptOnPlane = p - dist * plane.GetNormal();

            return dist;
        }

        /**
         * Find the closest point to 'p' on a triangle (a,b,c).
         * \return closest point to 'p' on the triangle (a,b,c)
         */
        inline Vector3 ClosestPointTriangle(const Vector3& p, const Vector3& a, const Vector3& b, const Vector3& c)
        {
            // Check if p in vertex region is outside a.
            Vector3 ab = b - a;
            Vector3 ac = c - a;
            Vector3 ap = p - a;
            float d1 = ab.Dot(ap);
            float d2 = ac.Dot(ap);

            if (d1 <= 0.0f && d2 <= 0.0f)
            {
                return a; // barycentric coordinates (1,0,0)
            }
            // Check if p in vertex region is outside b.
            Vector3 bp = p - b;
            float d3 = ab.Dot(bp);
            float d4 = ac.Dot(bp);
            if (d3 >= 0.0f && d4 <= d3)
            {
                return b; // barycentric coordinates (0,1,0)
            }
            // Check if p in edge region of ab, if so return projection of p onto ab.
            float vc = d1 * d4 - d3 * d2;
            if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
            {
                float v = d1 / (d1 - d3);
                return a + v * ab; // barycentric coordinates (1-v,v,0)
            }

            // Check if p in vertex region outside C.
            Vector3 cp = p - c;
            float d5 = ab.Dot(cp);
            float d6 = ac.Dot(cp);
            if (d6 >= 0.0f && d5 <= d6)
            {
                return c; // barycentric coordinates (0,0,1)
            }

            // Check if P in edge region of ac, if so return projection of p onto ac.
            float vb = d5 * d2 - d1 * d6;
            if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
            {
                float w = d2 / (d2 - d6);
                return a + w * ac; // barycentric coordinates (1-w,0,w)
            }

            // Check if p in edge region of bc, if so return projection of p onto bc.
            float va = d3 * d6 - d5 * d4;
            if (va <= 0.0f && d4 >= d3 && d5 >= d6)
            {
                float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
                return b + w * (c - b);  // barycentric coordinates (0,1-w,w)
            }

            // p inside face region. Compute q trough its barycentric coordinates (u,v,w)
            float denomRCP = 1.0f / (va + vb + vc);
            float v = vb * denomRCP;
            float w = vc * denomRCP;
            return a + ab * v + ac * w; // = u*a + v*b + w*c, u = va * denom = 1.0 - v - w
        }

        /**
        * \brief This method checks if a point is inside a sphere or outside it
        * \param centerPosition Position of the center of the sphere
        * \param radiusSquared square of cylinder radius
        * \param testPoint Point to be tested
        * \return boolean value indicating if the point is inside or not
        */
        AZ_INLINE bool PointSphere(const Vector3& centerPosition,
            float radiusSquared, const Vector3& testPoint)
        {
            return testPoint.GetDistanceSq(centerPosition) < radiusSquared;
        }

        /**
        * \brief This method checks if a point is inside a cylinder or outside it
        * \param baseCenterPoint Vector to the base of the cylinder
        * \param axisVector Non normalized vector from the base of the cylinder to the other end
        * \param axisLengthSquared square of length of "axisVector"
        * \param radiusSquared square of cylinder radius
        * \param testPoint Point to be tested
        * \return boolean value indicating if the point is inside or not
        */
        AZ_INLINE bool PointCylinder(const AZ::Vector3& baseCenterPoint, const AZ::Vector3& axisVector,
            float axisLengthSquared, float radiusSquared, const AZ::Vector3& testPoint)
        {
            // If the cylinder shape has no volume then the point cannot be inside.
            if (axisLengthSquared <= 0.0f || radiusSquared <= 0.0f)
            {
                return false;
            }

            AZ::Vector3 baseCenterPointToTestPoint = testPoint - baseCenterPoint;
            float dotProduct = baseCenterPointToTestPoint.Dot(axisVector);

            // If the dot is < 0, the point is below the base cap of the cylinder, if it's > lengthSquared then it's beyond the other cap.
            if (dotProduct < 0.0f || dotProduct > axisLengthSquared)
            {
                return false;
            }
            else
            {
                float distanceSquared = (baseCenterPointToTestPoint.GetLengthSq()) - (dotProduct * dotProduct / axisLengthSquared);
                return distanceSquared <= radiusSquared;
            }
        }
    }
}

#endif // AZCORE_MATH_POINT_TESTS_H
#pragma once
