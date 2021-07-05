/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Common matrix class


#ifndef CRYINCLUDE_CRYCOMMON_CRY_MATRIXDIAG_H
#define CRYINCLUDE_CRYCOMMON_CRY_MATRIXDIAG_H
#pragma once


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// struct Diag33_tpl
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename F>
struct Diag33_tpl
{
    F x, y, z;

#ifdef _DEBUG
    ILINE Diag33_tpl()
    {
        if constexpr (sizeof(F) == 4)
        {
            uint32* p = alias_cast<uint32*>(&x);
            p[0] = F32NAN;
            p[1] = F32NAN;
            p[2] = F32NAN;
        }
        if constexpr (sizeof(F) == 8)
        {
            uint64* p = alias_cast<uint64*>(&x);
            p[0] = F64NAN;
            p[1] = F64NAN;
            p[2] = F64NAN;
        }
    }
#else
    ILINE Diag33_tpl()  {};
#endif


    Diag33_tpl(F dx, F dy, F dz) { x = dx; y = dy; z = dz; }
    Diag33_tpl(const Vec3_tpl<F>& v) {  x = v.x; y = v.y; z = v.z;    }
    template<class F1>
    const Diag33_tpl& operator=(const Vec3_tpl<F1>& v) { x = v.x; y = v.y; z = v.z; return *this;    }
    Diag33_tpl& operator=(const Diag33_tpl<F>& diag) { x = diag.x; y = diag.y; z = diag.z; return *this;  }
    template<class F1>
    Diag33_tpl& operator=(const Diag33_tpl<F1>& diag) { x = diag.x; y = diag.y; z = diag.z; return *this; }

    const void SetIdentity() { x = y = z = 1; }
    Diag33_tpl(type_identity) { x = y = z = 1; }

    const Diag33_tpl& zero() {  x = y = z = 0; return *this; }

    Diag33_tpl& fabs() {    x = fabs_tpl(x); y = fabs_tpl(y); z = fabs_tpl(z); return *this;  }

    Diag33_tpl& invert()   // in-place inversion
    {
        F det = determinant();
        if (det == 0)
        {
            return *this;
        }
        det = (F)1.0 / det;
        F oldata[3];
        oldata[0] = x;
        oldata[1] = y;
        oldata[2] = z;
        x = oldata[1] * oldata[2] * det;
        y = oldata[0] * oldata[2] * det;
        z = oldata[0] * oldata[1] * det;
        return *this;
    }

    /*!
    * Linear-Interpolation between Diag33(lerp)
    *
    * Example:
    *  Diag33 r=Diag33::CreateLerp( p, q, 0.345f );
    */
    ILINE void SetLerp(const Diag33_tpl<F>& p, const Diag33_tpl<F>& q, F t)
    {
        x = p.x * (1.0f - t) + q.x * t;
        y = p.y * (1.0f - t) + q.y * t;
        z = p.z * (1.0f - t) + q.z * t;
    }
    ILINE static Diag33_tpl<F> CreateLerp(const Diag33_tpl<F>& p, const Diag33_tpl<F>& q, F t)
    {
        Diag33_tpl<F>  d;
        d.x = p.x * (1.0f - t) + q.x * t;
        d.y = p.y * (1.0f - t) + q.y * t;
        d.z = p.z * (1.0f - t) + q.z * t;
        return d;
    }

    F determinant() const { return x * y * z; }

    ILINE bool IsValid() const
    {
        if (!NumberValid(x))
        {
            return false;
        }
        if (!NumberValid(y))
        {
            return false;
        }
        if (!NumberValid(z))
        {
            return false;
        }
        return true;
    }
};

///////////////////////////////////////////////////////////////////////////////
// Typedefs                                                                  //
///////////////////////////////////////////////////////////////////////////////

typedef Diag33_tpl<f32>  Diag33; //always 32 bit
typedef Diag33_tpl<f64>  Diag33d;//always 64 bit
typedef Diag33_tpl<real> Diag33r;//variable float precision. depending on the target system it can be between 32, 64 or 80 bit


template<class F1, class F2>
Diag33_tpl<F1> operator*(const Diag33_tpl<F1>& l, const Diag33_tpl<F2>& r)
{
    return Diag33_tpl<F1>(l.x * r.x, l.y * r.y,   l.z * r.z);
}

template<class F1, class F2>
Matrix33_tpl<F2> operator*(const Diag33_tpl<F1>& l, const Matrix33_tpl<F2>& r)
{
    Matrix33_tpl<F2> res;
    res.m00 = r.m00 * l.x;
    res.m01 = r.m01 * l.x;
    res.m02 = r.m02 * l.x;
    res.m10 = r.m10 * l.y;
    res.m11 = r.m11 * l.y;
    res.m12 = r.m12 * l.y;
    res.m20 = r.m20 * l.z;
    res.m21 = r.m21 * l.z;
    res.m22 = r.m22 * l.z;
    return res;
}
template<class F1, class F2>
Matrix34_tpl<F2> operator*(const Diag33_tpl<F1>& l, const Matrix34_tpl<F2>& r)
{
    Matrix34_tpl<F2> m;
    m.m00 = l.x * r.m00;
    m.m01 = l.x * r.m01;
    m.m02 = l.x * r.m02;
    m.m03 = l.x * r.m03;
    m.m10 = l.y * r.m10;
    m.m11 = l.y * r.m11;
    m.m12 = l.y * r.m12;
    m.m13 = l.y * r.m13;
    m.m20 = l.z * r.m20;
    m.m21 = l.z * r.m21;
    m.m22 = l.z * r.m22;
    m.m23 = l.z * r.m23;
    return m;
}

template<class F1, class F2>
Vec3_tpl<F2> operator *(const Diag33_tpl<F1>& mtx, const Vec3_tpl<F2>& vec)
{
    return Vec3_tpl<F2>(mtx.x * vec.x, mtx.y * vec.y, mtx.z * vec.z);
}

template<class F1, class F2>
Vec3_tpl<F1> operator *(const Vec3_tpl<F1>& vec, const Diag33_tpl<F2>& mtx)
{
    return Vec3_tpl<F1>(mtx.x * vec.x, mtx.y * vec.y, mtx.z * vec.z);
}



#endif // CRYINCLUDE_CRYCOMMON_CRY_MATRIXDIAG_H

