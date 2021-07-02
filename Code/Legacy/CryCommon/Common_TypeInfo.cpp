/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "TypeInfo_impl.h"
#include "Cry_Geo.h"

STRUCT_INFO_T_BEGIN(Vec2_tpl, class, F)
VAR_INFO(x)
VAR_INFO(y)
STRUCT_INFO_T_END(Vec2_tpl, class, F)

#include "Cry_Vector3.h"

STRUCT_INFO_T_BEGIN(Vec3_tpl, typename, F)
VAR_INFO(x)
VAR_INFO(y)
VAR_INFO(z)
STRUCT_INFO_T_END(Vec3_tpl, typename, F)

typedef TFixed<unsigned char, 1, 255, 0> TFixedUChar_1_255_0;
STRUCT_INFO_T_INSTANTIATE(Vec3_tpl, TFixedUChar_1_255_0)

STRUCT_INFO_T_BEGIN(Vec4_tpl, typename, F)
VAR_INFO(x)
VAR_INFO(y)
VAR_INFO(z)
VAR_INFO(w)
STRUCT_INFO_T_END(Vec4_tpl, typename, F)

STRUCT_INFO_T_INSTANTIATE(Vec4_tpl, short)

STRUCT_INFO_T_BEGIN(Ang3_tpl, typename, F)
VAR_INFO(x)
VAR_INFO(y)
VAR_INFO(z)
STRUCT_INFO_T_END(Ang3_tpl, typename, F)

STRUCT_INFO_T_BEGIN(Plane_tpl, typename, F)
VAR_INFO(n)
VAR_INFO(d)
STRUCT_INFO_T_END(Plane_tpl, typename, F)

//-----------------------------------------------------------------
//#include "Cry_Quat_info.h"
STRUCT_INFO_T_BEGIN(Quat_tpl, typename, F)
VAR_INFO(v)
VAR_INFO(w)
STRUCT_INFO_T_END(Quat_tpl, typename, F)

STRUCT_INFO_T_INSTANTIATE(Quat_tpl, float)

STRUCT_INFO_T_BEGIN(QuatT_tpl, typename, F)
VAR_INFO(q)
VAR_INFO(t)
STRUCT_INFO_T_END(QuatT_tpl, typename, F)

STRUCT_INFO_T_INSTANTIATE(QuatT_tpl, float)

STRUCT_INFO_T_BEGIN(QuatTS_tpl, typename, F)
VAR_INFO(q)
VAR_INFO(t)
VAR_INFO(s)
STRUCT_INFO_T_END(QuatTS_tpl, typename, F)

STRUCT_INFO_T_BEGIN(DualQuat_tpl, typename, F)
VAR_INFO(nq)
VAR_INFO(dq)
STRUCT_INFO_T_END(DualQuat_tpl, typename, F)


//------------------------------------------------------------
//#include "Cry_Matrix_info.h"
STRUCT_INFO_T_BEGIN(Matrix33_tpl, typename, F)
VAR_INFO(m00)
VAR_INFO(m01)
VAR_INFO(m02)
VAR_INFO(m10)
VAR_INFO(m11)
VAR_INFO(m12)
VAR_INFO(m20)
VAR_INFO(m21)
VAR_INFO(m22)
STRUCT_INFO_T_END(Matrix33_tpl, typename, F)

STRUCT_INFO_T_BEGIN(Matrix34_tpl, typename, F)
VAR_INFO(m00)
VAR_INFO(m01)
VAR_INFO(m02)
VAR_INFO(m03)
VAR_INFO(m10)
VAR_INFO(m11)
VAR_INFO(m12)
VAR_INFO(m13)
VAR_INFO(m20)
VAR_INFO(m21)
VAR_INFO(m22)
VAR_INFO(m23)
STRUCT_INFO_T_END(Matrix34_tpl, typename, F)

STRUCT_INFO_T_INSTANTIATE(Matrix34_tpl, float)

STRUCT_INFO_T_BEGIN(Matrix44_tpl, typename, F)
VAR_INFO(m00)
VAR_INFO(m01)
VAR_INFO(m02)
VAR_INFO(m03)
VAR_INFO(m10)
VAR_INFO(m11)
VAR_INFO(m12)
VAR_INFO(m13)
VAR_INFO(m20)
VAR_INFO(m21)
VAR_INFO(m22)
VAR_INFO(m23)
VAR_INFO(m30)
VAR_INFO(m31)
VAR_INFO(m32)
VAR_INFO(m33)
STRUCT_INFO_T_END(Matrix44_tpl, typename, F)


//#include "Cry_Color_info.h"
STRUCT_INFO_T_BEGIN(Color_tpl, class, T)
VAR_INFO(r)
VAR_INFO(g)
VAR_INFO(b)
VAR_INFO(a)
STRUCT_INFO_T_END(Color_tpl, class, T)

STRUCT_INFO_T_INSTANTIATE(Color_tpl, unsigned char)

//#include "Cry_Geo_info.h"
STRUCT_INFO_BEGIN(AABB)
VAR_INFO(min)
VAR_INFO(max)
STRUCT_INFO_END(AABB)

STRUCT_INFO_BEGIN(RectF)
VAR_INFO(x)
VAR_INFO(y)
VAR_INFO(w)
VAR_INFO(h)
STRUCT_INFO_END(RectF)

#include "TimeValue_info.h"
#include "CryHalf_info.h"

// Manually instantiate templates as needed here.
template struct Vec3_tpl<float>;
template struct Vec4_tpl<float>;
template struct Vec2_tpl<float>;
template struct Ang3_tpl<float>;
template struct Plane_tpl<float>;
template struct Matrix33_tpl<float>;
template struct Color_tpl<float>;
