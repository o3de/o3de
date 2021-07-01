/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Misc mathematical functions


#ifndef CRYINCLUDE_CRYCOMMON_CRY_XOPTIMISE_H
#define CRYINCLUDE_CRYCOMMON_CRY_XOPTIMISE_H
#pragma once

#include <platform.h>




inline float AngleMod(float a)
{
    a = (float)((360.0 / 65536) * ((int)(a * (65536 / 360.0)) & 65535));
    return a;
}

inline float AngleModRad(float a)
{
    a = (float)((gf_PI2 / 65536) * ((int)(a * (65536 / gf_PI2)) & 65535));
    return a;
}
inline unsigned short Degr2Word(float f)
{
    return (unsigned short)(AngleMod(f) / 360.0f * 65536.0f);
}
inline float Word2Degr(unsigned short s)
{
    return (float)s / 65536.0f * 360.0f;
}

#if defined(_CPU_X86)
ILINE float __fastcall Ffabs(float f)
{
    *((unsigned*) &f) &= ~0x80000000;
    return (f);
}
#else
inline float Ffabs(float x) { return fabsf(x); }
#endif



#define mathMatrixRotationZ(pOut, angle) (*(Matrix44*)pOut) = GetTransposed44(Matrix44(Matrix34::CreateRotationZ(angle)))
#define mathMatrixRotationY(pOut, angle) (*(Matrix44*)pOut) = GetTransposed44(Matrix44(Matrix34::CreateRotationY(angle)))
#define mathMatrixRotationX(pOut, angle) (*(Matrix44*)pOut) = GetTransposed44(Matrix44(Matrix34::CreateRotationX(angle)))
#define mathMatrixTranslation(pOut, x, y, z) (*(Matrix44*)pOut) = GetTransposed44(Matrix44(Matrix34::CreateTranslationMat(Vec3(x, y, z))))
#define mathMatrixScaling(pOut, sx, sy, sz) (*(Matrix44*)pOut) = GetTransposed44(Matrix44(Matrix34::CreateScale(Vec3(sx, sy, sz))))

template <class T>
inline void ExchangeVals(T& X, T& Y)
{
    const T Tmp = X;
    X = Y;
    Y = Tmp;
}


inline void mathMatrixPerspectiveFov(Matrix44A* pMatr, f32 fovY, f32 Aspect, f32 zn, f32 zf)
{
    f32 yScale = 1.0f / tan_tpl(fovY / 2.0f);
    f32 xScale = yScale / Aspect;

    f32 m22 = f32(f64(zf) / (f64(zn) - f64(zf)));
    f32 m32 = f32(f64(zn) * f64(zf) / (f64(zn) - f64(zf)));

    (*pMatr)(0, 0) = xScale;
    (*pMatr)(0, 1) = 0;
    (*pMatr)(0, 2) = 0;
    (*pMatr)(0, 3) = 0;
    (*pMatr)(1, 0) = 0;
    (*pMatr)(1, 1) = yScale;
    (*pMatr)(1, 2) = 0;
    (*pMatr)(1, 3) = 0;
    (*pMatr)(2, 0) = 0;
    (*pMatr)(2, 1) = 0;
    (*pMatr)(2, 2) = m22;
    (*pMatr)(2, 3) = -1.0f;
    (*pMatr)(3, 0) = 0;
    (*pMatr)(3, 1) = 0;
    (*pMatr)(3, 2) = m32;
    (*pMatr)(3, 3) = 0;
}



inline void mathMatrixOrtho(Matrix44A* pMatr, f32 w, f32 h, f32 zn, f32 zf)
{
    f32 m22 = f32(1.0 / (f64(zn) - f64(zf)));
    f32 m32 = f32(f64(zn) / (f64(zn) - f64(zf)));

    (*pMatr)(0, 0) = 2.0f / w;
    (*pMatr)(0, 1) = 0;
    (*pMatr)(0, 2) = 0;
    (*pMatr)(0, 3) = 0;
    (*pMatr)(1, 0) =     0;
    (*pMatr)(1, 1) = 2.0f / h;
    (*pMatr)(1, 2) = 0;
    (*pMatr)(1, 3) = 0;
    (*pMatr)(2, 0) =     0;
    (*pMatr)(2, 1) = 0;
    (*pMatr)(2, 2) = m22;
    (*pMatr)(2, 3) = 0;
    (*pMatr)(3, 0) =     0;
    (*pMatr)(3, 1) = 0;
    (*pMatr)(3, 2) = m32;
    (*pMatr)(3, 3) = 1;
}

