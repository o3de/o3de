/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Common structures for geometry computations
#pragma once

#include "Cry_Math.h"
///////////////////////////////////////////////////////////////////////////////
// Forward declarations                                                      //
///////////////////////////////////////////////////////////////////////////////

struct Ray;
template <typename F>
struct Lineseg_tpl;
template <typename F>
struct Triangle_tpl;

struct AABB;
template <typename F>
struct OBB_tpl;

//-----------------------------------------------------


///////////////////////////////////////////////////////////////////////////////
// Definitions                                                               //
///////////////////////////////////////////////////////////////////////////////

struct PosNorm
{
    Vec3    vPos;
    Vec3    vNorm;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// struct Ray
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
struct Ray
{
    Vec3 origin;
    Vec3 direction;

    //default Ray constructor (without initialisation)
    inline Ray(void) {}
    inline Ray(const Vec3& o, const Vec3& d) {  origin = o; direction = d; }
    inline void operator () (const Vec3& o, const Vec3& d) {  origin = o; direction = d; }

    ~Ray(void) {};
};


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// struct Lineseg
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
struct Lineseg
{
    Vec3_tpl<float> start;
    Vec3_tpl<float> end;

    //default Lineseg constructor (without initialisation)
    inline Lineseg(void) {}
    inline Lineseg(const Vec3_tpl<float>& s, const Vec3_tpl<float>& e) {  start = s; end = e; }

    ~Lineseg(void) {};
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// struct Triangle
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
template <typename F>
struct Triangle_tpl
{
    Vec3_tpl<F> v0, v1, v2;

    //default Lineseg constructor (without initialisation)
    ILINE Triangle_tpl(void) {}
    ILINE Triangle_tpl(const Vec3_tpl<F>& a, const Vec3_tpl<F>& b, const Vec3_tpl<F>& c) {  v0 = a; v1 = b; v2 = c; }
    ILINE void operator () (const Vec3_tpl<F>& a, const Vec3_tpl<F>& b, const Vec3_tpl<F>& c) { v0 = a; v1 = b; v2 = c; }

    ~Triangle_tpl(void) {};

    Vec3_tpl<F> GetNormal() const
    {
        return ((v1 - v0) ^ (v2 - v0)).GetNormalized();
    }

    F GetArea() const
    {
        return 0.5f * (v1 - v0).Cross(v2 - v0).GetLength();
    }
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// struct AABB
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
struct AABB
{
    Vec3 min;
    Vec3 max;

    /// default AABB constructor (without initialisation)
    ILINE  AABB()
    {}
    enum type_reset
    {
        RESET
    };
    // AABB aabb(RESET) generates a reset aabb
    ILINE  AABB(type_reset)
    { Reset(); }
    ILINE  explicit AABB(float radius)
    { max = Vec3(radius); min = -max; }
    ILINE  explicit AABB(const Vec3& v)
    { min = max = v; }
    ILINE  AABB(const Vec3& v, float radius)
    { Vec3 ext(radius); min = v - ext; max = v + ext; }
    ILINE  AABB(const Vec3& vmin, const Vec3& vmax)
    { min = vmin; max = vmax; }
    ILINE  AABB(const AABB& aabb)
    {
        min.x = aabb.min.x;
        min.y = aabb.min.y;
        min.z = aabb.min.z;
        max.x = aabb.max.x;
        max.y = aabb.max.y;
        max.z = aabb.max.z;
    }
    inline AABB(const Vec3* points, int num)
    {
        Reset();
        for (int i = 0; i < num; i++)
        {
            Add(points[i]);
        }
    }

    //! Reset Bounding box before calculating bounds.
    //! These values ensure that Add() functions work correctly for Reset bbs, without additional comparisons.
    ILINE void Reset()
    {   min = Vec3(1e15f);  max = Vec3(-1e15f); }

    ILINE bool IsReset() const
    { return min.x > max.x; }

    ILINE float IsResetSel(float ifReset, float ifNotReset) const
    { return (float)fsel(max.x - min.x, ifNotReset, ifReset); }

    //! Check if bounding box is empty (Zero volume).
    ILINE bool IsEmpty() const
    { return min == max; }

    //! Check if bounding box has valid, non zero volume
    ILINE bool IsNonZero() const
    { return min.x < max.x && min.y < max.y && min.z < max.z; }

    ILINE Vec3 GetCenter() const
    { return (min + max) * 0.5f; }

    ILINE Vec3 GetSize() const
    { return (max - min) * IsResetSel(0.0f, 1.0f); }

    ILINE float GetRadius() const
    { return IsResetSel(0.0f, (max - min).GetLengthFloat() * 0.5f); }

    ILINE void Add(const Vec3& v)
    {
        min.CheckMin(v);
        max.CheckMax(v);
    }

    inline void Add(const Vec3& v, float radius)
    {
        Vec3 ext(radius);
        min.CheckMin(v - ext);
        max.CheckMax(v + ext);
    }

    ILINE void Add(const AABB& bb)
    {
        min.CheckMin(bb.min);
        max.CheckMax(bb.max);
    }

