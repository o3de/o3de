/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/IntersectSegment.h>

using namespace AZ;
using namespace Intersect;

//=========================================================================
// IntersectSegmentTriangleCCW
// [10/21/2009]
//=========================================================================
bool Intersect::IntersectSegmentTriangleCCW(
    const Vector3& p, const Vector3& q, const Vector3& a, const Vector3& b, const Vector3& c,
    /*float &u, float &v, float &w,*/ Vector3& normal, float& t)
{
    float v, w; // comment this and enable input params if we need the barycentric coordinates

    Vector3 ab = b - a;
    Vector3 ac = c - a;
    Vector3 qp = p - q;

    // Compute triangle normal. Can be pre-calculated/cached if
    // intersecting multiple segments against the same triangle
    normal = ab.Cross(ac);  // Right hand CCW

    // Compute denominator d. If d <= 0, segment is parallel to or points
    // away from triangle, so exit early
    float d = qp.Dot(normal);
    if (d <= 0.0f)
    {
        return false;
    }

    // Compute intersection t value of pq with plane of triangle. A ray
    // intersects iff 0 <= t. Segment intersects iff 0 <= t <= 1. Delay
    // dividing by d until intersection has been found to pierce triangle
    Vector3 ap = p - a;
    t = ap.Dot(normal);

    // range segment check t[0,1] (it this case [0,d])
    if (t < 0.0f || t > d)
    {
        return false;
    }

    // Compute barycentric coordinate components and test if within bounds
    Vector3 e = qp.Cross(ap);
    v = ac.Dot(e);
    if (v < 0.0f || v > d)
    {
        return false;
    }
    w = -ab.Dot(e);
    if (w < 0.0f || v + w > d)
    {
        return false;
    }

    // Segment/ray intersects triangle. Perform delayed division and
    // compute the last barycentric coordinate component
    float ood = 1.0f / d;
    t *= ood;
    /*v *= ood;
    w *= ood;
    u = 1.0f - v - w;*/

    normal.Normalize();

    return true;
}

//=========================================================================
// IntersectSegmentTriangle
// [10/21/2009]
//=========================================================================
bool
Intersect::IntersectSegmentTriangle(
    const Vector3& p, const Vector3& q, const Vector3& a, const Vector3& b, const Vector3& c,
    /*float &u, float &v, float &w,*/ Vector3& normal, float& t)
{
    float v, w; // comment this and enable input params if we need the barycentric coordinates

    Vector3 ab = b - a;
    Vector3 ac = c - a;
    Vector3 qp = p - q;
    Vector3 ap = p - a;

    // Compute triangle normal. Can be pre-calculated or cached if
    // intersecting multiple segments against the same triangle
    normal = ab.Cross(ac);  // Right hand CCW

    // Compute denominator d. If d <= 0, segment is parallel to or points
    // away from triangle, so exit early
    float d = qp.Dot(normal);
    Vector3 e;
    if (d > Constants::FloatEpsilon)
    {
        // the normal is on the right side
        e = qp.Cross(ap);
    }
    else
    {
        normal = -normal;

        // so either have a parallel ray or our normal is flipped
        if (d >= -Constants::FloatEpsilon)
        {
            return false; // parallel
        }
        d = -d;
        e = ap.Cross(qp);
    }

    // Compute intersection t value of pq with plane of triangle. A ray
    // intersects iff 0 <= t. Segment intersects iff 0 <= t <= 1. Delay
    // dividing by d until intersection has been found to pierce triangle
    t = ap.Dot(normal);

    // range segment check t[0,1] (it this case [0,d])
    if (t < 0.0f || t > d)
    {
        return false;
    }

    // Compute barycentric coordinate components and test if within bounds
    v = ac.Dot(e);
    if (v < 0.0f || v > d)
    {
        return false;
    }
    w = -ab.Dot(e);
    if (w < 0.0f || v + w > d)
    {
        return false;
    }

    // Segment/ray intersects the triangle. Perform delayed division and
    // compute the last barycentric coordinate component
    float ood = 1.0f / d;
    t *= ood;
    //v *= ood;
    //w *= ood;
    //u = 1.0f - v - w;

    normal.Normalize();

    return true;
}

//=========================================================================
// TestSegmentAABBOrigin
// [10/21/2009]
//=========================================================================
bool
AZ::Intersect::TestSegmentAABBOrigin(const Vector3& midPoint, const Vector3& halfVector, const Vector3& aabbExtends)
{
    const Vector3 EPSILON(0.001f); // \todo this is slow load move to a const
    Vector3 absHalfVector = halfVector.GetAbs();
    Vector3 absMidpoint = midPoint.GetAbs();
    Vector3 absHalfMidpoint = absHalfVector + aabbExtends;

    // Try world coordinate axes as separating axes
    if (!absMidpoint.IsLessEqualThan(absHalfMidpoint))
    {
        return false;
    }

    // Add in an epsilon term to counteract arithmetic errors when segment is
    // (near) parallel to a coordinate axis (see text for detail)
    absHalfVector += EPSILON;

    // Try cross products of segment direction vector with coordinate axes
    Vector3 absMDCross = midPoint.Cross(halfVector).GetAbs();
    //Vector3 eaDCross = absHalfVector.Cross(aabbExtends);
    float ex = aabbExtends.GetX();
    float ey = aabbExtends.GetY();
    float ez = aabbExtends.GetZ();
    float adx = absHalfVector.GetX();
    float ady = absHalfVector.GetY();
    float adz = absHalfVector.GetZ();

    Vector3 ead(ey * adz + ez * ady, ex * adz + ez * adx, ex * ady + ey * adx);
    if (!absMDCross.IsLessEqualThan(ead))
    {
        return false;
    }

    // No separating axis found; segment must be overlapping AABB
    return true;
}