inline void mathMatrixOrthoOffCenter(Matrix44A* pMatr, f32 l, f32 r, f32 b, f32 t, f32 zn, f32 zf)
{
    f32 m22 = f32(1.0 / (f64(zn) - f64(zf)));
    f32 m32 = f32(f64(zn) / (f64(zn) - f64(zf)));

    (*pMatr)(0, 0) = 2.0f / (r - l);
    (*pMatr)(0, 1) = 0;
    (*pMatr)(0, 2) = 0;
    (*pMatr)(0, 3) = 0;
    (*pMatr)(1, 0) =     0;
    (*pMatr)(1, 1) = 2.0f / (t - b);
    (*pMatr)(1, 2) = 0;
    (*pMatr)(1, 3) = 0;
    (*pMatr)(2, 0) =     0;
    (*pMatr)(2, 1) =  0;
    (*pMatr)(2, 2) = m22;
    (*pMatr)(2, 3) = 0;
    (*pMatr)(3, 0) = (l + r) / (l - r);
    (*pMatr)(3, 1) = (t + b) / (b - t);
    (*pMatr)(3, 2) = m32;
    (*pMatr)(3, 3) = 1.0f;
}


inline void mathMatrixOrthoOffCenterLH(Matrix44A* pMatr, f32 l, f32 r, f32 b, f32 t, f32 zn, f32 zf)
{
    f32 m22 = f32(1.0 / (f64(zf) - f64(zn)));
    f32 m32 = f32(f64(zn) / (f64(zn) - f64(zf)));

    (*pMatr)(0, 0) = 2.0f / (r - l);
    (*pMatr)(0, 1) = 0;
    (*pMatr)(0, 2) = 0;
    (*pMatr)(0, 3) = 0;
    (*pMatr)(1, 0) =     0;
    (*pMatr)(1, 1) = 2.0f / (t - b);
    (*pMatr)(1, 2) = 0;
    (*pMatr)(1, 3) = 0;
    (*pMatr)(2, 0) =     0;
    (*pMatr)(2, 1) =  0;
    (*pMatr)(2, 2) = m22;
    (*pMatr)(2, 3) = 0;
    (*pMatr)(3, 0) = (l + r) / (l - r);
    (*pMatr)(3, 1) = (t + b) / (b - t);
    (*pMatr)(3, 2) = m32;
    (*pMatr)(3, 3) = 1.0f;
}


inline void mathMatrixPerspectiveOffCenter(Matrix44A* pMatr, f32 l, f32 r, f32 b, f32 t, f32 zn, f32 zf)
{
    f32 m22 = f32(f64(zf) / (f64(zn) - f64(zf)));
    f32 m32 = f32(f64(zn) * f64(zf) / (f64(zn) - f64(zf)));

    (*pMatr)(0, 0) = 2 * zn / (r - l);
    (*pMatr)(0, 1) =  0;
    (*pMatr)(0, 2) = 0;
    (*pMatr)(0, 3) = 0;
    (*pMatr)(1, 0) = 0;
    (*pMatr)(1, 1) = 2 * zn / (t - b);
    (*pMatr)(1, 2) = 0;
    (*pMatr)(1, 3) = 0;
    (*pMatr)(2, 0) = (l + r) / (r - l);
    (*pMatr)(2, 1) = (t + b) / (t - b);
    (*pMatr)(2, 2) =   m22;
    (*pMatr)(2, 3) = -1;
    (*pMatr)(3, 0) = 0;
    (*pMatr)(3, 1) = 0;
    (*pMatr)(3, 2) =   m32;
    (*pMatr)(3, 3) = 0;
}

