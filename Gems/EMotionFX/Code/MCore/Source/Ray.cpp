/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

// include required headers
#include "Ray.h"
#include "AABB.h"
#include "BoundingSphere.h"
#include "PlaneEq.h"
#include "FastMath.h"
#include "Algorithms.h"


namespace MCore
{
    // ray-boundingsphere
    bool Ray::Intersects(const BoundingSphere& s, AZ::Vector3* intersectA, AZ::Vector3* intersectB) const
    {
        const AZ::Vector3 rayOrg = mOrigin - s.GetCenter(); // ray in space of the sphere

        // The Intersection can be solved by finding the solutions of the quadratic equation:
        // (mOrigin + t * mDirection)^2 - s.GetRadiusSquared() = 0
        // Where t is a value that makes (mOrigin + t * mDirection) intersect the sphere
        // Expanding the above equation we have to find t1 and t2:
        // t1 = (-2 * mOrigin * mDirection + sqrt(delta)) / (2 * mDirection^2)
        // t2 = (-2 * mOrigin * mDirection - sqrt(delta)) / (2 * mDirection^2)
        // where delta = (2 * mOrigin * mDirection) ^ 2 - 4 * mDirection^2 * (mOrigin^2 - s.GetRadiusSquared())
        // The two intersection points will be:
        // mOrigin + mDirection * t1
        // mOrigin + mDirection * t2
        // If delta < 0, then there is no intersection
        // If delta == 0, it intersects int he same point
        //
        const float a = mDirection.GetLengthSq();
        const float b = 2.0f * mDirection.Dot(rayOrg);
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
                    (*intersectA) = mOrigin + mDirection * t1;
                }
                if (intersectB)
                {
                    (*intersectB) = mOrigin + mDirection * t2;
                }
            }
            else
            {
                if (intersectA)
                {
                    (*intersectA) = mOrigin + mDirection * t2;
                }
                if (intersectB)
                {
                    (*intersectB) = mOrigin + mDirection * t1;
                }
            }
        }
        else
        {
            // if we are here it means that delta equals zero and we have only one solution
            const float t = -0.5f * b / a;
            if (intersectA)
            {
                (*intersectA) = mOrigin + mDirection * t;
            }
            if (intersectB)
            {
                (*intersectB) = mOrigin + mDirection * t;
            }
        }

        return true;
    }


    // ray-plane
    bool Ray::Intersects(const PlaneEq& p, AZ::Vector3* intersect) const
    {
        // check if ray is parallel to plane (no intersection) or ray pointing away from plane (no intersection)
        float dot1 = p.GetNormal().Dot(mDirection);
        //if (dot1 >= 0) return false;  // backface cull

        // calc second dot product
        float dot2 = -(p.GetNormal().Dot(mOrigin) + p.GetDist());

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
            intersect->SetX(mOrigin.GetX() + (mDirection.GetX() * t));
            intersect->SetY(mOrigin.GetY() + (mDirection.GetY() * t));
            intersect->SetZ(mOrigin.GetZ() + (mDirection.GetZ() * t));
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
        const AZ::Vector3 dir = mDest - mOrigin;
        const AZ::Vector3 pvec = dir.Cross(edge2);

        // if determinant is near zero, ray lies in plane of triangle
        const float det = edge1.Dot(pvec);
        if (det > -Math::epsilon && det < Math::epsilon)
        {
            return false;
        }

        // calculate distance from vert0 to ray origin
        const AZ::Vector3 tvec = mOrigin - p1;

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
            *intersect = mOrigin + t * dir;
        }

        // yes, there was an intersection
        return true;
    }


    // ray-axis aligned bounding box
    bool Ray::Intersects(const AABB& b, AZ::Vector3* intersectA, AZ::Vector3* intersectB) const
    {
        float tNear = -FLT_MAX, tFar = FLT_MAX;

        const AZ::Vector3& minVec = b.GetMin();
        const AZ::Vector3& maxVec = b.GetMax();

        // For all three axes, check the near and far intersection point on the two slabs
        for (int32 i = 0; i < 3; i++)
        {
            if (Math::Abs(mDirection.GetElement(i)) < Math::epsilon)
            {
                // direction is parallel to this plane, check if we're somewhere between min and max
                if ((mOrigin.GetElement(i) < minVec.GetElement(i)) || (mOrigin.GetElement(i) > maxVec.GetElement(i)))
                {
                    return false;
                }
            }
            else
            {
                // calculate t's at the near and far slab, see if these are min or max t's
                float t1 = (minVec.GetElement(i) - mOrigin.GetElement(i)) / mDirection.GetElement(i);
                float t2 = (maxVec.GetElement(i) - mOrigin.GetElement(i)) / mDirection.GetElement(i);
                if (t1 > t2)
                {
                    float temp = t1;
                    t1 = t2;
                    t2 = temp;
                }
                if (t1 > tNear)
                {
                    tNear = t1;                                 // accept nearest value
                }
                if (t2 < tFar)
                {
                    tFar  = t2;                                 // accept farthest value
                }
                if ((tNear > tFar) || (tFar < 0.0f))
                {
                    return false;
                }
            }
        }

        if (intersectA)
        {
            *intersectA = mOrigin + mDirection * tNear;
        }

        if (intersectB)
        {
            *intersectB = mOrigin + mDirection * tFar;
        }

        return true;
    }
}   // namespace MCore