//=========================================================================
// IntersectRayAABB
// [10/21/2009]
//=========================================================================
RayAABBIsectTypes
AZ::Intersect::IntersectRayAABB(
    const Vector3& rayStart, const Vector3& dir, const Vector3& dirRCP, const Aabb& aabb,
    float& tStart, float& tEnd, Vector3& startNormal /*, Vector3& inter*/)
{
    // we don't need to test with all 6 normals (just 3)

    const float eps = 0.0001f;  // \todo move to constant
    float tmin = 0.0f;  // set to -RR_FLT_MAX to get first hit on line
    float tmax = std::numeric_limits<float>::max();    // set to max distance ray can travel (for segment)

    const Vector3& aabbMin = aabb.GetMin();
    const Vector3& aabbMax = aabb.GetMax();

    // we unroll manually because there is no way to get in efficient way vectors for
    // each axis while getting it as a index
    Vector3 time1 = (aabbMin - rayStart) * dirRCP;
    Vector3 time2 = (aabbMax - rayStart) * dirRCP;

    // X
    if (std::fabs(dir.GetX()) < eps)
    {
        // Ray is parallel to slab. No hit if origin not within slab
        if (rayStart.GetX() < aabbMin.GetX() || rayStart.GetX() > aabbMax.GetX())
        {
            return ISECT_RAY_AABB_NONE;
        }
    }
    else
    {
        // Compute intersection t value of ray with near and far plane of slab
        float t1 = time1.GetX();
        float t2 = time2.GetX();
        float nSign = -1.0f;

        // Make t1 be intersection with near plane, t2 with far plane
        if (t1 > t2)
        {
            AZStd::swap(t1, t2);
            nSign = 1.0f;
        }

        // Compute the intersection of slab intersections intervals
        if (tmin < t1)
        {
            tmin = t1;

            startNormal.Set(nSign, 0.0f, 0.0f);
        }

        tmax = AZ::GetMin(tmax, t2);

        // Exit with no collision as soon as slab intersection becomes empty
        if (tmin > tmax)
        {
            return ISECT_RAY_AABB_NONE;
        }
    }

    // Y
    if (std::fabs(dir.GetY()) < eps)
    {
        // Ray is parallel to slab. No hit if origin not within slab
        if (rayStart.GetY() < aabbMin.GetY() || rayStart.GetY() > aabbMax.GetY())
        {
            return ISECT_RAY_AABB_NONE;
        }
    }
    else
    {
        // Compute intersection t value of ray with near and far plane of slab
        float t1 = time1.GetY();
        float t2 = time2.GetY();
        float nSign = -1.0f;

        // Make t1 be intersection with near plane, t2 with far plane
        if (t1 > t2)
        {
            AZStd::swap(t1, t2);
            nSign = 1.0f;
        }

        // Compute the intersection of slab intersections intervals
        if (tmin < t1)
        {
            tmin = t1;

            startNormal.Set(0.0f, nSign, 0.0f);
        }

        tmax = AZ::GetMin(tmax, t2);

        // Exit with no collision as soon as slab intersection becomes empty
        if (tmin > tmax)
        {
            return ISECT_RAY_AABB_NONE;
        }
    }

    // Z
    if (std::fabs(dir.GetZ()) < eps)
    {
        // Ray is parallel to slab. No hit if origin not within slab
        if (rayStart.GetZ() < aabbMin.GetZ() || rayStart.GetZ() > aabbMax.GetZ())
        {
            return ISECT_RAY_AABB_NONE;
        }
    }
    else
    {
        // Compute intersection t value of ray with near and far plane of slab
        float t1 = time1.GetZ();
        float t2 = time2.GetZ();
        float nSign = -1.0f;

        // Make t1 be intersection with near plane, t2 with far plane
        if (t1 > t2)
        {
            AZStd::swap(t1, t2);
            nSign = 1.0f;
        }

        // Compute the intersection of slab intersections intervals
        if (tmin < t1)
        {
            tmin = t1;

            startNormal.Set(0.0f, 0.0f, nSign);
        }

        tmax = AZ::GetMin(tmax, t2);

        // Exit with no collision as soon as slab intersection becomes empty
        if (tmin > tmax)
        {
            return ISECT_RAY_AABB_NONE;
        }
    }

    tStart = tmin;
    tEnd = tmax;

    if (tmin == 0.0f)  // no intersect if the segments starts inside or coincident the aabb
    {
        return ISECT_RAY_AABB_SA_INSIDE;
    }

    // Ray intersects all 3 slabs. Return point (q) and intersection t value (tmin)
    //inter = rayStart + dir * tmin;
    return ISECT_RAY_AABB_ISECT;
}

//=========================================================================
// IntersectRayAABB2
// [2/18/2011]
//=========================================================================
RayAABBIsectTypes
AZ::Intersect::IntersectRayAABB2(const Vector3& rayStart, const Vector3& dirRCP, const Aabb& aabb, float& start, float& end)
{
    float tmin, tmax, tymin, tymax, tzmin, tzmax;
    Vector3 vZero = Vector3::CreateZero();

    Vector3 min = (Vector3::CreateSelectCmpGreaterEqual(dirRCP, vZero, aabb.GetMin(), aabb.GetMax()) - rayStart) * dirRCP;
    Vector3 max = (Vector3::CreateSelectCmpGreaterEqual(dirRCP, vZero, aabb.GetMax(), aabb.GetMin()) - rayStart) * dirRCP;

    tmin = min.GetX();
    tmax = max.GetX();
    tymin = min.GetY();
    tymax = max.GetY();

    if (tmin > tymax || tymin > tmax)
    {
        return ISECT_RAY_AABB_NONE;
    }

    if (tymin > tmin)
    {
        tmin = tymin;
    }

    if (tymax < tmax)
    {
        tmax = tymax;
    }

    tzmin = min.GetZ();
    tzmax = max.GetZ();

    if (tmin > tzmax || tzmin > tmax)
    {
        return ISECT_RAY_AABB_NONE;
    }

    if (tzmin > tmin)
    {
        tmin = tzmin;
    }
    if (tzmax < tmax)
    {
        tmax = tzmax;
    }

    start = tmin;
    end = tmax;

    return ISECT_RAY_AABB_ISECT;
}

bool AZ::Intersect::IntersectRayDisk(
    const Vector3& rayOrigin, const Vector3& rayDir, const Vector3& diskCenter, const float diskRadius, const Vector3& diskNormal, float& t)
{
    // First intersect with the plane of the disk
    float planeIntersectionDistance;
    int intersectionCount = IntersectRayPlane(rayOrigin, rayDir, diskCenter, diskNormal, planeIntersectionDistance);
    if (intersectionCount == 1)
    {
        // If the plane intersection point is inside the disk radius, then it intersected the disk.
        Vector3 pointOnPlane = rayOrigin + rayDir * planeIntersectionDistance;
        if (pointOnPlane.GetDistance(diskCenter) < diskRadius)
        {
            t = planeIntersectionDistance;
            return true;
        }
    }
    return false;
}