inline void mathMatrixPerspectiveOffCenterReverseDepth(Matrix44A* pMatr, f32 l, f32 r, f32 b, f32 t, f32 zn, f32 zf)
{
    f32 m22 = f32(-f64(zn) / (f64(zn) - f64(zf)));
    f32 m32 = f32(-f64(zn) * f64(zf) / (f64(zn) - f64(zf)));

    (*pMatr)(0, 0) = 2 * zn / (r - l);
    (*pMatr)(0, 1) =  0;
    (*pMatr)(0, 2) = 0;
    (*pMatr)(0, 3) = 0;
    (*pMatr)(1, 0) = 0;
    (*pMatr)(1, 1) = 2 * zn / (t - b);
    (*pMatr)(1, 2) = 0;
    (*pMatr)(1, 3) = 0;
    (*pMatr)(2, 0) = (l + r) / (r - l);
    (*pMatr)(2, 1) = (t + b) / (t - b);
    (*pMatr)(2, 2) =   m22;
    (*pMatr)(2, 3) = -1;
    (*pMatr)(3, 0) = 0;
    (*pMatr)(3, 1) = 0;
    (*pMatr)(3, 2) =   m32;
    (*pMatr)(3, 3) = 0;
}

//RH
inline void mathMatrixLookAt(Matrix44A* pMatr, const Vec3& Eye, const Vec3& At, const Vec3& Up)
{
    Vec3 vLightDir = (Eye - At);
    Vec3 zaxis = vLightDir.GetNormalized();
    Vec3 xaxis = (Up.Cross(zaxis)).GetNormalized();
    Vec3 yaxis = zaxis.Cross(xaxis);

    (*pMatr)(0, 0) = xaxis.x;
    (*pMatr)(0, 1) = yaxis.x;
    (*pMatr)(0, 2) = zaxis.x;
    (*pMatr)(0, 3) = 0;
    (*pMatr)(1, 0) = xaxis.y;
    (*pMatr)(1, 1) = yaxis.y;
    (*pMatr)(1, 2) = zaxis.y;
    (*pMatr)(1, 3) = 0;
    (*pMatr)(2, 0) = xaxis.z;
    (*pMatr)(2, 1) = yaxis.z;
    (*pMatr)(2, 2) = zaxis.z;
    (*pMatr)(2, 3) = 0;
    (*pMatr)(3, 0) = -xaxis.Dot(Eye);
    (*pMatr)(3, 1) = -yaxis.Dot(Eye);
    (*pMatr)(3, 2) = -zaxis.Dot(Eye);
    (*pMatr)(3, 3) = 1;
}

inline bool mathMatrixPerspectiveFovInverse(Matrix44_tpl<f64>* pResult, const Matrix44A* pProjFov)
{
    if ((*pProjFov)(0, 1) == 0.0f && (*pProjFov)(0, 2) == 0.0f && (*pProjFov)(0, 3) == 0.0f &&
        (*pProjFov)(1, 0) == 0.0f && (*pProjFov)(1, 2) == 0.0f && (*pProjFov)(1, 3) == 0.0f &&
        (*pProjFov)(3, 0) == 0.0f && (*pProjFov)(3, 1) == 0.0f && (*pProjFov)(3, 2) != 0.0f)
    {
        (*pResult)(0, 0) = 1.0 / (*pProjFov).m00;
        (*pResult)(0, 1) = 0;
        (*pResult)(0, 2) = 0;
        (*pResult)(0, 3) = 0;
        (*pResult)(1, 0) = 0;
        (*pResult)(1, 1) = 1.0 / (*pProjFov).m11;
        (*pResult)(1, 2) = 0;
        (*pResult)(1, 3) = 0;
        (*pResult)(2, 0) = 0;
        (*pResult)(2, 1) = 0;
        (*pResult)(2, 2) = 0;
        (*pResult)(2, 3) = 1.0 / (*pProjFov).m32;
        (*pResult)(3, 0) = (*pProjFov).m20 / (*pProjFov).m00;
        (*pResult)(3, 1) = (*pProjFov).m21 / (*pProjFov).m11;
        (*pResult)(3, 2) = -1;
        (*pResult)(3, 3) = (*pProjFov).m22 / (*pProjFov).m32;

        return true;
    }

    return false;
}

