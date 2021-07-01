/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description: Hardware vector class - scalar implementation


#ifndef CRYINCLUDE_CRYCOMMON_CRY_HWVECTOR3_H
#define CRYINCLUDE_CRYCOMMON_CRY_HWVECTOR3_H
#pragma once


#define HWV_PERMUTE_0X 0
#define HWV_PERMUTE_0Y 1
#define HWV_PERMUTE_0Z 2
#define HWV_PERMUTE_0W 3
#define HWV_PERMUTE_1X 4
#define HWV_PERMUTE_1Y 5
#define HWV_PERMUTE_1Z 6
#define HWV_PERMUTE_1W 7

typedef Vec3 hwvec3;
typedef Vec4 hwvec4;
typedef float simdf;
typedef Vec4 hwvec4fconst;
typedef int hwvec4i[4];

#define HWV3Constant(name, f0, f1, f2) const Vec3 name(f0, f1, f2)
#define SIMDFConstant(name, f0) const simdf name = f0
#define HWV4PermuteControl(name, i0, i1, i2, i3) static const hwvec4i name = {i0, i1, i2, i3};
#define SIMDFAsVec3(a) (hwvec3)a
#define HWV3AsSIMDF(a) (simdf)a.x

ILINE hwvec3 HWVLoadVecUnaligned(const Vec3* pLoadFrom)
{
    return *pLoadFrom;
}

ILINE hwvec3 HWVLoadVecAligned(const Vec4A* pLoadFrom)
{
    return Vec3(pLoadFrom->x, pLoadFrom->y, pLoadFrom->z);
}

ILINE void HWVSaveVecUnaligned(Vec3* pSaveTo, const hwvec3& pLoadFrom)
{
    *pSaveTo = pLoadFrom;
}

ILINE void HWVSaveVecAligned(Vec4* pSaveTo, const hwvec4& pLoadFrom)
{
    *pSaveTo = pLoadFrom;
}

