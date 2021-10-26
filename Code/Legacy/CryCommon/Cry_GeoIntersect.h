/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Common intersection-tests
#pragma once

#include <Cry_Geo.h>

namespace Intersect {
    inline bool Ray_Plane(const Ray& ray, const Plane_tpl<f32>& plane, Vec3& output, bool bSingleSidePlane = true)
    {
        float cosine = plane.n | ray.direction;

        //REJECTION 1: if "line-direction" is perpendicular to "plane-normal", an intersection is not possible! That means ray is parallel
        //             to the plane
        //REJECTION 2: if bSingleSidePlane == true we deal with single-sided planes. That means
        //             if "line-direction" is pointing in the same direction as "the plane-normal",
        //             an intersection is not possible!
        if ((cosine == 0.0f) ||                                     // normal is orthogonal to vector, cant intersect
            (bSingleSidePlane && (cosine > 0.0f)))  // we are trying to find an intersection in the same direction as the plane normal
        {
            return false;
        }

        float numer = plane.DistFromPlane(ray.origin);
        float fLength = -numer / cosine;
        output = ray.origin + (ray.direction * fLength);
        //skip, if cutting-point is "behind" ray.origin
        if (fLength < 0.0f)
        {
            return false;
        }

        return true;        //intersection occurred
    }

    /*
    * calculates intersection between a ray and a triangle.
    * IMPORTANT: this is a single-sided intersection test. That means its not sufficient
    * that the triangle and rayt overlap, its also important that the triangle
    * is "visible" when you from the origin along the ray-direction.
    *
    * If you need a double-sided test, you'll have to call this function twice with
    * reversed order of triangle vertices.
    *
    * return values
    * if there is an intertection the functions return "true" and stores the
    * 3d-intersection point in "output". if the function returns "false" the value in
    * "output" is undefined
    */
    inline bool Ray_Triangle(const Ray& ray, const Vec3& v0, const Vec3& v1, const Vec3& v2, Vec3& output)
    {
        const float Epsilon = 0.0000001f;

        Vec3 edgeA = v1 - v0;
        Vec3 edgeB = v2 - v0;

        Vec3 dir = ray.direction;

        Vec3 p = dir.Cross(edgeA);
        Vec3 t = ray.origin - v0;
        Vec3 q = t.Cross(edgeB);

        float dot = edgeB.Dot(p);

        float u = t.Dot(p);
        float v = dir.Dot(q);

        float DotGreaterThanEpsilon = dot - Epsilon;
        float VGreaterEqualThanZero = v;
        float UGreaterEqualThanZero = u;
        float UVLessThanDot = dot - (u + v);
        float ULessThanDot = dot - u;

        float UVGreaterEqualThanZero = (float)fsel(VGreaterEqualThanZero, UGreaterEqualThanZero, VGreaterEqualThanZero);
        float UUVLessThanDot = (float)fsel(UVLessThanDot, ULessThanDot, UVLessThanDot);
        float BothGood = (float)fsel(UVGreaterEqualThanZero, UUVLessThanDot, UVGreaterEqualThanZero);
        float AllGood = (float)fsel(DotGreaterThanEpsilon, BothGood, DotGreaterThanEpsilon);

        if (AllGood < 0.0f)
        {
            return false;
        }

        float dt = edgeA.Dot(q) / dot;

        Vec3 result = (dir * dt) + ray.origin;
        output = result;

        float AfterStart = (result - ray.origin).Dot(dir);

        return AfterStart >= 0.0f;
    }