template<class T_out, class T_in>
inline void mathMatrixLookAtInverse(Matrix44_tpl<T_out>* pResult, const Matrix44_tpl<T_in>* pLookAt)
{
    (*pResult)(0, 0) = (*pLookAt).m00;
    (*pResult)(0, 1) = (*pLookAt).m10;
    (*pResult)(0, 2) = (*pLookAt).m20;
    (*pResult)(0, 3) = (*pLookAt).m03;
    (*pResult)(1, 0) = (*pLookAt).m01;
    (*pResult)(1, 1) = (*pLookAt).m11;
    (*pResult)(1, 2) = (*pLookAt).m21;
    (*pResult)(1, 3) = (*pLookAt).m13;
    (*pResult)(2, 0) = (*pLookAt).m02;
    (*pResult)(2, 1) = (*pLookAt).m12;
    (*pResult)(2, 2) = (*pLookAt).m22;
    (*pResult)(2, 3) = (*pLookAt).m23;

    (*pResult)(3, 0) = T_out(-(f64((*pLookAt).m00) * f64((*pLookAt).m30) + f64((*pLookAt).m01) * f64((*pLookAt).m31) + f64((*pLookAt).m02) * f64((*pLookAt).m32)));
    (*pResult)(3, 1) = T_out(-(f64((*pLookAt).m10) * f64((*pLookAt).m30) + f64((*pLookAt).m11) * f64((*pLookAt).m31) + f64((*pLookAt).m12) * f64((*pLookAt).m32)));
    (*pResult)(3, 2) = T_out(-(f64((*pLookAt).m20) * f64((*pLookAt).m30) + f64((*pLookAt).m21) * f64((*pLookAt).m31) + f64((*pLookAt).m22) * f64((*pLookAt).m32)));
    (*pResult)(3, 3) = (*pLookAt).m33;
};

inline void mathVec4Transform(f32 out[4], const f32 m[16], const f32 in[4])
{
#define M(row, col)  m[col * 4 + row]
    out[0] =    M(0, 0) * in[0] + M(0, 1) * in[1] + M(0, 2) * in[2] + M(0, 3) * in[3];
    out[1] =    M(1, 0) * in[0] + M(1, 1) * in[1] + M(1, 2) * in[2] + M(1, 3) * in[3];
    out[2] =    M(2, 0) * in[0] + M(2, 1) * in[1] + M(2, 2) * in[2] + M(2, 3) * in[3];
    out[3] =    M(3, 0) * in[0] + M(3, 1) * in[1] + M(3, 2) * in[2] + M(3, 3) * in[3];
#undef M
}

//fix: replace by 3x4 Matrix transformation and move to crymath
inline void mathVec3Transform(f32 out[4], const f32 m[16], const f32 in[3])
{
#define M(row, col)  m[col * 4 + row]
    out[0] =    M(0, 0) * in[0] + M(0, 1) * in[1] + M(0, 2) * in[2] + M(0, 3) * 1.0f;
    out[1] =    M(1, 0) * in[0] + M(1, 1) * in[1] + M(1, 2) * in[2] + M(1, 3) * 1.0f;
    out[2] =    M(2, 0) * in[0] + M(2, 1) * in[1] + M(2, 2) * in[2] + M(2, 3) * 1.0f;
    out[3] =    M(3, 0) * in[0] + M(3, 1) * in[1] + M(3, 2) * in[2] + M(3, 3) * 1.0f;
#undef M
}

#define mathVec3TransformF(pOut, pV, pM) mathVec3Transform((f32*)pOut, (const f32*)pM, (f32*)pV)
#define mathVec4TransformF(pOut, pV, pM) mathVec4Transform((f32*)pOut, (const f32*)pM, (f32*)pV)
#define mathVec3NormalizeF(pOut, pV) (*(Vec3*)pOut) = (((Vec3*)pV)->GetNormalizedSafe())
#define mathVec2NormalizeF(pOut, pV) (*(Vec2*)pOut) = (((Vec2*)pV)->GetNormalizedSafe())