    //! Check if this bounding box contains a point within itself.
    bool IsContainPoint(const Vec3& pos) const
    {
        assert(min.IsValid());
        assert(max.IsValid());
        assert(pos.IsValid());
        if (pos.x < min.x)
        {
            return false;
        }
        if (pos.y < min.y)
        {
            return false;
        }
        if (pos.z < min.z)
        {
            return false;
        }
        if (pos.x > max.x)
        {
            return false;
        }
        if (pos.y > max.y)
        {
            return false;
        }
        if (pos.z > max.z)
        {
            return false;
        }
        return true;
    }

    float GetDistanceSqr(Vec3 const& v) const
    {
        Vec3 vNear = v;
        vNear.CheckMax(min);
        vNear.CheckMin(max);
        return vNear.GetSquaredDistance(v);
    }

    float GetDistance(Vec3 const& v) const
    {
        return sqrt(GetDistanceSqr(v));
    }

    // Check two bounding boxes for intersection.
    inline bool IsIntersectBox(const AABB& b) const
    {
        assert(min.IsValid());
        assert(max.IsValid());
        assert(b.min.IsValid());
        assert(b.max.IsValid());
        // Check for intersection on X axis.
        if ((min.x > b.max.x) || (b.min.x > max.x))
        {
            return false;
        }
        // Check for intersection on Y axis.
        if ((min.y > b.max.y) || (b.min.y > max.y))
        {
            return false;
        }
        // Check for intersection on Z axis.
        if ((min.z > b.max.z) || (b.min.z > max.z))
        {
            return false;
        }
        // Boxes overlap in all 3 axises.
        return true;
    }

    /*!
    * calculate the new bounds of a transformed AABB
    *
    * Example:
    * AABB aabb = AABB::CreateAABBfromOBB(m34,aabb);
    *
    * return values:
    *  expanded AABB in world-space
    */
    ILINE void SetTransformedAABB(const Matrix34& m34, const AABB& aabb)
    {
        if (aabb.IsReset())
        {
            Reset();
        }
        else
        {
            Matrix33 m33;
            m33.m00 = fabs_tpl(m34.m00);
            m33.m01 = fabs_tpl(m34.m01);
            m33.m02 = fabs_tpl(m34.m02);
            m33.m10 = fabs_tpl(m34.m10);
            m33.m11 = fabs_tpl(m34.m11);
            m33.m12 = fabs_tpl(m34.m12);
            m33.m20 = fabs_tpl(m34.m20);
            m33.m21 = fabs_tpl(m34.m21);
            m33.m22 = fabs_tpl(m34.m22);

            Vec3 sz     =   m33 * ((aabb.max - aabb.min) * 0.5f);
            Vec3 pos    = m34 * ((aabb.max + aabb.min) * 0.5f);
            min = pos - sz;
            max = pos + sz;
        }
    }
};

ILINE bool IsEquivalent(const AABB& a, const AABB& b, float epsilon = VEC_EPSILON)
{
    return IsEquivalent(a.min, b.min, epsilon) && IsEquivalent(a.max, b.max, epsilon);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// struct OBB
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template <typename F>
struct OBB_tpl
{
    Matrix33 m33; //orientation vectors
    Vec3 h;             //half-length-vector
    Vec3 c;             //center of obb

    //default OBB constructor (without initialisation)
    inline OBB_tpl() {}

    ILINE void SetOBB(const Matrix33& matrix, const Vec3& hlv, const Vec3& center) {  m33 = matrix; h = hlv; c = center;   }
    ILINE static OBB_tpl<F> CreateOBB(const Matrix33& m33, const Vec3& hlv, const Vec3& center) {    OBB_tpl<f32> obb; obb.m33 = m33; obb.h = hlv; obb.c = center; return obb; }

    ILINE void SetOBBfromAABB(const Matrix33& mat33, const AABB& aabb)
    {
        m33 =   mat33;
        h       =   (aabb.max - aabb.min) * 0.5f;   //calculate the half-length-vectors
        c       =   (aabb.max + aabb.min) * 0.5f;   //the center is relative to the PIVOT
    }
    ILINE void SetOBBfromAABB(const Quat& q, const AABB& aabb)
    {
        m33 =   Matrix33(q);
        h       =   (aabb.max - aabb.min) * 0.5f;   //calculate the half-length-vectors
        c       =   (aabb.max + aabb.min) * 0.5f;   //the center is relative to the PIVOT
    }
    ILINE static OBB_tpl<F> CreateOBBfromAABB(const Matrix33& m33, const AABB& aabb) { OBB_tpl<f32> obb; obb.SetOBBfromAABB(m33, aabb); return obb;    }
    ILINE static OBB_tpl<F> CreateOBBfromAABB(const Quat& q, const AABB& aabb) { OBB_tpl<f32> obb; obb.SetOBBfromAABB(q, aabb); return obb;    }

    ~OBB_tpl(void) {};
};

typedef OBB_tpl<f32>    OBB;



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// struct Sphere
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
struct Sphere
{
    Vec3 center;
    float radius;

    Sphere() {}
    Sphere(const Vec3& c, float r)
        : center(c)
        , radius(r) {}
    void operator()(const Vec3& c, float r) { center = c; radius = r; }
};

typedef Triangle_tpl<f32>   Triangle;