// Reference: Real-Time Collision Detection - 5.3.7 Intersecting Ray or Segment Against Cylinder, and the book's errata.
int AZ::Intersect::IntersectRayCappedCylinder(
    const Vector3& rayOrigin, const Vector3& rayDir,
    const Vector3& cylinderEnd1, const Vector3& cylinderDir,
    float cylinderHeight, float cylinderRadius, float &t1, float &t2)
{
    // dr = rayDir
    // dc = cylinderDir
    // r = cylinderRadius
    // Vector3 cylinderEnd2 = cylinderEnd1 + cylinderHeight * cylinderDir;
    Vector3 m = rayOrigin - cylinderEnd1; // vector from cylinderEnd1 to rayOrigin
    float dcm = cylinderDir.Dot(m); // projection of m on cylinderDir
    float dcdr = cylinderDir.Dot(rayDir); // projection of rayDir on cylinderDir
    float drm = rayDir.Dot(m); // projection of m on rayDir
    float r2 = cylinderRadius * cylinderRadius;

    if (dcm < 0.0f && dcdr <= 0.0f)
    {
        return 0; // rayOrigin is outside cylinderEnd1 and rayDir is pointing away from cylinderEnd1
    }
    if (dcm > cylinderHeight && dcdr >= 0.0f)
    {
        return 0; // rayOrigin is outside cylinderEnd2 and rayDir is pointing away from cylinderEnd2
    }

    // point RP on the ray: RP(t) = rayOrigin + t * rayDir
    // point CP on the cylinder surface: |(CP - cylinderEnd1) - cylinderDir.Dot(cp - cylinderEnd1) * cylinderDir|^2 = cylinderRadius^2
    // substitute RP(t) for CP: a*t^2 + 2b*t + c = 0, solving for t = [-2b +/- sqrt(4b^2 - 4ac)] / 2a
    float a = 1.0f - dcdr * dcdr; // always greater than or equal to 0
    float b = drm - dcm * dcdr;
    float c = m.Dot(m) - dcm * dcm - r2;

    const float EPSILON = 0.00001f;

    if (fabsf(a) < EPSILON) // the ray is parallel to the cylinder
    {
        if (c > EPSILON) // the ray is outside the cylinder
        {
            return 0;
        }
        else if (dcm < 0.0f) // the ray origin is on cylinderEnd1 side and ray is pointing to cylinderEnd2
        {
            t1 = -dcm;
            t2 = -dcm + cylinderHeight;
            return 2;
        }
        else if (dcm > cylinderHeight) // the ray origin is on cylinderEnd2 side and ray is pointing to cylinderEnd1
        {
            t1 = dcm - cylinderHeight;
            t2 = dcm;
            return 2;
        }
        else // (dcm > 0.0f && dcm < cylinderHeight) // the ray origin is inside the cylinder
        {
            if (dcdr > 0.0f) // the ray is pointing to cylinderEnd2
            {
                t1 = cylinderHeight - dcm;
                return 1;
            }
            else if (dcdr < 0.0f) // the ray is pointing to cylinderEnd1
            {
                t2 = dcm;
                return 1;
            }
            else // impossible in theory        
            {
                return 0;
            }
        }
    }

    float discr = b * b - a * c;
    if (discr < 0.0f)
    {
        return 0;
    }

    float sqrt_discr = sqrt(discr);
    float tt1 = (-b - sqrt_discr) / a;
    float tt2 = (-b + sqrt_discr) / a;

    if (tt2 < 0.0f) // both intersections are behind the ray origin
    {
        return 0;
    }

    // Vector3 AP2 = (rayOrigin + tt2 * rayDir) - cylinderEnd1; // vector from cylinderEnd1 to the intersecting point of parameter tt2
    // float s2 = cylinderDir.Dot(AP2);
    float s2 = dcm + tt2 * dcdr;

    if (discr < EPSILON) // tt1 == tt2
    {
        if (s2 >= 0.0f && s2 <= cylinderHeight)
        {
            t1 = tt1;
            return 1;
        }
    }

    // Vector3 AP1 = (rayOrigin + tt1 * rayDir) - cylinderEnd1; // vector from cylinderEnd1 to the intersecting point of parameter tt1
    // float s1 = cylinderDir.Dot(AP1);
    float s1 = dcm + tt1 * dcdr;

    if (s1 < 0.0f) // intersecting point of parameter tt1 is outside on cylinderEnd1 side
    {
        if (s2 < 0.0f) // intersecting point of parameter tt2 is outside on cylinderEnd1 side
        {
            return 0;
        }
        else if (s2 == 0.0f) // ray touching the brim of the cylinderEnd1
        {
            t1 = tt2;
            return 1;
        }
        else
        {
            if (s2 > cylinderHeight) // intersecting point of parameter tt2 is outside on cylinderEnd2 side
            {
                // t2 can be computed from the equation: dot(rayOrigin + t2 * rayDir - cylinderEnd1, cylinderDir) = cylinderHeight
                t2 = (cylinderHeight - dcm) / dcdr; 
            }
            else
            {
                t2 = tt2;
            }
            if (dcm > 0.0f) // ray origin inside cylinder
            {
                t1 = t2;
                return 1;
            }
            else
            {
                // t1 can be computed from the equation: dot(rayOrigin + t1 * rayDir - cylinderEnd1, cylinderDir) = 0
                t1 = -dcm / dcdr;
                return 2;
            }
        }
    }
    else if (s1 > cylinderHeight) // intersecting point of parameter tt1 is outside on cylinderEnd2 side
    {
        if (s2 > cylinderHeight) // intersecting point of parameter tt2 is outside on cylinderEnd2 side
        {
            return 0;
        }
        else if (s2 == cylinderHeight)
        {
            t1 = tt2;
            return 1;
        }
        else
        {
            if (s2 < 0.0f)
            {
                t2 = -dcm / dcdr;
            }
            else
            {
                t2 = tt2;
            }
            if (dcm < cylinderHeight)
            {
                t1 = t2;
                return 1;
            }
            else
            {
                t1 = (cylinderHeight - dcm) / dcdr;
                return 2;
            }
        }
    }
    else // intersecting point of parameter tt1 is in between two cylinder ends
    {
        if (s2 < 0.0f)
        {
            t2 = -dcm / dcdr;
        }
        else if (s2 > cylinderHeight)
        {
            t2 = (cylinderHeight - dcm) / dcdr;
        }
        else
        {
            t2 = tt2;
        }
        if (tt1 > 0.0f)
        {
            t1 = tt1;
            return 2;
        }
        else
        {
            t1 = t2;
            return 1;
        }
    }
}

int AZ::Intersect::IntersectRayCone(
    const Vector3& rayOrigin, const Vector3& rayDir,
    const Vector3& coneApex, const Vector3& coneDir, float coneHeight,
    float coneBaseRadius, float& t1, float& t2)
{
    // Q = rayOrgin, A = coneApex
    Vector3 AQ = rayOrigin - coneApex;
    float m = coneDir.Dot(AQ); // projection of m on cylinderDir
    float k = coneDir.Dot(rayDir); // projection of rayDir on cylinderDir
    
    if (m < 0.0f && k <= 0.0f)
    {
        // rayOrigin is outside the cone on coneApex side and rayDir is pointing away
        return 0; 
    }
    if (m > coneHeight && k >= 0.0f)
    {
        // rayOrigin is outside the cone on coneBase side and rayDir is pointing away
        return 0;
    }

    float r2 = coneBaseRadius * coneBaseRadius;
    float h2 = coneHeight * coneHeight;

    float m2 = m * m;
    float k2 = k * k;
    float q2 = AQ.Dot(AQ);
    
    float n = rayDir.Dot(AQ);

    const float EPSILON = 0.00001f;

    // point RP on the ray: RP(t) = rayOrigin + t * rayDir
    // point CP on the cone surface: similar triangle property
    //     |dot(CP - A, coneDir) * coneDir| / coneHeight = |(CP - A) - (dot(CP - A, coneDir) * coneDir)| coneRadius
    // substitute RP(t) for CP: a*t^2 + 2b*t + c = 0, solving for t
    float a = (r2 + h2) * k2 - h2;
    float b = (r2 + h2) * m * k - h2 * n;
    float c = (r2 + h2) * m2 - h2 * q2;

    float discriminant = b * b - a * c;
    if (discriminant < -EPSILON)
    {
        return 0;
    }
    discriminant = AZ::GetMax(discriminant, 0.0f);

    if (fabsf(a) < EPSILON) // the ray is parallel to the cone surface's tangent line
    {
        if (b < EPSILON && fabsf(c) < EPSILON) // ray overlapping with cone surface
        { 
            t1 = rayDir.Dot(coneApex - rayOrigin);
        }
        else // ray has only one intersecting point with the cone
        {
            t1 = -c / (2 * b);
        }

        t2 = (coneHeight - m) / k; // t2 can be computed from the equation: dot(Q + t2 * rayDir - A, coneDir) = coneHeight

        if (t1 < 0.0f && t2 < 0.0f)
        {
            return 0;
        }

        if (fabsf(t1 - t2) < EPSILON) // the ray intersects the brim of the circumference of the cone base
        {
            return 1;
        }

        float s1 = m + t1 * k; // coneDir.Dot(rayOrigin + t1 * rayDir - coneApex);
        if (s1 < 0.0f || s1 > coneHeight)
        {
            return 0;
        }
        else
        {
            if (k < 0.0f) // ray shooting from base to apex
            {
                if (m >= coneHeight) // ray origin outside cone
                {
                    float temp = t1;
                    t1 = t2;
                    t2 = temp;
                    return 2;
                }
                else if (t1 >= 0.0f) // ray origin inside cone
                {
                    t1 = t2;
                    return 1;
                }
                else
                {
                    return 0;
                }
            }
            else
            {
                if (m > coneHeight)
                {
                    return 0;
                }
                if (t1 >= 0.0f) // ray origin outside cone
                {
                    return 2;
                }
                else
                {
                    t1 = t2;
                    return 1;
                }
            }
        }
    }

    if (discriminant < EPSILON) // two intersecting points coincide
    {
        if (fabsf(n * n - q2) < EPSILON) // the ray is through the apex
        {
            float cosineA2 = h2 / (r2 + h2);
            float cosineAQ2 = cosineA2 * q2;

            if (m2 > cosineAQ2) // the ray origin is inside the cone or its mirroring counterpart
            {
                if (m <= 0.0f) // the ray origin outside the cone on the apex side, shooting towards the base
                {
                    t1 = -b / a;
                    t2 = (coneHeight - m) / k;
                    return 2;
                }
                else if (m >= coneHeight) // the ray origin is outside the cone on the base side, shooting towards towards the apex
                {
                    t1 = (coneHeight - m) / k;
                    t2 = -b / a;
                    return 2;
                }
                else
                {
                    if (k > 0.0f) // the ray origin is inside the cone, shooting towards the base
                    {
                        t1 = (coneHeight - m) / k;
                        return 1;
                    }
                    else // the ray origin is inside the cone, shooting towards the apex
                    {
                        t1 = -b / a;
                        return 1;
                    }
                }
            }
            else // the ray origin is outside the cone
            {
                t1 = -b / a;
                if (t1 > 0.0f)
                {
                    return 1;
                }
                else
                {
                    return 0;
                }
            }
        }
        else // the ray is touching the cone surface but not through the apex
        {
            t1 = -b / a;
            if (t1 > 0.0f)
            {
                float s1 = m + t1 * k; // projection length of the line segment from the apex to intersection_t1 onto the coneDir
                if (s1 >= 0.0f && s1 <= coneHeight)
                {
                    return 1;
                }
            }
            return 0;
        }
    }
    
    float sqrtDiscr = sqrt(discriminant);
    float tt1 = (-b - sqrtDiscr) / a;
    float tt2 = (-b + sqrtDiscr) / a;

    /* Test s1 and s2 to see the positions of the intersecting points relative to the cylinder's two ends. */

    // s1 = coneDir.Dot(rayOrigin + tt1 * rayDir - coneApex), which expands into the following
    float s1 = m + tt1 * k; 
    // s2 = coneDir.Dot(rayOrigin + tt2 * rayDir - coneApex), which expands into the following
    float s2 = m + tt2 * k; 

    if (s1 < 0.0f)
    {
        if (s2 < 0.0f || s2 > coneHeight)
        {
            return 0;
        }
        else
        {
            if (tt2 >= 0.0f) // ray origin outside cone
            {
                t1 = tt2;
                t2 = (coneHeight - m) / k;
                return 2;
            }
            else if (m > coneHeight) // ray origin outside cone on the base side, the 
            {
                return 0;
            }
            else
            {
                t1 = (coneHeight - m) / k;
                return 1;
            }
        }
    }
    else if (s1 > coneHeight)
    {
        if (s2 < 0.0f || s2 > coneHeight )
        {
            return 0;
        }
        else
        {
            if (tt2 < 0.0f)
            {
                return 0;
            }
            else if (m >= coneHeight)
            {
                t1 = (coneHeight - m) / k;
                t2 = tt2;
                return 2;
            }
            else // ray origin inside cone
            {
                t1 = tt2;
                return 1;
            }
        }
    }
    else
    {
        if (s2 < 0.0f)
        {
            if (m >= coneHeight)
            {
                t1 = (coneHeight - m) / k;
                t2 = tt1;
                return 2;
            }
            else if (tt1 >= 0.0f) // ray origin inside cone
            {
                t1 = tt1;
                return 1;
            }
            else
            {
                return 0;
            }
        }
        else if (s2 > coneHeight)
        {
            if (tt1 >= 0.0f)
            {
                t1 = tt1;
                t2 = (coneHeight - m) / k;
                return 2;
            }
            else if (m <= coneHeight)
            {
                t1 = (coneHeight - m) / k;
                return 1;
            }
            else
            {
                return 0;
            }
        }
        else
        {
            if (tt1 >= 0.0f)
            {
                t1 = tt1;
                t2 = tt2;
                return 2;
            }
            else if (tt2 >= 0.0f)
            {
                t1 = tt2;
                return 1;
            }
            else
            {
                return 0;
            }
        }
    }
}