//fix replace viewport by int16 array
//fix for d3d viewport
inline f32 mathVec3Project(Vec3* pvWin, const Vec3* pvObj, const int32 pViewport[4], const Matrix44A* pProjection, const Matrix44A* pView, const Matrix44A* pWorld)
{
    Vec4 in, out;

    in.x = pvObj->x;
    in.y = pvObj->y;
    in.z = pvObj->z;
    in.w = 1.0f;
    mathVec4Transform((f32*)&out, (f32*)pWorld, (f32*)&in);
    mathVec4Transform((f32*)&in, (f32*)pView, (f32*)&out);
    mathVec4Transform((f32*)&out, (f32*)pProjection, (f32*)&in);

    if (out.w == 0.0f)
    {
        return 0.f;
    }

    out.x /= out.w;
    out.y /= out.w;
    out.z /= out.w;

    //output coords
    pvWin->x = pViewport[0] + (1 + out.x) * pViewport[2] / 2;
    pvWin->y = pViewport[1] + (1 - out.y) * pViewport[3] / 2;  //flip coords for y axis

    //FIX: update fViewportMinZ fViewportMaxZ support for Viewport everywhere
    float fViewportMinZ = 0, fViewportMaxZ = 1.0f;

    pvWin->z = fViewportMinZ + out.z * (fViewportMaxZ - fViewportMinZ);

    return out.w;
}

inline Vec3* mathVec3UnProject(Vec3* pvObj, const Vec3* pvWin, const int32 pViewport[4], const Matrix44A* pProjection, const Matrix44A* pView, const Matrix44A* pWorld, [[maybe_unused]] int32 OptFlags)
{
    Matrix44A m, mA;
    Vec4 in, out;

    //FIX: update fViewportMinZ fViewportMaxZ support for Viewport everywhere
    float fViewportMinZ = 0, fViewportMaxZ = 1.0f;

    in.x = (pvWin->x - pViewport[0]) * 2 / pViewport[2] - 1.0f;
    in.y = 1.0f - ((pvWin->y - pViewport[1]) * 2 / pViewport[3]);   //flip coords for y axis
    in.z = (pvWin->z - fViewportMinZ) / (fViewportMaxZ - fViewportMinZ);
    in.w = 1.0f;

    //prepare inverse projection matrix
    mA = ((*pWorld) * (*pView)) * (*pProjection);
    m = mA.GetInverted();

    mathVec4Transform((f32*)&out, m.GetData(), (f32*)&in);
    if (out.w == 0.0f)
    {
        return NULL;
    }

    pvObj->x = out.x / out.w;
    pvObj->y = out.y / out.w;
    pvObj->z = out.z / out.w;

    return pvObj;
}


inline Vec3* mathVec3ProjectArray(Vec3* pOut, uint32 OutStride, const Vec3* pV,   uint32 VStride, const int32 pViewport[4],   const Matrix44A* pProjection,   const Matrix44A* pView, const Matrix44A* pWorld, uint32 n, int32)
{
    Matrix44A m;
    Vec4 in, out;

    int8* pOutT = (int8*)pOut;
    int8* pInT = (int8*)pV;

    Vec3* pvWin;
    Vec3* pvObj;

    //FIX: update fViewportMinZ fViewportMaxZ support for Viewport everywhere
    float fViewportMinZ = 0, fViewportMaxZ = 1.0f;

    m =  ((*pWorld) * (*pView)) * (*pProjection);

    for (uint32 i = 0; i < n; i++)
    {
        pvObj = (Vec3*)pInT;
        pvWin = (Vec3*)pOutT;

        in.x = pvObj->x;
        in.y = pvObj->y;
        in.z = pvObj->z;
        in.w = 1.0f;

        mathVec4Transform((f32*)&out, m.GetData(), (f32*)&in);

        if (out.w == 0.0f)
        {
            return NULL;
        }

        float fInvW = 1.0f / out.w;
        out.x *= fInvW;
        out.y *= fInvW;
        out.z *= fInvW;

        //output coords
        pvWin->x = pViewport[0] + (1 + out.x) * pViewport[2] / 2;
        pvWin->y = pViewport[1] + (1 - out.y) * pViewport[3] / 2;  //flip coords for y axis

        pvWin->z = fViewportMinZ + out.z * (fViewportMaxZ - fViewportMinZ);

        pOutT += OutStride;
        pInT += VStride;
    }

    return pOut;
}


