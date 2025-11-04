/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include "Ray.h"
#include "BoundingSphere.h"
#include "PlaneEq.h"
#include "FastMath.h"
#include "Algorithms.h"


namespace MCore
{
    // ray-boundingsphere
    bool Ray::Intersects(const BoundingSphere& s, AZ::Vector3* intersectA, AZ::Vector3* intersectB) const
    {
        const AZ::Vector3 rayOrg = m_origin - s.GetCenter(); // ray in space of the sphere

        // The Intersection can be solved by finding the solutions of the quadratic equation:
        // (m_origin + t * m_direction)^2 - s.GetRadiusSquared() = 0
        // Where t is a value that makes (m_origin + t * m_direction) intersect the sphere
        // Expanding the above equation we have to find t1 and t2:
        // t1 = (-2 * m_origin * m_direction + sqrt(delta)) / (2 * m_direction^2)
        // t2 = (-2 * m_origin * m_direction - sqrt(delta)) / (2 * m_direction^2)
        // where delta = (2 * m_origin * m_direction) ^ 2 - 4 * m_direction^2 * (m_origin^2 - s.GetRadiusSquared())
        // The two intersection points will be:
        // m_origin + m_direction * t1
        // m_origin + m_direction * t2
        // If delta < 0, then there is no intersection
        // If delta == 0, it intersects int he same point
        //
        const float a = m_direction.GetLengthSq();
        const float b = 2.0f * m_direction.Dot(rayOrg);
        const float c = rayOrg.GetLengthSq() - s.GetRadiusSquared();
        const float delta = ((b * b) - 4.0f * a * c);

        if (delta < 0.0f)
        {
            return false; // no intersection
        }

        if (intersectA == nullptr && intersectB == nullptr)
        {
            // Early out if we are not interested in getting the intersection but to know if there was or not
            // intersection. If delta is positive, then we have solutions
            return true;
        }

        if (delta < Math::epsilon)
        {
            const float q = (b > 0)
                ? (-0.5f * (b + Math::Sqrt(delta)))
                : (-0.5f * (b - Math::Sqrt(delta)));
            const float t1 = q / a;
            const float t2 = c / q;
            if (t1 < t2)
            {
                if (intersectA)
                {
                    (*intersectA) = m_origin + m_direction * t1;
                }
                if (intersectB)
                {
                    (*intersectB) = m_origin + m_direction * t2;
                }
            }
            else
            {
                if (intersectA)
                {
                    (*intersectA) = m_origin + m_direction * t2;
                }
                if (intersectB)
                {
                    (*intersectB) = m_origin + m_direction * t1;
                }
            }
        }
        else
        {
            // if we are here it means that delta equals zero and we have only one solution
            const float t = -0.5f * b / a;
            if (intersectA)
            {
                (*intersectA) = m_origin + m_direction * t;
            }
            if (intersectB)
            {
                (*intersectB) = m_origin + m_direction * t;
            }
        }

        return true;
    }


    // ray-plane
    bool Ray::Intersects(const PlaneEq& p, AZ::Vector3* intersect) const
    {
        // check if ray is parallel to plane (no intersection) or ray pointing away from plane (no intersection)
        float dot1 = p.GetNormal().Dot(m_direction);
        //if (dot1 >= 0) return false;  // backface cull

        // calc second dot product
        float dot2 = -(p.GetNormal().Dot(m_origin) + p.GetDist());

        // calc t value
        float t = dot2 / dot1;

        // if t<0 then the line defined by the ray, intersects the plane behind the rays origin and so no
        // intersection occurs. else we can calculate the intersection point
        if (t < 0.0f)
        {
            return false;
        }
        if (t > Length())
        {
            return false;
        }

        // calc intersection point
        if (intersect)
        {
            intersect->SetX(m_origin.GetX() + (m_direction.GetX() * t));
            intersect->SetY(m_origin.GetY() + (m_direction.GetY() * t));
            intersect->SetZ(m_origin.GetZ() + (m_direction.GetZ() * t));
        }

        return true;
    }


    // ray-triangle intersection test
    bool Ray::Intersects(const AZ::Vector3& p1, const AZ::Vector3& p2, const AZ::Vector3& p3, AZ::Vector3* intersect, float* baryU, float* baryV) const
    {
        // calculate two vectors of the polygon
        const AZ::Vector3 edge1 = p2 - p1;
        const AZ::Vector3 edge2 = p3 - p1;

        // begin calculating determinant - also used to calculate U parameter
        const AZ::Vector3 dir = m_dest - m_origin;
        const AZ::Vector3 pvec = dir.Cross(edge2);

        // if determinant is near zero, ray lies in plane of triangle
        const float det = edge1.Dot(pvec);
        if (det > -Math::epsilon && det < Math::epsilon)
        {
            return false;
        }

        // calculate distance from vert0 to ray origin
        const AZ::Vector3 tvec = m_origin - p1;

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
        const float v = dir.Dot(qvec) * inv_det;
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
        if (baryU)
        {
            *baryU = u;
        }
        if (baryV)
        {
            *baryV = v;
        }
        if (intersect)
        {
            *intersect = m_origin + t * dir;
        }

        // yes, there was an intersection
        return true;
    }

}   // namespace MCore