    //----------------------------------------------------------------------------------
    //  Ray_AABB
    //
    //  just ONE intersection point is calculated, and thats the entry point           -
    //   Lineseg and AABB are assumed to be in the same space
    //
    //--- 0x00 = no intersection (output undefined)        --------------------------
    //--- 0x01 = intersection (intersection point in output)              --------------
    //--- 0x02 = start of Lineseg is inside the AABB (ls.start is output)
    //----------------------------------------------------------------------------------
    inline uint8 Ray_AABB(const Ray& ray, const AABB& aabb, Vec3& output1)
    {
        uint8 cflags;
        float cosine;
        Vec3 cut;
        //--------------------------------------------------------------------------------------
        //----         check if "ray.origin" is inside of AABB    ---------------------------
        //--------------------------------------------------------------------------------------
        cflags = (ray.origin.x >= aabb.min.x) << 0;
        cflags |= (ray.origin.x <= aabb.max.x) << 1;
        cflags |= (ray.origin.y >= aabb.min.y) << 2;
        cflags |= (ray.origin.y <= aabb.max.y) << 3;
        cflags |= (ray.origin.z >= aabb.min.z) << 4;
        cflags |= (ray.origin.z <= aabb.max.z) << 5;
        if (cflags == 0x3f)
        {
            output1 = ray.origin;
            return 0x02;
        }

        //--------------------------------------------------------------------------------------
        //----         check intersection with planes           ------------------------------
        //--------------------------------------------------------------------------------------
        for (int i = 0; i < 3; i++)
        {
            if ((ray.direction[i] > 0) && (ray.origin[i] < aabb.min[i]))
            {
                cosine = (-ray.origin[i] + aabb.min[i]) / ray.direction[i];
                cut[i] = aabb.min[i];
                cut[incm3(i)] = ray.origin[incm3(i)] + (ray.direction[incm3(i)] * cosine);
                cut[decm3(i)] = ray.origin[decm3(i)] + (ray.direction[decm3(i)] * cosine);
                if ((cut[incm3(i)] > aabb.min[incm3(i)]) && (cut[incm3(i)] < aabb.max[incm3(i)]) && (cut[decm3(i)] > aabb.min[decm3(i)]) && (cut[decm3(i)] < aabb.max[decm3(i)]))
                {
                    output1 = cut;
                    return 0x01;
                }
            }
            if ((ray.direction[i] < 0) && (ray.origin[i] > aabb.max[i]))
            {
                cosine = (+ray.origin[i] - aabb.max[i]) / ray.direction[i];
                cut[i] = aabb.max[i];
                cut[incm3(i)] = ray.origin[incm3(i)] - (ray.direction[incm3(i)] * cosine);
                cut[decm3(i)] = ray.origin[decm3(i)] - (ray.direction[decm3(i)] * cosine);
                if ((cut[incm3(i)] > aabb.min[incm3(i)]) && (cut[incm3(i)] < aabb.max[incm3(i)]) && (cut[decm3(i)] > aabb.min[decm3(i)]) && (cut[decm3(i)] < aabb.max[decm3(i)]))
                {
                    output1 = cut;
                    return 0x01;
                }
            }
        }
        return 0x00;//no intersection
    }

    //----------------------------------------------------------------------------------
    //--- 0x00 = no intersection                               --------------------------
    //--- 0x01 = not possible   --
    //--- 0x02 = one intersection, lineseg has just an EXIT point but no ENTRY point (ls.start is inside the sphere)  --
    //--- 0x03 = two intersection, lineseg has ENTRY and EXIT point  --
    //----------------------------------------------------------------------------------

    inline unsigned char Ray_Sphere(const Ray& ray, const ::Sphere& s, Vec3& i0, Vec3& i1)
    {
        Vec3 end = ray.origin + ray.direction;
        float a = ray.direction | ray.direction;
        float b = (ray.direction | (ray.origin - s.center)) * 2.0f;
        float c = ((ray.origin - s.center) | (ray.origin - s.center)) - (s.radius * s.radius);

        float desc = (b * b) - (4 * a * c);

        unsigned char intersection = 0;
        if (desc >= 0.0f)
        {
            float lamba0 = (-b - sqrt_tpl(desc)) / (2.0f * a);
            //  _stprintf(d3dApp.token,"lamba0: %20.12f",lamba0);
            //  d3dApp.m_pFont->DrawText( 2, d3dApp.PrintY, D3DCOLOR_ARGB(255,255,255,0), d3dApp.token );   d3dApp.PrintY+=20;
            if (lamba0 > 0.0f)
            {
                i0 = ray.origin + ((end - ray.origin) * lamba0);
                intersection = 1;
            }

            float lamba1 = (-b + sqrt_tpl(desc)) / (2.0f * a);
            //  _stprintf(d3dApp.token,"lamba1: %20.12f",lamba1);
            //  d3dApp.m_pFont->DrawText( 2, d3dApp.PrintY, D3DCOLOR_ARGB(255,255,255,0), d3dApp.token );   d3dApp.PrintY+=20;
            if (lamba1 > 0.0f)
            {
                i1 = ray.origin + ((end - ray.origin) * lamba1);
                intersection |= 2;
            }
        }
        return intersection;
    }

    inline bool Ray_SphereFirst(const Ray& ray, const ::Sphere& s, Vec3& intPoint)
    {
        Vec3 p2;
        unsigned char res = Ray_Sphere(ray, s, intPoint, p2);
        if (res == 2)
        {
            intPoint = p2;
        }
        if (res > 1)
        {
            return true;
        }
        return false;
    }
} //Intersect
