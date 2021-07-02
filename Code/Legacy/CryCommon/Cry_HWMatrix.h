/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Scalar implementation of hardware vector based matrix


#ifndef CRYINCLUDE_CRYCOMMON_CRY_HWMATRIX_H
#define CRYINCLUDE_CRYCOMMON_CRY_HWMATRIX_H
#pragma once

struct hwmtx33
{
    hwvec3 m0, m1, m2;
};

ILINE void HWMtx33LoadAligned(hwmtx33& out, const Matrix34A& inMtx)
{
    out.m0 = HWVLoadVecAligned(reinterpret_cast<const Vec4A*>(&inMtx.m00));
    out.m1 = HWVLoadVecAligned(reinterpret_cast<const Vec4A*>(&inMtx.m10));
    out.m2 = HWVLoadVecAligned(reinterpret_cast<const Vec4A*>(&inMtx.m20));
}

ILINE hwvec3 HWMtx33RotateVec(const hwmtx33& m, const hwvec3& v)
{
    return Vec3((m.m0[0] * v.x) + (m.m0[1] * v.y) + (m.m0[2] * v.z),
        (m.m1[0] * v.x) + (m.m1[1] * v.y) + (m.m1[2] * v.z),
        (m.m2[0] * v.x) + (m.m2[1] * v.y) + (m.m2[2] * v.z));
}

ILINE hwvec3 HWMtx33RotateVecOpt(const hwmtx33& m, const hwvec3& v)
{
    return HWMtx33RotateVec(m, v);
}

ILINE hwmtx33 HWMtx33CreateRotationV0V1(const hwvec3& v0, const hwvec3& v1)
{
    assert((fabs_tpl(1 - (v0 | v0))) < 0.01); //check if unit-vector
    assert((fabs_tpl(1 - (v1 | v1))) < 0.01); //check if unit-vector
    hwmtx33 m;
    float dot = v0 | v1;
    if (dot < -0.9999f)
    {
        Vec3 axis = v0.GetOrthogonal().GetNormalized();
        m.m0[0] = (2.0f * axis.x * axis.x - 1);
        m.m0[1] = (2.0f * axis.x * axis.y);
        m.m0[2] = (2.0f * axis.x * axis.z);
        m.m1[0] = (2.0f * axis.y * axis.x);
        m.m1[1] = (2.0f * axis.y * axis.y - 1);
        m.m1[2] = (2.0f * axis.y * axis.z);
        m.m2[0] = (2.0f * axis.z * axis.x);
        m.m2[1] = (2.0f * axis.z * axis.y);
        m.m2[2] = (2.0f * axis.z * axis.z - 1);
    }
    else
    {
        Vec3 v = v0 % v1;
        f32 h = 1.0f / (1.0f + dot);
        m.m0[0] = (dot + h * v.x * v.x);
        m.m0[1] = (h * v.x * v.y - v.z);
        m.m0[2] = (h * v.x * v.z + v.y);
        m.m1[0] = (h * v.x * v.y + v.z);
        m.m1[1] = (dot + h * v.y * v.y);
        m.m1[2] = (h * v.y * v.z - v.x);
        m.m2[0] = (h * v.x * v.z - v.y);
        m.m2[1] = (h * v.y * v.z + v.x);
        m.m2[2] = (dot + h * v.z * v.z);
    }

    return m;
}

//Returns a matrix optimized for this platform's matrix ops, in this case, not doing a thing;
ILINE hwmtx33 HWMtx33GetOptimized(const hwmtx33& m)
{
    return (hwmtx33)m;
}

#endif // CRYINCLUDE_CRYCOMMON_CRY_HWMATRIX_H

