/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AzCore/Math/Vector2.h"
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Ray.h>
#include <AzCore/Math/Vector3.h>

namespace AZ
{
    namespace RayIntersection
    {
        bool RayTriangle(
            const AZ::Ray& ray,
            const AZ::Vector3& p1,
            const AZ::Vector3 p2,
            const AZ::Vector3& p3,
            AZ::Vector3& hitPoint,
            AZ::Vector2& barycentricUV)
        {
            // calculate two vectors of the polygon
            const AZ::Vector3 edge1 = p2 - p1;
            const AZ::Vector3 edge2 = p3 - p1;

            // begin calculating determinant - also used to calculate U parameter
            const AZ::Vector3 pvec = ray.GetDirection().Cross(edge2);

            // if determinant is near zero, ray lies in plane of triangle
            const float det = edge1.Dot(pvec);
            if (det > -Constants::Tolerance && det < Constants::Tolerance)
            {
                return false;
            }

            // calculate distance from vert0 to ray origin
            const AZ::Vector3 tvec = ray.GetOrigin() - p1;

            // calculate U parameter and test bounds
            const float inv_det = 1.0f / det;
            const float u = tvec.Dot(pvec) * inv_det;
            if (u < 0.0f || u > 1.0f)
            {
                return false;
            }

            // prepare to test V parameter
            const AZ::Vector3 qvec = tvec.Cross(edge1);

            // calculate V parameter and test bounds
            const float v = ray.GetDirection().Dot(qvec) * inv_det;
            if (v < 0.0f || u + v > 1.0f)
            {
                return false;
            }

            // calculate t, ray intersects triangle
            const float t = edge2.Dot(qvec) * inv_det;
            if (t < 0.0f || t > 1.0f)
            {
                return false;
            }
            
            // output the results
            barycentricUV = AZ::Vector2(u, v);
            hitPoint = ray.GetOrigin() + t * ray.GetDirection();

            // yes, there was an intersection
            return true;
        }
    } // namespace RayIntersection
} // namespace AZ