int AZ::Intersect::IntersectRayPlane(const Vector3& rayOrigin, const Vector3& rayDir, const Vector3& planePos, const Vector3& planeNormal, float& t)
{
    // (rayOrigin + t * rayDir - planePos).dot(planeNormal) = 0

    const float EPSILON = 0.00001f;

    float n = rayDir.Dot(planeNormal);
    if (fabsf(n) < EPSILON)
    {
        return 0;
    }

    t = planeNormal.Dot(planePos - rayOrigin) / n;
    if (t < 0.0f)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

int AZ::Intersect::IntersectRayQuad(
    const Vector3& rayOrigin, const Vector3& rayDir, const Vector3& vertexA,
    const Vector3& vertexB, const Vector3& vertexC, const Vector3& vertexD, float& t)
{
    const float EPSILON = 0.0001f;

    Vector3 AC = vertexC - vertexA;
    Vector3 AB = vertexB - vertexA;
    Vector3 QA = vertexA - rayOrigin;

    Vector3 triN = AB.Cross(AC); // the normal of the triangle ABC
    float dn = rayDir.Dot(triN);

    // Early-out if ray is facing away from ABC triangle
    if (dn * triN.Dot(QA) < 0)
    {
        return 0;
    }

    Vector3 E = rayDir.Cross(QA);
    float dnAbs = 0.0f;

    if (dn < -EPSILON) // vertices have counter-clock wise winding when looking at the quad from rayOrigin
    {
        dnAbs = -dn;
    }
    else if (dn > EPSILON)
    {
        E = -E;
        dnAbs = dn;
    }
    else // the ray is parallel to the quad plane
    {
        return 0;
    }

    // compute barycentric coordinates
    float v = E.Dot(AC);

    if (v >= 0.0f && v < dnAbs)
    {
        float w = -E.Dot(AB);
        if (w < 0.0f || v + w > dnAbs)
        {
            return 0;
        }
    }
    else if (v < 0.0f && v > -dnAbs)
    {
        Vector3 DA = vertexA - vertexD;
        float w = E.Dot(DA);
        if (w > 0.0f || v + w < -dnAbs) // v, w are negative
        {
            return 0;
        }
    }
    else
    {
        return 0;
    }

    t = triN.Dot(QA) / dn;
    return 1;
}

// reference: Real-Time Collision Detection, 5.3.3 Intersecting Ray or Segment Against Box
bool AZ::Intersect::IntersectRayBox(
    const Vector3& rayOrigin, const Vector3& rayDir, const Vector3& boxCenter, const Vector3& boxAxis1,
    const Vector3& boxAxis2, const Vector3& boxAxis3, float boxHalfExtent1, float boxHalfExtent2, float boxHalfExtent3, float& t)
{
    const float EPSILON = 0.00001f;

    float tmin = 0.0f; // the nearest to the ray origin
    float tmax = AZ::Constants::FloatMax; // the farthest from the ray origin

    Vector3 P = boxCenter - rayOrigin; // precomputed variable for calculating the vector from rayOrigin to a point on each box facet
    Vector3 QAp; // vector from rayOrigin to the center of the facet of boxAxis
    Vector3 QAn; // vector from rayOrigin to the center of the facet of -boxAxis
    float tp = 0.0f;
    float tn = 0.0f;
    bool isRayOriginInsideBox = true;
    
    /* Test the slab_1 formed by the planes with normals boxAxis1 and -boxAxis1. */

    Vector3 axis1 = boxHalfExtent1 * boxAxis1;

    QAp = P + axis1;
    tp = QAp.Dot(boxAxis1);

    QAn = P - axis1;
    tn = -QAn.Dot(boxAxis1);

    float n = rayDir.Dot(boxAxis1);
    if (fabsf(n) < EPSILON)
    {
        // If the ray is parallel to the slab and the ray origin is outside, return no intersection.
        if (tp < 0.0f || tn < 0.0f)
        {
            return false;
        }
    }
    else
    {
        if (tp < 0.0f || tn < 0.0f)
        {
            isRayOriginInsideBox = false;
        }

        float div = 1.0f / n;
        float t1 = tp * div;
        float t2 = tn * (-div);
        if (t1 > t2)
        {
            AZStd::swap(t1, t2);
        }
        tmin = AZ::GetMax(tmin, t1);
        tmax = AZ::GetMin(tmax, t2);
        if (tmin > tmax)
        {
            return false;
        }
    }

    /* test the slab_2 formed by plane with normals boxAxis2 and -boxAxis2 */

    Vector3 axis2 = boxHalfExtent2 * boxAxis2;

    QAp = P + axis2;
    tp = QAp.Dot(boxAxis2);

    QAn = P - axis2;
    tn = -QAn.Dot(boxAxis2);

    n = rayDir.Dot(boxAxis2);
    if (fabsf(n) < EPSILON)
    {
        // If the ray is parallel to the slab and the ray origin is outside, return no intersection.
        if (tp < 0.0f || tn < 0.0f)
        {
            return false;
        }
    }
    else
    {
        if (tp < 0.0f || tn < 0.0f)
        {
            isRayOriginInsideBox = false;
        }

        float div = 1.0f / n;
        float t1 = tp * div;
        float t2 = tn * (-div);
        if (t1 > t2)
        {
            AZStd::swap(t1, t2);
        }
        tmin = AZ::GetMax(tmin, t1);
        tmax = AZ::GetMin(tmax, t2);
        if (tmin > tmax)
        {
            return false;
        }
    }

    /* test the slab_3 formed by plane with normals boxAxis3 and -boxAxis3 */

    Vector3 axis3 = boxHalfExtent3 * boxAxis3;

    QAp = P + axis3;
    tp = QAp.Dot(boxAxis3);

    QAn = P - axis3;
    tn = -QAn.Dot(boxAxis3);

    n = rayDir.Dot(boxAxis3);
    if (fabsf(n) < EPSILON)
    {
        // If the ray is parallel to the slab and the ray origin is outside, return no intersection.
        if (tp < 0.0f || tn < 0.0f)
        {
            return false;
        }
    }
    else
    {
        if (tp < 0.0f || tn < 0.0f)
        {
            isRayOriginInsideBox = false;
        }

        float div = 1.0f / n;
        float t1 = tp * div;
        float t2 = tn * (-div);
        if (t1 > t2)
        {
            AZStd::swap(t1, t2);
        }
        tmin = AZ::GetMax(tmin, t1);
        tmax = AZ::GetMin(tmax, t2);
        if (tmin > tmax)
        {
            return false;
        }
    }

    t = (isRayOriginInsideBox ? tmax : tmin);
    return true;
}

bool AZ::Intersect::IntersectRayObb(const Vector3& rayOrigin, const Vector3& rayDir, const Obb& obb, float& t)
{
    return AZ::Intersect::IntersectRayBox(rayOrigin, rayDir, obb.GetPosition(),
        obb.GetAxisX(), obb.GetAxisY(), obb.GetAxisZ(),
        obb.GetHalfLengthX(), obb.GetHalfLengthY(), obb.GetHalfLengthZ(), t);
}

//=========================================================================
// IntersectSegmentCylinder
// [10/21/2009]
//=========================================================================
CylinderIsectTypes
AZ::Intersect::IntersectSegmentCylinder(
    const Vector3& sa, const Vector3& dir, const Vector3& p, const Vector3& q, const float r, float& t)
{
    const float epsilon = 0.001f;
    Vector3 d = q - p;  // can be cached
    Vector3 m = sa - p; // -"-
    Vector3 n = /*sb - sa*/ dir; // -"-

    float md = m.Dot(d);
    float nd = n.Dot(d);
    float dd = d.Dot(d);

    // Test if segment fully outside either endcap of cylinder
    if (md < 0.0f && md + nd < 0.0f)
    {
        return RR_ISECT_RAY_CYL_NONE;                                                    // Segment outside 'p' side of cylinder
    }
    if (md > dd && md + nd > dd)
    {
        return RR_ISECT_RAY_CYL_NONE;                                                          // Segment outside 'q' side of cylinder
    }
    float nn = n.Dot(n);
    float mn = m.Dot(n);
    float a = dd * nn - nd * nd;
    float k = m.Dot(m) - r * r;
    float c = dd * k - md * md;
    if (std::fabs(a) < epsilon)
    {
        // Segment runs parallel to cylinder axis
        if (c > 0.0f)
        {
            return RR_ISECT_RAY_CYL_NONE;                        // 'a' and thus the segment lie outside cylinder
        }
        // Now known that segment intersects cylinder; figure out how it intersects
        if (md < 0.0f)
        {
            t = -mn / nn; // Intersect segment against 'p' endcap
            return RR_ISECT_RAY_CYL_P_SIDE;
        }
        else if (md > dd)
        {
            t = (nd - mn) / nn; // Intersect segment against 'q' endcap
            return RR_ISECT_RAY_CYL_Q_SIDE;
        }
        else
        {
            // 'a' lies inside cylinder
            t = 0.0f;
            return RR_ISECT_RAY_CYL_SA_INSIDE;
        }
    }
    float b = dd * mn - nd * md;
    float discr = b * b - a * c;
    if (discr < 0.0f)
    {
        return RR_ISECT_RAY_CYL_NONE;                         // No real roots; no intersection
    }
    t = (-b - Sqrt(discr)) / a;
    CylinderIsectTypes result = RR_ISECT_RAY_CYL_PQ; // default along the PQ segment

    if (md + t * nd < 0.0f)
    {
        // Intersection outside cylinder on 'p' side
        if (nd <= 0.0f)
        {
            return RR_ISECT_RAY_CYL_NONE;                           // Segment pointing away from endcap
        }
        float t0 = -md / nd;
        // Keep intersection if Dot(S(t) - p, S(t) - p) <= r^2
        if (k + t0 * (2.0f * mn + t0 * nn) <= 0.0f)
        {
            //                  if( t0 < 0.0f ) t0 = 0.0f; // it's inside the cylinder
            t = t0;
            result = RR_ISECT_RAY_CYL_P_SIDE;
        }
        else
        {
            return RR_ISECT_RAY_CYL_NONE;
        }
    }
    else if (md + t * nd > dd)
    {
        // Intersection outside cylinder on 'q' side
        if (nd >= 0.0f)
        {
            return RR_ISECT_RAY_CYL_NONE;                              // Segment pointing away from endcap
        }
        float t0 = (dd - md) / nd;
        // Keep intersection if Dot(S(t) - q, S(t) - q) <= r^2
        if (k + dd - 2.0f * md + t0 * (2.0f * (mn - nd) + t0 * nn) <= 0.0f)
        {
            //                  if( t0 < 0.0f ) t0 = 0.0f; // it's inside the cylinder
            t = t0;
            result = RR_ISECT_RAY_CYL_Q_SIDE;
        }
        else
        {
            return RR_ISECT_RAY_CYL_NONE;
        }
    }

    // Segment intersects cylinder between the end-caps; t is correct
    if (t > 1.0f)
    {
        return RR_ISECT_RAY_CYL_NONE; // Intersection lies outside segment
    }
    else if (t < 0.0f)
    {
        if (c <= 0.0f)
        {
            t = 0.0f;
            return RR_ISECT_RAY_CYL_SA_INSIDE; // Segment starts inside
        }
        else
        {
            return RR_ISECT_RAY_CYL_NONE; // Intersection lies outside segment
        }
    }
    else
    {
        return result;
    }
}
//=========================================================================
// IntersectSegmentCapsule
// [10/21/2009]
//=========================================================================
CapsuleIsectTypes
AZ::Intersect::IntersectSegmentCapsule(const Vector3& sa, const Vector3& dir, const Vector3& p, const Vector3& q, const float r, float& t)
{
    int result = IntersectSegmentCylinder(sa, dir, p, q, r, t);

    if (result == RR_ISECT_RAY_CYL_SA_INSIDE)
    {
        return ISECT_RAY_CAPSULE_SA_INSIDE;
    }

    if (result == RR_ISECT_RAY_CYL_PQ)
    {
        return ISECT_RAY_CAPSULE_PQ;
    }

    Vector3 dirNorm = dir;
    float len = dirNorm.NormalizeWithLength();

    // check spheres
    float timeLenTop, timeLenBottom;
    int resultTop = IntersectRaySphere(sa, dirNorm, p, r, timeLenTop);
    if (resultTop == ISECT_RAY_SPHERE_SA_INSIDE)
    {
        return ISECT_RAY_CAPSULE_SA_INSIDE;
    }
    int resultBottom = IntersectRaySphere(sa, dirNorm, q, r, timeLenBottom);
    if (resultBottom == ISECT_RAY_SPHERE_SA_INSIDE)
    {
        return ISECT_RAY_CAPSULE_SA_INSIDE;
    }

    if (resultTop == ISECT_RAY_SPHERE_ISECT)
    {
        if (resultBottom == ISECT_RAY_SPHERE_ISECT)
        {
            // if we intersect both spheres pick the closest one
            if (timeLenTop < timeLenBottom)
            {
                t = timeLenTop / len;
                return ISECT_RAY_CAPSULE_P_SIDE;
            }
            else
            {
                t = timeLenBottom / len;
                return ISECT_RAY_CAPSULE_Q_SIDE;
            }
        }
        else
        {
            t = timeLenTop / len;
            return ISECT_RAY_CAPSULE_P_SIDE;
        }
    }

    if (resultBottom == ISECT_RAY_SPHERE_ISECT)
    {
        t = timeLenBottom / len;
        return ISECT_RAY_CAPSULE_Q_SIDE;
    }

    return ISECT_RAY_CAPSULE_NONE;
}

//=========================================================================
// IntersectSegmentPolyhedron
// [10/21/2009]
//=========================================================================
bool
AZ::Intersect::IntersectSegmentPolyhedron(
    const Vector3& sa, const Vector3& dir, const Plane p[], int numPlanes,
    float& tfirst, float& tlast, int& iFirstPlane, int& iLastPlane)
{
    // Compute direction vector for the segment
    Vector3 d = /*b - a*/ dir;
    // Set initial interval to being the whole segment. For a ray, tlast should be
    // set to +RR_FLT_MAX. For a line, additionally tfirst should be set to -RR_FLT_MAX
    tfirst = 0.0f;
    tlast = 1.0f;
    iFirstPlane = -1;
    iLastPlane = -1;
    // Intersect segment against each plane
    for (int i = 0; i < numPlanes; i++)
    {
        const Vector4& plane = p[i].GetPlaneEquationCoefficients();

        float denom = plane.Dot3(d);
        // don't forget we store -D in the plane
        float dist = (-plane.GetW()) - plane.Dot3(sa);
        // Test if segment runs parallel to the plane
        if (denom == 0.0f)
        {
            // If so, return "no intersection" if segment lies outside plane
            if (dist < 0.0f)
            {
                return false;
            }
        }
        else
        {
            // Compute parameterized t value for intersection with current plane
            float t = dist / denom;
            if (denom < 0.0f)
            {
                // When entering half space, update tfirst if t is larger
                if (t > tfirst)
                {
                    tfirst = t;
                    iFirstPlane = i;
                }
            }
            else
            {
                // When exiting half space, update tlast if t is smaller
                if (t < tlast)
                {
                    tlast = t;
                    iLastPlane = i;
                }
            }

            // Exit with "no intersection" if intersection becomes empty
            if (tfirst > tlast)
            {
                return false;
            }
        }
    }

    //DBG_Assert(iFirstPlane!=-1&&iLastPlane!=-1,("We have some bad border case to have only one plane, fix this function!"));
    if (iFirstPlane == -1 && iLastPlane == -1)
    {
        return false;
    }

    // A nonzero logical intersection, so the segment intersects the polyhedron
    return true;
}

//=========================================================================
// ClosestSegmentSegment
// [10/21/2009]
//=========================================================================
void
AZ::Intersect::ClosestSegmentSegment(
    const Vector3& segment1Start, const Vector3& segment1End,
    const Vector3& segment2Start, const Vector3& segment2End,
    float& segment1Proportion, float& segment2Proportion, 
    Vector3& closestPointSegment1, Vector3& closestPointSegment2,
    float epsilon)
{
    const Vector3 segment1 = segment1End - segment1Start;
    const Vector3 segment2 = segment2End - segment2Start;
    const Vector3 segmentStartsVector = segment1Start - segment2Start;
    const float segment1LengthSquared = segment1.Dot(segment1);
    const float segment2LengthSquared = segment2.Dot(segment2);

    // Check if both segments degenerate into points
    if (segment1LengthSquared <= epsilon && segment2LengthSquared <= epsilon)
    {
        segment1Proportion = 0.0f;
        segment2Proportion = 0.0f;
        closestPointSegment1 = segment1Start;
        closestPointSegment2 = segment2Start;
        return;
    }

    float projSegment2SegmentStarts = segment2.Dot(segmentStartsVector);

    // Check if segment 1 degenerates into a point
    if (segment1LengthSquared <= epsilon)
    {
        segment1Proportion = 0.0f;
        segment2Proportion = AZ::GetClamp(projSegment2SegmentStarts / segment2LengthSquared, 0.0f, 1.0f);
    }
    else
    {
        float projSegment1SegmentStarts = segment1.Dot(segmentStartsVector);
        // Check if segment 2 degenerates into a point
        if (segment2LengthSquared <= epsilon)
        {
            segment1Proportion = AZ::GetClamp(-projSegment1SegmentStarts / segment1LengthSquared, 0.0f, 1.0f);
            segment2Proportion = 0.0f;
        }
        else
        {
            // The general non-degenerate case starts here
            float projSegment1Segment2 = segment1.Dot(segment2);
            float denom = segment1LengthSquared * segment2LengthSquared - projSegment1Segment2 * projSegment1Segment2; // Always nonnegative

            // If segments not parallel, compute closest point on segment1 to segment2, and
            // clamp to segment1. Else pick arbitrary segment1Proportion (here 0)
            if (denom != 0.0f)
            {
                segment1Proportion = AZ::GetClamp((projSegment1Segment2 * projSegment2SegmentStarts - projSegment1SegmentStarts * segment2LengthSquared) / denom, 0.0f, 1.0f);
            }
            else
            {
                segment1Proportion = 0.0f;
            }

            // Compute point on segment2 closest to segment1 using
            segment2Proportion = (projSegment1Segment2 * segment1Proportion + projSegment2SegmentStarts) / segment2LengthSquared;

            // If segment2Proportion in [0,1] done. Else clamp segment2Proportion, recompute segment1Proportion for the new value of segment2Proportion
            // and clamp segment1Proportion to [0, 1]
            if (segment2Proportion < 0.0f)
            {
                segment2Proportion = 0.0f;
                segment1Proportion = AZ::GetClamp(-projSegment1SegmentStarts / segment1LengthSquared, 0.0f, 1.0f);
            }
            else if (segment2Proportion > 1.0f)
            {
                segment2Proportion = 1.0f;
                segment1Proportion = AZ::GetClamp((projSegment1Segment2 - projSegment1SegmentStarts) / segment1LengthSquared, 0.0f, 1.0f);
            }
        }
    }

    closestPointSegment1 = segment1Start + segment1 * segment1Proportion;
    closestPointSegment2 = segment2Start + segment2 * segment2Proportion;
}

void AZ::Intersect::ClosestSegmentSegment(
    const Vector3& segment1Start, const Vector3& segment1End,
    const Vector3& segment2Start, const Vector3& segment2End,
    Vector3& closestPointSegment1, Vector3& closestPointSegment2,
    float epsilon)
{
    float proportion1, proportion2;
    AZ::Intersect::ClosestSegmentSegment(
        segment1Start, segment1End,
        segment2Start, segment2End,
        proportion1, proportion2,
        closestPointSegment1, closestPointSegment2, epsilon);
}

void AZ::Intersect::ClosestPointSegment(
    const Vector3& point, const Vector3& segmentStart, const Vector3& segmentEnd,
    float& proportion, Vector3& closestPointOnSegment)
{
    Vector3 segment = segmentEnd - segmentStart;
    // Project point onto segment, but deferring divide by segment.Dot(segment)
    proportion = (point - segmentStart).Dot(segment);
    if (proportion <= 0.0f)
    {
        // Point projects outside the [segmentStart, segmentEnd] interval, on the segmentStart side, clamp to segmentStart
        proportion = 0.0f;
        closestPointOnSegment = segmentStart;
    }
    else
    {
        float segmentLengthSquared = segment.Dot(segment);
        if (proportion >= segmentLengthSquared)
        {
            // Point projects outside the [segmentStart, segmentEnd] interval, on the segmentEnd side, clamp to segmentEnd
            proportion = 1.0f;
            closestPointOnSegment = segmentEnd;
        }
        else
        {
            // Point projects inside the [segmentStart, segmentEnd] interval, must do deferred divide now
            proportion = proportion / segmentLengthSquared;
            closestPointOnSegment = segmentStart + (proportion * segment);
        }
    }
}

#if 0
//////////////////////////////////////////////////////////////////////////
// TEST AABB/RAY TEST using slopes
namespace test
{
    enum CLASSIFICATION
    {
        MMM, MMP, MPM, MPP, PMM, PMP, PPM, PPP, POO, MOO, OPO, OMO, OOP, OOM,
        OMM, OMP, OPM, OPP, MOM, MOP, POM, POP, MMO, MPO, PMO, PPO
    };

    struct ray
    {
        //common variables
        float x, y, z;      // ray origin
        float i, j, k;      // ray direction
        float ii, ij, ik;   // inverses of direction components

        // ray slope
        int classification;
        float ibyj, jbyi, kbyj, jbyk, ibyk, kbyi; //slope
        float c_xy, c_xz, c_yx, c_yz, c_zx, c_zy;
    };

    struct aabox
    {
        float x0, y0, z0, x1, y1, z1;
    };

    void make_ray(float x, float y, float z, float i, float j, float k, ray* r)
    {
        //common variables
        r->x = x;
        r->y = y;
        r->z = z;
        r->i = i;
        r->j = j;
        r->k = k;

        r->ii = 1.0f / i;
        r->ij = 1.0f / j;
        r->ik = 1.0f / k;

        //ray slope
        r->ibyj = r->i * r->ij;
        r->jbyi = r->j * r->ii;
        r->jbyk = r->j * r->ik;
        r->kbyj = r->k * r->ij;
        r->ibyk = r->i * r->ik;
        r->kbyi = r->k * r->ii;
        r->c_xy = r->y - r->jbyi * r->x;
        r->c_xz = r->z - r->kbyi * r->x;
        r->c_yx = r->x - r->ibyj * r->y;
        r->c_yz = r->z - r->kbyj * r->y;
        r->c_zx = r->x - r->ibyk * r->z;
        r->c_zy = r->y - r->jbyk * r->z;

        //ray slope classification
        if (i < 0)
        {
            if (j < 0)
            {
                if (k < 0)
                {
                    r->classification = MMM;
                }
                else if (k > 0)
                {
                    r->classification = MMP;
                }
                else//(k >= 0)
                {
                    r->classification = MMO;
                }
            }
            else//(j >= 0)
            {
                if (k < 0)
                {
                    r->classification = MPM;
                    if (j == 0)
                    {
                        r->classification = MOM;
                    }
                }
                else//(k >= 0)
                {
                    if ((j == 0) && (k == 0))
                    {
                        r->classification = MOO;
                    }
                    else if (k == 0)
                    {
                        r->classification = MPO;
                    }
                    else if (j == 0)
                    {
                        r->classification = MOP;
                    }
                    else
                    {
                        r->classification = MPP;
                    }
                }
            }
        }
        else//(i >= 0)
        {
            if (j < 0)
            {
                if (k < 0)
                {
                    r->classification = PMM;
                    if (i == 0)
                    {
                        r->classification = OMM;
                    }
                }
                else//(k >= 0)
                {
                    if ((i == 0) && (k == 0))
                    {
                        r->classification = OMO;
                    }
                    else if (k == 0)
                    {
                        r->classification = PMO;
                    }
                    else if (i == 0)
                    {
                        r->classification = OMP;
                    }
                    else
                    {
                        r->classification = PMP;
                    }
                }
            }
            else//(j >= 0)
            {
                if (k < 0)
                {
                    if ((i == 0) && (j == 0))
                    {
                        r->classification = OOM;
                    }
                    else if (i == 0)
                    {
                        r->classification = OPM;
                    }
                    else if (j == 0)
                    {
                        r->classification = POM;
                    }
                    else
                    {
                        r->classification = PPM;
                    }
                }
                else//(k > 0)
                {
                    if (i == 0)
                    {
                        if (j == 0)
                        {
                            r->classification = OOP;
                        }
                        else if (k == 0)
                        {
                            r->classification = OPO;
                        }
                        else
                        {
                            r->classification = OPP;
                        }
                    }
                    else
                    {
                        if ((j == 0) && (k == 0))
                        {
                            r->classification = POO;
                        }
                        else if (j == 0)
                        {
                            r->classification = POP;
                        }
                        else if (k == 0)
                        {
                            r->classification = PPO;
                        }
                        else
                        {
                            r->classification = PPP;
                        }
                    }
                }
            }
        }
    }

    bool slope(ray* r, aabox* b)
    {
        switch (r->classification)
        {
        case MMM:

            if ((r->x < b->x0) || (r->y < b->y0) || (r->z < b->z0)
                || (r->jbyi * b->x0 - b->y1 + r->c_xy > 0)
                || (r->ibyj * b->y0 - b->x1 + r->c_yx > 0)
                || (r->jbyk * b->z0 - b->y1 + r->c_zy > 0)
                || (r->kbyj * b->y0 - b->z1 + r->c_yz > 0)
                || (r->kbyi * b->x0 - b->z1 + r->c_xz > 0)
                || (r->ibyk * b->z0 - b->x1 + r->c_zx > 0)
                )
            {
                return false;
            }

            return true;

        case MMP:

            if ((r->x < b->x0) || (r->y < b->y0) || (r->z > b->z1)
                || (r->jbyi * b->x0 - b->y1 + r->c_xy > 0)
                || (r->ibyj * b->y0 - b->x1 + r->c_yx > 0)
                || (r->jbyk * b->z1 - b->y1 + r->c_zy > 0)
                || (r->kbyj * b->y0 - b->z0 + r->c_yz < 0)
                || (r->kbyi * b->x0 - b->z0 + r->c_xz < 0)
                || (r->ibyk * b->z1 - b->x1 + r->c_zx > 0)
                )
            {
                return false;
            }

            return true;

        case MPM:

            if ((r->x < b->x0) || (r->y > b->y1) || (r->z < b->z0)
                || (r->jbyi * b->x0 - b->y0 + r->c_xy < 0)
                || (r->ibyj * b->y1 - b->x1 + r->c_yx > 0)
                || (r->jbyk * b->z0 - b->y0 + r->c_zy < 0)
                || (r->kbyj * b->y1 - b->z1 + r->c_yz > 0)
                || (r->kbyi * b->x0 - b->z1 + r->c_xz > 0)
                || (r->ibyk * b->z0 - b->x1 + r->c_zx > 0)
                )
            {
                return false;
            }

            return true;

        case MPP:

            if ((r->x < b->x0) || (r->y > b->y1) || (r->z > b->z1)
                || (r->jbyi * b->x0 - b->y0 + r->c_xy < 0)
                || (r->ibyj * b->y1 - b->x1 + r->c_yx > 0)
                || (r->jbyk * b->z1 - b->y0 + r->c_zy < 0)
                || (r->kbyj * b->y1 - b->z0 + r->c_yz < 0)
                || (r->kbyi * b->x0 - b->z0 + r->c_xz < 0)
                || (r->ibyk * b->z1 - b->x1 + r->c_zx > 0)
                )
            {
                return false;
            }

            return true;

        case PMM:

            if ((r->x > b->x1) || (r->y < b->y0) || (r->z < b->z0)
                || (r->jbyi * b->x1 - b->y1 + r->c_xy > 0)
                || (r->ibyj * b->y0 - b->x0 + r->c_yx < 0)
                || (r->jbyk * b->z0 - b->y1 + r->c_zy > 0)
                || (r->kbyj * b->y0 - b->z1 + r->c_yz > 0)
                || (r->kbyi * b->x1 - b->z1 + r->c_xz > 0)
                || (r->ibyk * b->z0 - b->x0 + r->c_zx < 0)
                )
            {
                return false;
            }

            return true;

        case PMP:

            if ((r->x > b->x1) || (r->y < b->y0) || (r->z > b->z1)
                || (r->jbyi * b->x1 - b->y1 + r->c_xy > 0)
                || (r->ibyj * b->y0 - b->x0 + r->c_yx < 0)
                || (r->jbyk * b->z1 - b->y1 + r->c_zy > 0)
                || (r->kbyj * b->y0 - b->z0 + r->c_yz < 0)
                || (r->kbyi * b->x1 - b->z0 + r->c_xz < 0)
                || (r->ibyk * b->z1 - b->x0 + r->c_zx < 0)
                )
            {
                return false;
            }

            return true;

        case PPM:

            if ((r->x > b->x1) || (r->y > b->y1) || (r->z < b->z0)
                || (r->jbyi * b->x1 - b->y0 + r->c_xy < 0)
                || (r->ibyj * b->y1 - b->x0 + r->c_yx < 0)
                || (r->jbyk * b->z0 - b->y0 + r->c_zy < 0)
                || (r->kbyj * b->y1 - b->z1 + r->c_yz > 0)
                || (r->kbyi * b->x1 - b->z1 + r->c_xz > 0)
                || (r->ibyk * b->z0 - b->x0 + r->c_zx < 0)
                )
            {
                return false;
            }

            return true;

        case PPP:

            if ((r->x > b->x1) || (r->y > b->y1) || (r->z > b->z1)
                || (r->jbyi * b->x1 - b->y0 + r->c_xy < 0)
                || (r->ibyj * b->y1 - b->x0 + r->c_yx < 0)
                || (r->jbyk * b->z1 - b->y0 + r->c_zy < 0)
                || (r->kbyj * b->y1 - b->z0 + r->c_yz < 0)
                || (r->kbyi * b->x1 - b->z0 + r->c_xz < 0)
                || (r->ibyk * b->z1 - b->x0 + r->c_zx < 0)
                )
            {
                return false;
            }

            return true;

        case OMM:

            if ((r->x < b->x0) || (r->x > b->x1)
                || (r->y < b->y0) || (r->z < b->z0)
                || (r->jbyk * b->z0 - b->y1 + r->c_zy > 0)
                || (r->kbyj * b->y0 - b->z1 + r->c_yz > 0)
                )
            {
                return false;
            }

            return true;

        case OMP:

            if ((r->x < b->x0) || (r->x > b->x1)
                || (r->y < b->y0) || (r->z > b->z1)
                || (r->jbyk * b->z1 - b->y1 + r->c_zy > 0)
                || (r->kbyj * b->y0 - b->z0 + r->c_yz < 0)
                )
            {
                return false;
            }

            return true;

        case OPM:

            if ((r->x < b->x0) || (r->x > b->x1)
                || (r->y > b->y1) || (r->z < b->z0)
                || (r->jbyk * b->z0 - b->y0 + r->c_zy < 0)
                || (r->kbyj * b->y1 - b->z1 + r->c_yz > 0)
                )
            {
                return false;
            }

            return true;

        case OPP:

            if ((r->x < b->x0) || (r->x > b->x1)
                || (r->y > b->y1) || (r->z > b->z1)
                || (r->jbyk * b->z1 - b->y0 + r->c_zy < 0)
                || (r->kbyj * b->y1 - b->z0 + r->c_yz < 0)
                )
            {
                return false;
            }

            return true;

        case MOM:

            if ((r->y < b->y0) || (r->y > b->y1)
                || (r->x < b->x0) || (r->z < b->z0)
                || (r->kbyi * b->x0 - b->z1 + r->c_xz > 0)
                || (r->ibyk * b->z0 - b->x1 + r->c_zx > 0)
                )
            {
                return false;
            }

            return true;

        case MOP:

            if ((r->y < b->y0) || (r->y > b->y1)
                || (r->x < b->x0) || (r->z > b->z1)
                || (r->kbyi * b->x0 - b->z0 + r->c_xz < 0)
                || (r->ibyk * b->z1 - b->x1 + r->c_zx > 0)
                )
            {
                return false;
            }

            return true;

        case POM:

            if ((r->y < b->y0) || (r->y > b->y1)
                || (r->x > b->x1) || (r->z < b->z0)
                || (r->kbyi * b->x1 - b->z1 + r->c_xz > 0)
                || (r->ibyk * b->z0 - b->x0 + r->c_zx < 0)
                )
            {
                return false;
            }

            return true;

        case POP:

            if ((r->y < b->y0) || (r->y > b->y1)
                || (r->x > b->x1) || (r->z > b->z1)
                || (r->kbyi * b->x1 - b->z0 + r->c_xz < 0)
                || (r->ibyk * b->z1 - b->x0 + r->c_zx < 0)
                )
            {
                return false;
            }

            return true;

        case MMO:

            if ((r->z < b->z0) || (r->z > b->z1)
                || (r->x < b->x0) || (r->y < b->y0)
                || (r->jbyi * b->x0 - b->y1 + r->c_xy > 0)
                || (r->ibyj * b->y0 - b->x1 + r->c_yx > 0)
                )
            {
                return false;
            }

            return true;

        case MPO:

            if ((r->z < b->z0) || (r->z > b->z1)
                || (r->x < b->x0) || (r->y > b->y1)
                || (r->jbyi * b->x0 - b->y0 + r->c_xy < 0)
                || (r->ibyj * b->y1 - b->x1 + r->c_yx > 0)
                )
            {
                return false;
            }

            return true;

        case PMO:

            if ((r->z < b->z0) || (r->z > b->z1)
                || (r->x > b->x1) || (r->y < b->y0)
                || (r->jbyi * b->x1 - b->y1 + r->c_xy > 0)
                || (r->ibyj * b->y0 - b->x0 + r->c_yx < 0)
                )
            {
                return false;
            }

            return true;

        case PPO:

            if ((r->z < b->z0) || (r->z > b->z1)
                || (r->x > b->x1) || (r->y > b->y1)
                || (r->jbyi * b->x1 - b->y0 + r->c_xy < 0)
                || (r->ibyj * b->y1 - b->x0 + r->c_yx < 0)
                )
            {
                return false;
            }

            return true;

        case MOO:

            if ((r->x < b->x0)
                || (r->y < b->y0) || (r->y > b->y1)
                || (r->z < b->z0) || (r->z > b->z1)
                )
            {
                return false;
            }

            return true;

        case POO:

            if ((r->x > b->x1)
                || (r->y < b->y0) || (r->y > b->y1)
                || (r->z < b->z0) || (r->z > b->z1)
                )
            {
                return false;
            }

            return true;

        case OMO:

            if ((r->y < b->y0)
                || (r->x < b->x0) || (r->x > b->x1)
                || (r->z < b->z0) || (r->z > b->z1)
                )
            {
                return false;
            }

        case OPO:

            if ((r->y > b->y1)
                || (r->x < b->x0) || (r->x > b->x1)
                || (r->z < b->z0) || (r->z > b->z1)
                )
            {
                return false;
            }

        case OOM:

            if ((r->z < b->z0)
                || (r->x < b->x0) || (r->x > b->x1)
                || (r->y < b->y0) || (r->y > b->y1)
                )
            {
                return false;
            }

        case OOP:

            if ((r->z > b->z1)
                || (r->x < b->x0) || (r->x > b->x1)
                || (r->y < b->y0) || (r->y > b->y1)
                )
            {
                return false;
            }

            return true;
        }

        return false;
    }
}
//////////////////////////////////////////////////////////////////////////
#endif