ILINE hwvec3 HWVAdd(const hwvec3& a, const hwvec3& b)
{
    return hwvec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

ILINE hwvec3 HWVMultiply(const hwvec3& a, const hwvec3& b)
{
    return hwvec3(a.x * b.x, a.y * b.y, a.z * b.z);
}

ILINE hwvec3 HWVMultiplySIMDF(const hwvec3& a, const simdf& b)
{
    return hwvec3(a.x * b, a.y * b, a.z * b);
}

ILINE hwvec3 HWVMultiplyAdd(const hwvec3& a, const hwvec3& b, const hwvec3& c)
{
    return hwvec3((a.x * b.x) + c.x, (a.y * b.y) + c.y, (a.z * b.z) + c.z);
}

ILINE hwvec3 HWVMultiplySIMDFAdd(const hwvec3& a, const simdf& b, const hwvec3& c)
{
    return hwvec3((a.x * b) + c.x, (a.y * b) + c.y, (a.z * b) + c.z);
}

ILINE hwvec3 HWVSub(const hwvec3& a, const hwvec3& b)
{
    return (a - b);
}

ILINE hwvec3 HWVCross(const hwvec3& a, const hwvec3& b)
{
    return hwvec3((a.y * b.z) - (a.z * b.y)
        , (a.z * b.x) - (a.x * b.z)
        , (a.x * b.y) - (a.y * b.x));
}

ILINE simdf HWV3Dot(const hwvec3& a, const hwvec3& b)
{
    return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

ILINE hwvec3 HWVMax(const hwvec3& a, const hwvec3& b)
{
    return Vec3(a.x > b.x ? a.x : b.x,
        a.y > b.y ? a.y : b.y,
        a.z > b.z ? a.z : b.z);
}

ILINE hwvec3 HWVMin(const hwvec3& a, const hwvec3& b)
{
    return Vec3(a.x < b.x ? a.x : b.x,
        a.y < b.y ? a.y : b.y,
        a.z < b.z ? a.z : b.z);
}

ILINE hwvec3 HWVClamp(const hwvec3& a, const hwvec3& min, const hwvec3& max)
{
    return HWVMax(min, HWVMin(a, max));
}

ILINE hwvec3 HWV3Normalize(const hwvec3& a)
{
    float fInvLen = isqrt_safe_tpl((a.x * a.x) + (a.y * a.y) + (a.z * a.z));
    return Vec3(a.x * fInvLen, a.y * fInvLen, a.z * fInvLen);
}

ILINE hwvec3 HWVGetOrthogonal(const hwvec3& a)
{
    hwvec3 result = a;

    //int i = isneg(square(0.9f)*a.GetLengthSquared()-(a.x*a.x));
    int i = isneg(square(0.9f) * a.GetLengthSquared() - a.x * a.x);
    result[i] = 0;
    result[incm3(i)] = a[decm3(i)];
    result[decm3(i)] = -a[incm3(i)];
    return result;
}

ILINE simdf HWV3SplatXToSIMDF(const hwvec3& a)
{
    return a.x;
}

ILINE simdf HWV3SplatYToSIMDF(const hwvec3& a)
{
    return a.y;
}

ILINE hwvec3 HWV3PermuteWord(const hwvec3& a, const hwvec3& b, const hwvec4i& p)
{
    hwvec3 selection[2] = {a, b};
    float* fSelection = (float*)selection;
    return Vec3(fSelection[p[0]], fSelection[p[1]], fSelection[p[2]]);
}

ILINE hwvec3 HWV3Zero()
{
    return Vec3(0.0f, 0.0f, 0.0f);
}

ILINE hwvec4 HWV4Zero()
{
    return Vec4(0.0f, 0.0f, 0.0f, 0.0f);
}


ILINE hwvec3 HWV3Negate(const hwvec3& a)
{
    return Vec3(-a.x, -a.y, -a.z);
}

ILINE hwvec3 HWVSelect(const hwvec3& a, const hwvec3& b, const hwvec3& control)
{
    return Vec3(control[0] > 0.0f ? b[0] : a[0],
        control[1] > 0.0f ? b[1] : a[1],
        control[2] > 0.0f ? b[2] : a[2]);
}

ILINE hwvec3 HWVSelectSIMDF(const hwvec3& a, const hwvec3& b, const bool& control)
{
    return control ? b : a;
}

ILINE simdf HWV3LengthSq(const hwvec3& a)
{
    return a.len2();
}


//////////////////////////////////////////////////
//SIMDF functions for float stored in HWVEC4 data
//////////////////////////////////////////////////

ILINE simdf SIMDFLoadFloat(const float& f)
{
    return f;
}

ILINE void SIMDFSaveFloat(float* f, const simdf& a)
{
    *f = a;
}

ILINE bool SIMDFGreaterThan(const simdf& a, const simdf& b)
{
    return (a > b);
}

ILINE bool SIMDFLessThanEqualB(const simdf& a, const simdf& b)
{
    return (a <= b);
}

ILINE bool SIMDFLessThanEqual(const simdf& a, const simdf& b)
{
    return (a <= b);
}

ILINE bool SIMDFLessThanB(const simdf& a, const simdf& b)
{
    return (a < b);
}

ILINE bool SIMDFLessThan(const simdf& a, const simdf& b)
{
    return (a < b);
}

ILINE simdf SIMDFAdd(const simdf& a, const simdf& b)
{
    return a + b;
}

ILINE simdf SIMDFMult(const simdf& a, const simdf& b)
{
    return a * b;
}

ILINE simdf SIMDFReciprocal(const simdf& a)
{
    return 1.0f / a;
}

ILINE simdf SIMDFSqrt(const simdf& a)
{
    return sqrt_tpl(a);
}

ILINE simdf SIMDFSqrtEst(const simdf& a)
{
    return sqrt_tpl(a);
}

ILINE simdf SIMDFSqrtEstFast(const simdf& a)
{
    return sqrt_tpl(a);
}

ILINE simdf SIMDFMax(const simdf& a, const simdf& b)
{
    return max(a, b);
}

ILINE simdf SIMDFMin(const simdf& a, const simdf& b)
{
    return min(a, b);
}

ILINE simdf SIMDFClamp(const simdf& a, const simdf& min, const simdf& max)
{
    return clamp_tpl(a, min, max);
}

ILINE simdf SIMDFAbs(const simdf& a)
{
    return fabsf(a);
}

#endif // CRYINCLUDE_CRYCOMMON_CRY_HWVECTOR3_H
