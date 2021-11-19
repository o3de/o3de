/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace AZ
{
    namespace Intersect
    {
        AZ_MATH_INLINE bool ClipRayWithAabb(const Aabb& aabb, Vector3& rayStart, Vector3& rayEnd, float& tClipStart, float& tClipEnd)
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

                return true;
            }

            return false;
        }

        AZ_MATH_INLINE SphereIsectTypes
        IntersectRaySphereOrigin(const Vector3& rayStart, const Vector3& rayDirNormalized, const float sphereRadius, float& t)
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
            // q = p + t * d;
            return ISECT_RAY_SPHERE_ISECT;
        }

        AZ_MATH_INLINE SphereIsectTypes IntersectRaySphere(const Vector3& rayStart, const Vector3& rayDirNormalized, const Vector3& sphereCenter, const float sphereRadius, float& t)
        {
            return IntersectRaySphereOrigin(rayStart - sphereCenter, rayDirNormalized, sphereRadius, t);
        }

        AZ_MATH_INLINE Vector3 LineToPointDistance(const Vector3& s1, const Vector3& s2, const Vector3& p, float& u)
        {
            const Vector3 s21 = s2 - s1;
            // we assume seg1 and seg2 are NOT coincident
            AZ_MATH_ASSERT(!s21.IsClose(Vector3(0.0f), 1e-4f), "OK we agreed that we will pass valid segments! (s1 != s2)");

            u = LineToPointDistanceTime(s1, s21, p);

            return s1 + u * s21;
        }

        AZ_MATH_INLINE float LineToPointDistanceTime(const Vector3& s1, const Vector3& s21, const Vector3& p)
        {
            // so u = (p.x - s1.x)*(s2.x - s1.x) + (p.y - s1.y)*(s2.y - s1.y) + (p.z-s1.z)*(s2.z-s1.z) /  |s2-s1|^2
            return s21.Dot(p - s1) / s21.Dot(s21);
        }

        AZ_MATH_INLINE bool TestSegmentAABB(const Vector3& p0, const Vector3& p1, const Aabb& aabb)
        {
            Vector3 e = aabb.GetExtents();
            Vector3 d = p1 - p0;
            Vector3 m = p0 + p1 - aabb.GetMin() - aabb.GetMax();

            return TestSegmentAABBOrigin(m, d, e);
        }
    } // namespace Intersect
} // namespace AZ