inline Vec3* mathVec3UnprojectArray(Vec3* pOut, uint32 OutStride, const Vec3* pV, uint32 VStride, const int32 pViewport[4], const Matrix44* pProjection, const Matrix44* pView, const Matrix44* pWorld, uint32 n, [[maybe_unused]]   int32 OptFlags)
{
    Vec4 in, out;
    Matrix44 m, mA;

    int8* pOutT = (int8*)pOut;
    int8* pInT = (int8*)pV;

    Vec3* pvWin;
    Vec3* pvObj;

    //FIX: update fViewportMinZ fViewportMaxZ support for Viewport everywhere
    float fViewportMinZ = 0, fViewportMaxZ = 1.0f;

    mA =  ((*pWorld) * (*pView)) * (*pProjection);
    m = mA.GetInverted();

    for (uint32 i = 0; i < n; i++)
    {
        pvWin = (Vec3*)pInT;
        pvObj = (Vec3*)pOutT;

        in.x = (pvWin->x - pViewport[0]) * 2 / pViewport[2] - 1.0f;
        in.y = 1.0f - ((pvWin->y - pViewport[1]) * 2 / pViewport[3]);   //flip coords for y axis
        in.z = (pvWin->z - fViewportMinZ) / (fViewportMaxZ - fViewportMinZ);
        in.w = 1.0f;

        mathVec4Transform((f32*)&out, m.GetData(), (f32*)&in);

        assert(out.w != 0.0f);

        if (out.w == 0.0f)
        {
            return NULL;
        }

        pvObj->x = out.x / out.w;
        pvObj->y = out.y / out.w;
        pvObj->z = out.z / out.w;

        pOutT += OutStride;
        pInT += VStride;
    }

    return pOut;
}




/*****************************************************
MISC FUNCTIONS
*****************************************************/

//////////////////////////////////////////////////////////////////////////
#if defined(_CPU_X86)
inline int fastftol_positive(float f)
{
    int i;

    f -= 0.5f;
#if defined(_MSC_VER)
    __asm fld [f]
    __asm fistp [i]
#elif defined(__GNUC__)
    __asm__ ("fld %[f]\n fistpl %[i]" : [i] "+m" (i) : [f] "m" (f));
#else
#error
#endif
    return i;
}
#else
inline int fastftol_positive (float f)
{
    assert(f >= 0.f);
    return (int)floorf(f);
}
#endif



//////////////////////////////////////////////////////////////////////////
#if defined(_CPU_X86)
inline int fastround_positive(float f)
{
    int i;
    assert(f >= 0.f);
#if defined(_MSC_VER)
    __asm fld [f]
    __asm fistp [i]
#elif defined(__GNUC__)
    __asm__ ("fld %[f]\n fistpl %[i]" : [i] "+m" (i) : [f] "m" (f));
#else
#error
#endif
    return i;
}
#else
inline int fastround_positive(float f)
{
    assert(f >= 0.f);
    return (int) (f + 0.5f);
}
#endif



//////////////////////////////////////////////////////////////////////////
#if defined(_CPU_X86)
ILINE int __fastcall FtoI(float  x)
{
    int    t;
#if defined(_MSC_VER)
    __asm
    {
        fld   x
        fistp t
    }
#elif defined(__GNUC__)
    __asm__ ("fld %[x]\n fistpl %[t]" : [t] "+m" (t) : [x] "m" (x));
#else
#error
#endif
    return t;
}
#else
inline int FtoI(float x) { return (int)x; }
#endif

#endif // CRYINCLUDE_CRYCOMMON_CRY_XOPTIMISE_H

