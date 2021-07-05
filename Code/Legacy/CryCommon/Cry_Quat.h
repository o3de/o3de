/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Common quaternion class


#ifndef CRYINCLUDE_CRYCOMMON_CRY_QUAT_H
#define CRYINCLUDE_CRYCOMMON_CRY_QUAT_H
#pragma once

#include <AzCore/Math/Quaternion.h>

//----------------------------------------------------------------------
// Quaternion
//----------------------------------------------------------------------
template <typename F>
struct Quat_tpl
{
    Vec3_tpl<F> v;
    F w;

    //-------------------------------

    //constructors
#if defined(_DEBUG)
    ILINE Quat_tpl()
    {
        if constexpr (sizeof(F) == 4)
        {
            uint32* p = alias_cast<uint32*>(&v.x);
            p[0] = F32NAN;
            p[1] = F32NAN;
            p[2] = F32NAN;
            p[3] = F32NAN;
        }
        if constexpr (sizeof(F) == 8)
        {
            uint64* p = alias_cast<uint64*>(&v.x);
            p[0] = F64NAN;
            p[1] = F64NAN;
            p[2] = F64NAN;
            p[3] = F64NAN;
        }
    }
#else
    ILINE Quat_tpl() {}
#endif

    //initialize with zeros
    ILINE Quat_tpl(type_zero)
    {
        w = 0, v.x = 0, v.y = 0, v.z = 0;
    }
    ILINE Quat_tpl(type_identity)
    {
        w = 1, v.x = 0, v.y = 0, v.z = 0;
    }

    //ASSIGNMENT OPERATOR of identical Quat types.
    //The assignment operator has precedence over assignment constructor
    //Quat q; q=q0;
    ILINE Quat_tpl<F>& operator = (const Quat_tpl<F>& src)
    {
        v.x = src.v.x;
        v.y = src.v.y;
        v.z = src.v.z;
        w = src.w;
        assert(IsValid());
        return *this;
    }


    //CONSTRUCTOR to initialize a Quat from 4 floats
    //Quat q(1,0,0,0);
    ILINE Quat_tpl<F>(F qw, F qx, F qy, F qz)
    {
        w = qw;
        v.x = qx;
        v.y = qy;
        v.z = qz;
        assert(IsValid());
    }
    //CONSTRUCTOR to initialize a Quat with a scalar and a vector
    //Quat q(1,Vec3(0,0,0));
    ILINE Quat_tpl<F>(F scalar, const Vec3_tpl<F> &vector)
    {
        v = vector;
        w = scalar;
        assert(IsValid());
    };


    //CONSTRUCTOR for identical types
    //Quat q=q0;
    ILINE  Quat_tpl<F>(const Quat_tpl<F>&q)
    {
        w = q.w;
        v.x = q.v.x;
        v.y = q.v.y;
        v.z = q.v.z;
        assert(IsValid());
    }

    //CONSTRUCTOR for AZ::Quaternion
    explicit ILINE  Quat_tpl<F>(const AZ::Quaternion& q)
    {
        w = q.GetW();
        v.x = q.GetX();
        v.y = q.GetY();
        v.z = q.GetZ();
        assert(IsValid());
    }

    //CONSTRUCTOR for identical types which converts between double/float
    //Quat q32=q64;
    //Quatr q64=q32;
    template <class F1>
    ILINE  Quat_tpl<F>(const Quat_tpl<F1>&q)
    {
        assert(q.IsValid());
        w = F(q.w);
        v.x = F(q.v.x);
        v.y = F(q.v.y);
        v.z = F(q.v.z);
    }







    //CONSTRUCTOR for different types. It converts a Euler Angle into a Quat.
    //Needs to be 'explicit' because we loose fp-precision in the conversion process
    //Quat(Ang3(1,2,3));
    explicit ILINE Quat_tpl<F>(const Ang3_tpl<F>&ang)
    {
        assert(ang.IsValid());
        SetRotationXYZ(ang);
    }
    //CONSTRUCTOR for different types. It converts a Euler Angle into a Quat and converts between double/float. .
    //Needs to be 'explicit' because we loose fp-precision in the conversion process
    //Quat(Ang3r(1,2,3));
    template<class F1>
    explicit ILINE Quat_tpl<F>(const Ang3_tpl<F1>&ang)
    {
        assert(ang.IsValid());
        SetRotationXYZ(Ang3_tpl<F>(F(ang.x), F(ang.y), F(ang.z)));
    }


    //CONSTRUCTOR for different types. It converts a Matrix33 into a Quat.
    //Needs to be 'explicit' because we loose fp-precision in the conversion process
    //Quat(m33);
    explicit ILINE Quat_tpl<F>(const Matrix33_tpl<F>&m)
    {
        assert(m.IsOrthonormalRH(0.1f));
        F s, p, tr = m.m00 + m.m11 + m.m22;
        w = 1, v.x = 0, v.y = 0, v.z = 0;
        if (tr > 0)
        {
            s = sqrt_tpl(tr + 1.0f), p = 0.5f / s, w = s * 0.5f, v.x = (m.m21 - m.m12) * p, v.y = (m.m02 - m.m20) * p, v.z = (m.m10 - m.m01) * p;
        }
        else if ((m.m00 >= m.m11) && (m.m00 >= m.m22))
        {
            s = sqrt_tpl(m.m00 - m.m11 - m.m22 + 1.0f), p = 0.5f / s, w = (m.m21 - m.m12) * p, v.x = s * 0.5f, v.y = (m.m10 + m.m01) * p, v.z = (m.m20 + m.m02) * p;
        }
        else if ((m.m11 >= m.m00) && (m.m11 >= m.m22))
        {
            s = sqrt_tpl(m.m11 - m.m22 - m.m00 + 1.0f), p = 0.5f / s, w = (m.m02 - m.m20) * p, v.x = (m.m01 + m.m10) * p, v.y = s * 0.5f, v.z = (m.m21 + m.m12) * p;
        }
        else if ((m.m22 >= m.m00) && (m.m22 >= m.m11))
        {
            s = sqrt_tpl(m.m22 - m.m00 - m.m11 + 1.0f), p = 0.5f / s, w = (m.m10 - m.m01) * p, v.x = (m.m02 + m.m20) * p, v.y = (m.m12 + m.m21) * p, v.z = s * 0.5f;
        }
    }
    //CONSTRUCTOR for different types. It converts a Matrix33 into a Quat and converts between double/float.
    //Needs to be 'explicit' because we loose fp-precision the conversion process
    //Quat(m33r);
    //Quatr(m33);
    template<class F1>
    explicit ILINE Quat_tpl<F>(const Matrix33_tpl<F1>&m)
    {
        assert(m.IsOrthonormalRH(0.1f));
        F1 s, p, tr = m.m00 + m.m11 + m.m22;
        w = 1, v.x = 0, v.y = 0, v.z = 0;
        if (tr > 0)
        {
            s = sqrt_tpl(tr + 1.0f), p = 0.5f / s, w = F(s * 0.5), v.x = F((m.m21 - m.m12) * p), v.y = F((m.m02 - m.m20) * p), v.z = F((m.m10 - m.m01) * p);
        }
        else if ((m.m00 >= m.m11) && (m.m00 >= m.m22))
        {
            s = sqrt_tpl(m.m00 - m.m11 - m.m22 + 1.0f), p = 0.5f / s, w = F((m.m21 - m.m12) * p), v.x = F(s * 0.5), v.y = F((m.m10 + m.m01) * p), v.z = F((m.m20 + m.m02) * p);
        }
        else if ((m.m11 >= m.m00) && (m.m11 >= m.m22))
        {
            s = sqrt_tpl(m.m11 - m.m22 - m.m00 + 1.0f), p = 0.5f / s, w = F((m.m02 - m.m20) * p), v.x = F((m.m01 + m.m10) * p), v.y = F(s * 0.5), v.z = F((m.m21 + m.m12) * p);
        }
        else if ((m.m22 >= m.m00) && (m.m22 >= m.m11))
        {
            s = sqrt_tpl(m.m22 - m.m00 - m.m11 + 1.0f), p = 0.5f / s, w = F((m.m10 - m.m01) * p), v.x = F((m.m02 + m.m20) * p), v.y = F((m.m12 + m.m21) * p), v.z = F(s * 0.5);
        }
    }

    //CONSTRUCTOR for different types. It converts a Matrix34 into a Quat.
    //Needs to be 'explicit' because we loose fp-precision in the conversion process
    //Quat(m34);
    explicit ILINE Quat_tpl<F>(const Matrix34_tpl<F>&m)
    {
        *this = Quat_tpl<F>(Matrix33_tpl<F>(m));
    }
    //CONSTRUCTOR for different types. It converts a Matrix34 into a Quat and converts between double/float.
    //Needs to be 'explicit' because we loose fp-precision the conversion process
    //Quat(m34r);
    //Quatr(m34);
    template<class F1>
    explicit ILINE Quat_tpl<F>(const Matrix34_tpl<F1>&m)
    {
        *this = Quat_tpl<F>(Matrix33_tpl<F1>(m));
    }





    /*!
    * invert quaternion.
    *
    * Example 1:
    *  Quat q=Quat::CreateRotationXYZ(Ang3(1,2,3));
    *  Quat result = !q;
    *  Quat result = GetInverted(q);
    *  q.Invert();
    */
    ILINE Quat_tpl<F> operator ! () const { return Quat_tpl(w, -v); }
    ILINE void Invert(void) { *this = !*this;   }
    ILINE Quat_tpl<F> GetInverted() const { return !(*this); }
    //flip quaternion. don't confuse this with quaternion-inversion.
    ILINE Quat_tpl<F>   operator - () const { return Quat_tpl<F>(-w, -v); };
    //multiplication by a scalar
    void operator *= (F op) {   w *= op; v *= op;   }

    // Exact compare of 2 quats.
    ILINE bool operator==(const Quat_tpl<F>& q) const { return (v == q.v) && (w == q.w); }
    ILINE bool operator!=(const Quat_tpl<F>& q) const { return !(*this == q); }




    //A quaternion is a compressed matrix. Thus there is no problem extracting the rows & columns.
    ILINE Vec3_tpl<F> GetColumn(uint32 i)
    {
        if (i == 0)
        {
            return GetColumn0();
        }
        if (i == 1)
        {
            return GetColumn1();
        }
        if (i == 2)
        {
            return GetColumn2();
        }
        assert(0); //bad index
        return Vec3(ZERO);
    }

    ILINE Vec3_tpl<F> GetColumn0() const {return Vec3_tpl<F>(2 * (v.x * v.x + w * w) - 1, 2 * (v.y * v.x + v.z * w), 2 * (v.z * v.x - v.y * w)); }
    ILINE Vec3_tpl<F> GetColumn1() const {return Vec3_tpl<F>(2 * (v.x * v.y - v.z * w), 2 * (v.y * v.y + w * w) - 1, 2 * (v.z * v.y + v.x * w)); }
    ILINE Vec3_tpl<F> GetColumn2() const {return Vec3_tpl<F>(2 * (v.x * v.z + v.y * w), 2 * (v.y * v.z - v.x * w), 2 * (v.z * v.z + w * w) - 1); }
    ILINE Vec3_tpl<F> GetRow0() const {return Vec3_tpl<F>(2 * (v.x * v.x + w * w) - 1, 2 * (v.x * v.y - v.z * w), 2 * (v.x * v.z + v.y * w)); }
    ILINE Vec3_tpl<F> GetRow1() const {return Vec3_tpl<F>(2 * (v.y * v.x + v.z * w), 2 * (v.y * v.y + w * w) - 1, 2 * (v.y * v.z - v.x * w)); }
    ILINE Vec3_tpl<F> GetRow2() const   {return Vec3_tpl<F>(2 * (v.z * v.x - v.y * w), 2 * (v.z * v.y + v.x * w), 2 * (v.z * v.z + w * w) - 1); }

    // These are just copy & pasted components of the GetColumn1() above.
    ILINE F GetFwdX() const { return (2 * (v.x * v.y - v.z * w)); }
    ILINE F GetFwdY() const { return (2 * (v.y * v.y + w * w) - 1); }
    ILINE F GetFwdZ() const { return (2 * (v.z * v.y + v.x * w)); }
    ILINE F GetRotZ() const { return atan2_tpl(-GetFwdX(), GetFwdY()); }



    /*!
    * set identity quaternion
    *
    * Example:
    *       Quat q=Quat::CreateIdentity();
    *   or
    *       q.SetIdentity();
    *   or
    *       Quat p=Quat(IDENTITY);
    */
    ILINE void SetIdentity(void)
    {
        w = 1;
        v.x = 0;
        v.y = 0;
        v.z = 0;
    }
    ILINE static Quat_tpl<F> CreateIdentity(void)
    {
        return Quat_tpl<F>(1, 0, 0, 0);
    }




    // Description:
    //    Check if identity quaternion.
    ILINE bool IsIdentity() const
    {
        return w == 1 && v.x == 0 && v.y == 0 && v.z == 0;
    }

    ILINE bool IsUnit(F e = VEC_EPSILON) const
    {
        return fabs_tpl(1 - (w * w + v.x * v.x + v.y * v.y + v.z * v.z)) < e;
    }

    ILINE bool IsValid([[maybe_unused]] F e = VEC_EPSILON) const
    {
        if (!v.IsValid())
        {
            return false;
        }
        if (!NumberValid(w))
        {
            return false;
        }
        //if (!IsUnit(e))   return false;
        return true;
    }



    ILINE void SetRotationAA(F rad, const Vec3_tpl<F>& axis)
    {
        F s, c;
        sincos_tpl(rad * (F)0.5, &s, &c);
        SetRotationAA(c, s, axis);
    }
    ILINE static Quat_tpl<F> CreateRotationAA(F rad, const Vec3_tpl<F>& axis)
    {
        Quat_tpl<F> q;
        q.SetRotationAA(rad, axis);
        return q;
    }

    ILINE void SetRotationAA(F cosha, F sinha, const Vec3_tpl<F>& axis)
    {
        assert(axis.IsUnit(0.001f));
        w = cosha;
        v = axis * sinha;
    }
    ILINE static Quat_tpl<F> CreateRotationAA(F cosha, F sinha, const Vec3_tpl<F>& axis)
    {
        Quat_tpl<F> q;
        q.SetRotationAA(cosha, sinha, axis);
        return q;
    }




    /*!
    * Create rotation-quaternion that around the fixed coordinate axes.
    *
    * Example:
    *       Quat q=Quat::CreateRotationXYZ( Ang3(1,2,3) );
    *   or
    *       q.SetRotationXYZ( Ang3(1,2,3) );
    */
    ILINE void SetRotationXYZ(const Ang3_tpl<F>& a)
    {
        assert(a.IsValid());
        F sx, cx;
        sincos_tpl(F(a.x * F(0.5)), &sx, &cx);
        F sy, cy;
        sincos_tpl(F(a.y * F(0.5)), &sy, &cy);
        F sz, cz;
        sincos_tpl(F(a.z * F(0.5)), &sz, &cz);
        w       = cx * cy * cz + sx * sy * sz;
        v.x = cz * cy * sx - sz * sy * cx;
        v.y = cz * sy * cx + sz * cy * sx;
        v.z = sz * cy * cx - cz * sy * sx;
    }
    ILINE static Quat_tpl<F> CreateRotationXYZ(const Ang3_tpl<F>& a)
    {
        assert(a.IsValid());
        Quat_tpl<F> q;
        q.SetRotationXYZ(a);
        return q;
    }

    /*!
    * Create rotation-quaternion that about the x-axis.
    *
    * Example:
    *       Quat q=Quat::CreateRotationX( radiant );
    *   or
    *       q.SetRotationX( Ang3(1,2,3) );
    */
    ILINE void SetRotationX(f32 r)
    {
        F s, c;
        sincos_tpl(F(r * F(0.5)), &s, &c);
        w = c;
        v.x = s;
        v.y = 0;
        v.z = 0;
    }
    ILINE static Quat_tpl<F> CreateRotationX(f32 r)
    {
        Quat_tpl<F> q;
        q.SetRotationX(r);
        return q;
    }


    /*!
    * Create rotation-quaternion that about the y-axis.
    *
    * Example:
    *       Quat q=Quat::CreateRotationY( radiant );
    *   or
    *       q.SetRotationY( radiant );
    */
    ILINE void SetRotationY(f32 r)
    {
        F s, c;
        sincos_tpl(F(r * F(0.5)), &s, &c);
        w = c;
        v.x = 0;
        v.y = s;
        v.z = 0;
    }
    ILINE static Quat_tpl<F> CreateRotationY(f32 r)
    {
        Quat_tpl<F> q;
        q.SetRotationY(r);
        return q;
    }


    /*!
    * Create rotation-quaternion that about the z-axis.
    *
    * Example:
    *       Quat q=Quat::CreateRotationZ( radiant );
    *   or
    *       q.SetRotationZ( radiant );
    */
    ILINE void SetRotationZ(f32 r)
    {
        F s, c;
        sincos_tpl(F(r * F(0.5)), &s, &c);
        w = c;
        v.x = 0;
        v.y = 0;
        v.z = s;
    }
    ILINE static Quat_tpl<F> CreateRotationZ(f32 r)
    {
        Quat_tpl<F> q;
        q.SetRotationZ(r);
        return q;
    }



    /*!
    *
    * Create rotation-quaternion that rotates from one vector to another.
    * Both vectors are assumed to be normalized.
    *
    * Example:
    *       Quat q=Quat::CreateRotationV0V1( v0,v1 );
    *       q.SetRotationV0V1( v0,v1 );
    */
    ILINE void SetRotationV0V1(const Vec3_tpl<F>& v0, const Vec3_tpl<F>& v1)
    {
        assert(v0.IsUnit(0.01f));
        assert(v1.IsUnit(0.01f));
        F dot = v0.x * v1.x + v0.y * v1.y + v0.z * v1.z + F(1.0);
        if (dot > F(0.0001))
        {
            F vx = v0.y * v1.z - v0.z * v1.y;
            F vy = v0.z * v1.x - v0.x * v1.z;
            F vz = v0.x * v1.y - v0.y * v1.x;
            F d = isqrt_tpl(dot * dot + vx * vx + vy * vy + vz * vz);
            w = F(dot * d);
            v.x = F(vx * d);
            v.y = F(vy * d);
            v.z = F(vz * d);
            return;
        }
        w = 0;
        v = v0.GetOrthogonal().GetNormalized();
    }
    ILINE static Quat_tpl<F> CreateRotationV0V1(const Vec3_tpl<F>& v0, const Vec3_tpl<F>& v1) { Quat_tpl<F> q;  q.SetRotationV0V1(v0, v1);   return q;   }






    /*!
    *
    * \param vdir  normalized view direction.
    * \param roll  radiant to rotate about Y-axis.
    *
    *  Given a view-direction and a radiant to rotate about Y-axis, this function builds a 3x3 look-at quaternion
    *  using only simple vector arithmetic. This function is always using the implicit up-vector Vec3(0,0,1).
    *  The view-direction is always stored in column(1).
    *  IMPORTANT: The view-vector is assumed to be normalized, because all trig-values for the orientation are being
    *  extracted  directly out of the vector. This function must NOT be called with a view-direction
    *  that is close to Vec3(0,0,1) or Vec3(0,0,-1). If one of these rules is broken, the function returns a quaternion
    *  with an undefined rotation about the Z-axis.
    *
    *  Rotation order for the look-at-quaternion is Z-X-Y. (Zaxis=YAW / Xaxis=PITCH / Yaxis=ROLL)
    *
    *  COORDINATE-SYSTEM
    *
    *  z-axis
    *    ^
    *    |
    *    |  y-axis
    *    |  /
    *    | /
    *    |/
    *    +--------------->   x-axis
    *
    *  Example:
    *       Quat LookAtQuat=Quat::CreateRotationVDir( Vec3(0,1,0) );
    *   or
    *       Quat LookAtQuat=Quat::CreateRotationVDir( Vec3(0,1,0), 0.333f );
    */
    ILINE void SetRotationVDir(const Vec3_tpl<F>& vdir)
    {
        assert(vdir.IsUnit(0.01f));
        //set default initialization for up-vector
        w = F(0.70710676908493042);
        v.x = F(vdir.z * 0.70710676908493042);
        v.y = F(0.0);
        v.z = F(0.0);
        F l = sqrt_tpl(vdir.x * vdir.x + vdir.y * vdir.y);
        if (l > F(0.00001))
        {
            //calculate LookAt quaternion
            Vec3_tpl<F> hv  =   Vec3_tpl<F> (vdir.x / l, vdir.y / l + 1.0f, l + 1.0f);
            F r = sqrt_tpl(hv.x * hv.x + hv.y * hv.y);
            F s = sqrt_tpl(hv.z * hv.z + vdir.z * vdir.z);
            //generate the half-angle sine&cosine
            F hacos0 = 0.0;
            F hasin0 = -1.0;
            if (r > F(0.00001))
            {
                hacos0 = hv.y / r;
                hasin0 = -hv.x / r;
            }                                                       //yaw
            F hacos1 = hv.z / s;
            F hasin1 = vdir.z / s;                                  //pitch
            w = F(hacos0 * hacos1);
            v.x = F(hacos0 * hasin1);
            v.y = F(hasin0 * hasin1);
            v.z = F(hasin0 * hacos1);
        }
    }
    ILINE static Quat_tpl<F> CreateRotationVDir(const Vec3_tpl<F>& vdir) {    Quat_tpl<F> q;  q.SetRotationVDir(vdir); return q;  }


    ILINE void SetRotationVDir(const Vec3_tpl<F>& vdir, F r)
    {
        SetRotationVDir(vdir);
        F sy, cy;
        sincos_tpl(r * (F)0.5, &sy, &cy);
        F vx = v.x, vy = v.y;
        v.x = F(vx * cy - v.z * sy);
        v.y = F(w * sy + vy * cy);
        v.z = F(v.z * cy + vx * sy);
        w = F(w * cy - vy * sy);
    }
    ILINE static Quat_tpl<F> CreateRotationVDir(const Vec3_tpl<F>& vdir, F roll) {    Quat_tpl<F> q; q.SetRotationVDir(vdir, roll);    return q;   }




    /*!
    * normalize quaternion.
    *
    * Example 1:
    *  Quat q; q.Normalize();
    *
    * Example 2:
    *  Quat q=Quat(1,2,3,4);
    *  Quat qn=q.GetNormalized();
    */
    ILINE void Normalize(void)
    {
        F d = isqrt_tpl(w * w + v.x * v.x + v.y * v.y + v.z * v.z);
        w *= d;
        v.x *= d;
        v.y *= d;
        v.z *= d;
    }
    ILINE Quat_tpl<F> GetNormalized() const
    {
        Quat_tpl<F> t = *this;
        t.Normalize();
        return t;
    }


    ILINE void NormalizeSafe(void)
    {
        F d = w * w + v.x * v.x + v.y * v.y + v.z * v.z;
        if (d > 1e-8f)
        {
            d = isqrt_tpl(d);
            w *= d;
            v.x *= d;
            v.y *= d;
            v.z *= d;
        }
        else
        {
            SetIdentity();
        }
    }
    ILINE Quat_tpl<F> GetNormalizedSafe() const
    {
        Quat_tpl<F> t = *this;
        t.NormalizeSafe();
        return t;
    }


    ILINE void  NormalizeFast(void)
    {
        assert(this->IsValid());
        F fInvLen = isqrt_fast_tpl(v.x * v.x + v.y * v.y + v.z * v.z + w * w);
        v.x *= fInvLen;
        v.y *= fInvLen;
        v.z *= fInvLen;
        w *= fInvLen;
    }
    ILINE Quat_tpl<F> GetNormalizedFast() const
    {
        Quat_tpl<F> t = *this;
        t.NormalizeFast();
        return t;
    }


    /*!
    * get length of quaternion.
    *
    * Example 1:
    *  f32 l=q.GetLength();
    */
    ILINE F GetLength() const
    {
        return sqrt_tpl(w * w + v.x * v.x + v.y * v.y + v.z * v.z);
    }

    ILINE static bool IsEquivalent(const Quat_tpl<F>& q1, const Quat_tpl<F>& q2, F qe = RAD_EPSILON)
    {
        Quat_tpl<f64> q1r = q1;
        Quat_tpl<f64> q2r = q2;
        f64 rad = acos(min(1.0, fabs_tpl(q1r.v.x * q2r.v.x + q1r.v.y * q2r.v.y + q1r.v.z * q2r.v.z + q1r.w * q2r.w)));
        bool qdif = rad <= qe;
        return qdif;
    }


    // Exponent of Quaternion.
    ILINE static Quat_tpl<F> exp(const Vec3_tpl<F>& v)
    {
        F lensqr = v.len2();
        if (lensqr > F(0))
        {
            F len = sqrt_tpl(lensqr);
            F s, c;
            sincos_tpl(len, &s, &c);
            s /= len;
            return Quat_tpl<F>(c, v.x * s, v.y * s, v.z * s);
        }
        return Quat_tpl<F> (IDENTITY);
    }

    // logarithm of a quaternion, imaginary part (the real part of the logarithm is always 0)
    ILINE static Vec3_tpl<F> log (const Quat_tpl<F>& q)
    {
        assert(q.IsValid());
        F lensqr = q.v.len2();
        if (lensqr > 0.0f)
        {
            F len = sqrt_tpl(lensqr);
            F angle = atan2_tpl(len, q.w) / len;
            return q.v * angle;
        }
        // logarithm of a quaternion, imaginary part (the real part of the logarithm is always 0)
        return Vec3_tpl<F>(0, 0, 0);
    }


    //////////////////////////////////////////////////////////////////////////
    //! Logarithm of Quaternion difference.
    ILINE static Quat_tpl<F> LnDif(const Quat_tpl<F>& q1, const Quat_tpl<F>& q2)
    {
        return Quat_tpl<F>(0, log(q2 / q1));
    }




    /*!
    * linear-interpolation between quaternions (lerp)
    *
    * Example:
    *  CQuaternion result,p,q;
    *  result=qlerp( p, q, 0.5f );
    */
    ILINE void SetNlerp(const Quat_tpl<F>& p, const Quat_tpl<F>& tq, F t)
    {
        Quat_tpl<F> q = tq;
        assert(p.IsValid());
        assert(q.IsValid());
        if ((p | q) < 0)
        {
            q = -q;
        }
        v.x = p.v.x * (1.0f - t) + q.v.x * t;
        v.y = p.v.y * (1.0f - t) + q.v.y * t;
        v.z = p.v.z * (1.0f - t) + q.v.z * t;
        w       = p.w  * (1.0f - t) + q.w  * t;
        Normalize();
    }

    ILINE static Quat_tpl<F> CreateNlerp(const Quat_tpl<F>& p, const Quat_tpl<F>& tq, F t)
    {
        Quat_tpl<F> d;
        d.SetNlerp(p, tq, t);
        return d;
    }


    /*!
    * linear-interpolation between quaternions (nlerp)
    * in this case we convert the t-value into a 1d cubic spline to get closer to Slerp
    *
    * Example:
    *  Quat result,p,q;
    *  result.SetNlerpCubic( p, q, 0.5f );
    */
    ILINE void SetNlerpCubic(const Quat_tpl<F>& p, const Quat_tpl<F>& tq, F t)
    {
        Quat_tpl<F> q = tq;
        assert((fabs_tpl(1 - (p | p))) < 0.001); //check if unit-quaternion
        assert((fabs_tpl(1 - (q | q))) < 0.001); //check if unit-quaternion
        F cosine = (p | q);
        if (cosine < 0)
        {
            q = -q;
        }
        F k = (1 - fabs_tpl(cosine)) * F(0.4669269);
        F s = 2 * k * t * t * t - 3 * k * t * t + (1 + k) * t;
        v.x = p.v.x * (1.0f - s) + q.v.x * s;
        v.y = p.v.y * (1.0f - s) + q.v.y * s;
        v.z = p.v.z * (1.0f - s) + q.v.z * s;
        w       = p.w  * (1.0f - s) + q.w * s;
        Normalize();
    }
    ILINE static Quat_tpl<F> CreateNlerpCubic(const Quat_tpl<F>& p, const Quat_tpl<F>& tq, F t)
    {
        Quat_tpl<F> d;
        d.SetNlerpCubic(p, tq, t);
        return d;
    }


    /*!
    * spherical-interpolation between quaternions (geometrical slerp)
    *
    * Example:
    *  Quat result,p,q;
    *  result.SetSlerp( p, q, 0.5f );
    */
    ILINE void SetSlerp(const Quat_tpl<F>& tp, const Quat_tpl<F>& tq, F t)
    {
        assert(tp.IsValid());
        assert(tq.IsUnit());
        Quat_tpl<F> p = tp, q = tq;
        Quat_tpl<F> q2;

        F cosine = (p | q);
        if (cosine < 0.0f)
        {
            cosine = -cosine;
            q = -q;
        }                                             //take shortest arc
        if (cosine > 0.9999f)
        {
            SetNlerp(p, q, t);
            return;
        }
        // from now on, a division by 0 is not possible any more
        q2.w        = q.w - p.w * cosine;
        q2.v.x  = q.v.x - p.v.x * cosine;
        q2.v.y  = q.v.y - p.v.y * cosine;
        q2.v.z  = q.v.z - p.v.z * cosine;
        F sine  = sqrt(q2 | q2);

        // Actually you can divide by 0 right here.
        assert(sine != 0.0000f);

        F s, c;
        sincos_tpl(atan2_tpl(sine, cosine) * t, &s, &c);
        w =   F(p.w  * c + q2.w  * s / sine);
        v.x = F(p.v.x * c + q2.v.x * s / sine);
        v.y = F(p.v.y * c + q2.v.y * s / sine);
        v.z = F(p.v.z * c + q2.v.z * s / sine);
    }

    ILINE static Quat_tpl<F> CreateSlerp(const Quat_tpl<F>& p, const Quat_tpl<F>& tq, F t)
    {
        Quat_tpl<F> d;
        d.SetSlerp(p, tq, t);
        return d;
    }

    /*!
    * spherical-interpolation between quaternions (algebraic slerp_a)
    * I have included this function just for the sake of completeness, because
    * its the only useful application to check if exp & log really work.
    * Both slerp-functions are returning the same result.
    *
    * Example:
    *  Quat result,p,q;
    *  result.SetExpSlerp( p,q,0.3345f );
    */
    ILINE void SetExpSlerp(const Quat_tpl<F>& p, const Quat_tpl<F>& tq, F t)
    {
        assert((fabs_tpl(1 - (p | p))) < 0.001); //check if unit-quaternion
        assert((fabs_tpl(1 - (tq | tq))) < 0.001); //check if unit-quaternion
        Quat_tpl<F> q = tq;
        if ((p | q) < 0)
        {
            q = -q;
        }
        *this = p * exp(log(!p * q) * t);                     //algebraic slerp (1)
        //...and some more exp-slerp-functions producing all the same result
        //*this = exp( log (p* !q) * (1-t)) * q;    //algebraic slerp (2)
        //*this = exp( log (q* !p) * t) * p;            //algebraic slerp (3)
        //*this = q * exp( log (!q*p) * (1-t));     //algebraic slerp (4)
    }
    ILINE static Quat_tpl<F> CreateExpSlerp(const Quat_tpl<F>& p, const Quat_tpl<F>& q, F t)
    {
        Quat_tpl<F> d;
        d.SetExpSlerp(p, q, t);
        return d;
    }


    //! squad(p,a,b,q,t) = slerp( slerp(p,q,t),slerp(a,b,t), 2(1-t)t).
    ILINE void SetSquad(const Quat_tpl<F>& p, const Quat_tpl<F>& a, const Quat_tpl<F>& b, const Quat_tpl<F>& q, F t)
    {
        SetSlerp(CreateSlerp(p, q, t), CreateSlerp(a, b, t), 2.0f * (1.0f - t) * t);
    }

    ILINE static Quat_tpl<F> CreateSquad(const Quat_tpl<F>& p, const Quat_tpl<F>& a, const Quat_tpl<F>& b, const Quat_tpl<F>& q, F t)
    {
        Quat_tpl<F> d;
        d.SetSquad(p, a, b, q, t);
        return d;
    }


    //useless, please delete
    ILINE Quat_tpl<F> GetScaled(F scale) const
    {
        return CreateNlerp(IDENTITY, *this, scale);
    }


    AUTO_STRUCT_INFO
};


///////////////////////////////////////////////////////////////////////////////
// Typedefs                                                                  //
///////////////////////////////////////////////////////////////////////////////

#ifndef MAX_API_NUM
typedef Quat_tpl<f32>  Quat;  //always 32 bit
typedef Quat_tpl<f64>  Quatd; //always 64 bit
typedef Quat_tpl<real> Quatr; //variable float precision. depending on the target system it can be between 32, 64 or 80 bit
#endif

typedef Quat_tpl<f32>   CryQuat;
typedef Quat_tpl<f32> quaternionf;
typedef Quat_tpl<real>  quaternion;

// alligned versions
#ifndef MAX_API_NUM
typedef DEFINE_ALIGNED_DATA (Quat, QuatA, 16);               // typedef __declspec(align(16)) Quat_tpl<f32>     CryQuatA;
typedef DEFINE_ALIGNED_DATA (Quatd, QuatrA, 32); // typedef __declspec(align(16)) Quat_tpl<f32>     quaternionfA;
#endif



/*!
*
* The "inner product" or "dot product" operation.
*
* calculate the "inner product" between two Quaternion.
* If both Quaternion are unit-quaternions, the result is the cosine: p*q=cos(angle)
*
* Example:
*  Quat p(1,0,0,0),q(1,0,0,0);
*  f32 cosine = ( p | q );
*
*/
template<typename F1, typename F2>
ILINE F1 operator | (const Quat_tpl<F1>& q, const Quat_tpl<F2>& p)
{
    assert(q.v.IsValid());
    assert(p.v.IsValid());
    return (q.v.x * p.v.x + q.v.y * p.v.y + q.v.z * p.v.z + q.w * p.w);
}


/*!
*
*  Implements the multiplication operator: Qua=QuatA*QuatB
*
*  AxB = operation B followed by operation A.
*  A multiplication takes 16 muls and 12 adds.
*
*  Example 1:
*   Quat p(1,0,0,0),q(1,0,0,0);
*   Quat result=p*q;
*
*  Example 2: (self-multiplication)
*   Quat p(1,0,0,0),q(1,0,0,0);
*   Quat p*=q;
*/
template<class F1, class F2>
Quat_tpl<F1> ILINE operator * (const Quat_tpl<F1>& q, const Quat_tpl<F2>& p)
{
    assert(q.IsValid());
    assert(p.IsValid());
    return Quat_tpl<F1>
           (
        q.w * p.w  - (q.v.x * p.v.x + q.v.y * p.v.y + q.v.z * p.v.z),
        q.v.y * p.v.z - q.v.z * p.v.y + q.w * p.v.x + q.v.x * p.w,
        q.v.z * p.v.x - q.v.x * p.v.z + q.w * p.v.y + q.v.y * p.w,
        q.v.x * p.v.y - q.v.y * p.v.x + q.w * p.v.z + q.v.z * p.w
           );
}
template<class F1, class F2>
ILINE void operator *= (Quat_tpl<F1>& q, const Quat_tpl<F2>& p)
{
    assert(q.IsValid());
    assert(p.IsValid());
    F1 s0 = q.w;
    q.w = q.w * p.w - (q.v | p.v);
    q.v = p.v * s0 + q.v * p.w + (q.v % p.v);
}

/*!
*  Implements the multiplication operator: QuatT=Quat*Quatpos
*
*  AxB = operation B followed by operation A.
*  A multiplication takes 31 muls and 27 adds (=58 float operations).
*
*  Example:
*   Quat    quat        =   Quat::CreateRotationX(1.94192f);;
*   QuatT quatpos   =   QuatT::CreateRotationZ(3.14192f,Vec3(11,22,33));
*     QuatT qp          =   quat*quatpos;
*/
template<class F1, class F2>
ILINE QuatT_tpl<F1> operator * (const Quat_tpl<F1>& q, const QuatT_tpl<F2>& p)
{
    return QuatT_tpl<F1>(q * p.q, q * p.t);
}



/*!
*  division operator
*
*  Example 1:
*   Quat p(1,0,0,0),q(1,0,0,0);
*   Quat result=p/q;
*
*  Example 2: (self-division)
*   Quat p(1,0,0,0),q(1,0,0,0);
*   Quat p/=q;
*/
template<class F1, class F2>
ILINE Quat_tpl<F1> operator / (const Quat_tpl<F1>& q, const Quat_tpl<F2>& p)
{
    return (!p * q);
}
template<class F1, class F2>
ILINE void operator /= (Quat_tpl<F1>& q, const Quat_tpl<F2>& p)
{
    q = (!p * q);
}


/*!
*  addition operator
*
*  Example:
*   Quat p(1,0,0,0),q(1,0,0,0);
*   Quat result=p+q;
*
*  Example:(self-addition operator)
*   Quat p(1,0,0,0),q(1,0,0,0);
*   Quat p-=q;
*/
template<class F1, class F2>
ILINE Quat_tpl<F1> operator + (const Quat_tpl<F1>& q, const Quat_tpl<F2>& p)
{
    return Quat_tpl<F1>(q.w + p.w, q.v + p.v);
}
template<class F1, class F2>
ILINE void operator += (Quat_tpl<F1>& q, const Quat_tpl<F2>& p)
{
    q.w += p.w;
    q.v += p.v;
}



/*!
*  subtraction operator
*
*  Example:
*   Quat p(1,0,0,0),q(1,0,0,0);
*   Quat result=p-q;
*
*  Example: (self-subtraction operator)
*   Quat p(1,0,0,0),q(1,0,0,0);
*   Quat p-=q;
*
*/
template<class F1, class F2>
ILINE Quat_tpl<F1> operator - (const Quat_tpl<F1>& q, const Quat_tpl<F2>& p)
{
    return Quat_tpl<F1>(q.w - p.w, q.v - p.v);
}
template<class F1, class F2>
ILINE void operator -= (Quat_tpl<F1>& q, const Quat_tpl<F2>& p)
{
    q.w -= p.w;
    q.v -= p.v;
}


//! Scale quaternion free function.
template <typename F>
ILINE Quat_tpl<F> operator * (F t, const Quat_tpl<F>& q)
{
    return Quat_tpl<F>(t * q.w, t * q.v);
};


template <typename F1, typename F2>
ILINE Quat_tpl<F1>   operator * (const Quat_tpl<F1>& q, F2 t)
{
    return Quat_tpl<F1>(q.w * t, q.v * t);
};
template <typename F1, typename F2>
ILINE Quat_tpl<F1>   operator / (const Quat_tpl<F1>& q, F2 t)
{
    return Quat_tpl<F1>(q.w / t, q.v / t);
};


/*!
*
* post-multiply of a quaternion and a Vec3 (3D rotations with quaternions)
*
* Example:
*  Quat q(1,0,0,0);
*  Vec3 v(33,44,55);
*    Vec3 result = q*v;
*/
template<class F, class F2>
ILINE Vec3_tpl<F> operator * (const Quat_tpl<F>& q, const Vec3_tpl<F2>& v)
{
    assert(v.IsValid());
    assert(q.IsValid());
    //muls=15 / adds=15
    Vec3_tpl<F> out, r2;
    r2.x = (q.v.y * v.z - q.v.z * v.y) + q.w * v.x;
    r2.y = (q.v.z * v.x - q.v.x * v.z) + q.w * v.y;
    r2.z = (q.v.x * v.y - q.v.y * v.x) + q.w * v.z;
    out.x = (r2.z * q.v.y - r2.y * q.v.z);
    out.x += out.x + v.x;
    out.y = (r2.x * q.v.z - r2.z * q.v.x);
    out.y += out.y + v.y;
    out.z = (r2.y * q.v.x - r2.x * q.v.y);
    out.z += out.z + v.z;
    return out;
}


/*!
* pre-multiply of a quaternion and a Vec3 (3D rotations with quaternions)
*
* Example:
*  Quat q(1,0,0,0);
*  Vec3 v(33,44,55);
*    Vec3 result = v*q;
*/
template<class F, class F2>
ILINE Vec3_tpl<F2> operator * (const Vec3_tpl<F>& v, const Quat_tpl<F2>& q)
{
    assert(v.IsValid());
    assert(q.IsValid());
    //muls=15 / adds=15
    Vec3_tpl<F> out, r2;
    r2.x = (q.v.z * v.y - q.v.y * v.z) + q.w * v.x;
    r2.y = (q.v.x * v.z - q.v.z * v.x) + q.w * v.y;
    r2.z = (q.v.y * v.x - q.v.x * v.y) + q.w * v.z;
    out.x = (r2.y * q.v.z - r2.z * q.v.y);
    out.x += out.x + v.x;
    out.y = (r2.z * q.v.x - r2.x * q.v.z);
    out.y += out.y + v.y;
    out.z = (r2.x * q.v.y - r2.y * q.v.x);
    out.z += out.z + v.z;
    return out;
}



template<class F1, class F2>
ILINE Quat_tpl<F1> operator % (const Quat_tpl<F1>& q, const Quat_tpl<F2>& tp)
{
    Quat_tpl<F1> p = tp;
    if ((p | q) < 0)
    {
        p = -p;
    }
    return Quat_tpl<F1>(q.w + p.w, q.v + p.v);
}
template<class F1, class F2>
ILINE void operator %= (Quat_tpl<F1>& q, const Quat_tpl<F2>& tp)
{
    Quat_tpl<F1> p = tp;
    if ((p | q) < 0)
    {
        p = -p;
    }
    q = Quat_tpl<F1>(q.w + p.w, q.v + p.v);
}


































//----------------------------------------------------------------------
// Quaternion with translation vector
//----------------------------------------------------------------------
template <typename F>
struct QuatT_tpl
{
    Quat_tpl<F> q; //this is the quaternion
    Vec3_tpl<F> t; //this is the translation vector and a scalar (for uniform scaling?)

    ILINE QuatT_tpl(){}

    //initialize with zeros
    ILINE QuatT_tpl(type_zero)
    {
        q.w = 0, q.v.x = 0, q.v.y = 0, q.v.z = 0, t.x = 0, t.y = 0, t.z = 0;
    }
    ILINE QuatT_tpl(type_identity)
    {
        q.w = 1, q.v.x = 0, q.v.y = 0, q.v.z = 0, t.x = 0, t.y = 0, t.z = 0;
    }

    ILINE QuatT_tpl(const Vec3_tpl<F>& _t, const Quat_tpl<F>& _q) { q = _q; t = _t; }

    //CONSTRUCTOR: implement the copy/casting/assignment constructor:
    template <typename F1>
    ILINE QuatT_tpl(const QuatT_tpl<F1>& qt)
        : q(qt.q)
        , t(qt.t) {}

    //convert unit DualQuat back to QuatT
    ILINE QuatT_tpl(const DualQuat_tpl<F>& qd)
    {
        //copy quaternion part
        q = qd.nq;
        //convert translation vector:
        t = (qd.nq.w * qd.dq.v - qd.dq.w * qd.nq.v + qd.nq.v % qd.dq.v) * 2; //perfect for HLSL
    }

    explicit ILINE QuatT_tpl(const QuatTS_tpl<F>& qts)
        : q(qts.q)
        , t(qts.t) {}

    explicit ILINE QuatT_tpl(const Matrix34_tpl<F>& m)
    {
        q = Quat_tpl<F>(Matrix33(m));
        t = m.GetTranslation();
    }

    ILINE QuatT_tpl(const Quat_tpl<F>& quat, const Vec3_tpl<F>& trans)
    {
        q = quat;
        t = trans;
    }

    ILINE void SetIdentity()
    {
        q.w = 1;
        q.v.x = 0;
        q.v.y = 0;
        q.v.z = 0;
        t.x = 0;
        t.y = 0;
        t.z = 0;
    }

    ILINE bool IsIdentity() const
    {
        return (q.IsIdentity() && t.IsZero());
    }

    /*!
    * Convert three Euler angle to mat33 (rotation order:XYZ)
    * The Euler angles are assumed to be in radians.
    * The translation-vector is set to zero by default.
    *
    *  Example 1:
    *       QuatT qp;
    *       qp.SetRotationXYZ( Ang3(0.5f,0.2f,0.9f), translation );
    *
    *  Example 2:
    *       QuatT qp=QuatT::CreateRotationXYZ( Ang3(0.5f,0.2f,0.9f), translation );
    */
    ILINE void SetRotationXYZ(const Ang3_tpl<F>& rad, const Vec3_tpl<F>& trans = Vec3(ZERO))
    {
        assert(rad.IsValid());
        assert(trans.IsValid());
        q.SetRotationXYZ(rad);
        t = trans;
    }
    ILINE static QuatT_tpl<F> CreateRotationXYZ(const Ang3_tpl<F>& rad, const Vec3_tpl<F>& trans = Vec3(ZERO))
    {
        assert(rad.IsValid());
        assert(trans.IsValid());
        QuatT_tpl<F> qp;
        qp.SetRotationXYZ(rad, trans);
        return qp;
    }



    ILINE void SetRotationAA(F cosha, F sinha, const Vec3_tpl<F> axis, const Vec3_tpl<F>& trans = Vec3(ZERO))
    {
        q.SetRotationAA(cosha, sinha, axis);
        t = trans;
    }
    ILINE static QuatT_tpl<F> CreateRotationAA(F cosha, F sinha, const Vec3_tpl<F> axis, const Vec3_tpl<F>& trans = Vec3(ZERO))
    {
        QuatT_tpl<F> qt;
        qt.SetRotationAA(cosha, sinha, axis, trans);
        return qt;
    }


    ILINE void Invert()
    { // in-place transposition
        assert(q.IsValid());
        t = -t * q;
        q = !q;
    }
    ILINE QuatT_tpl<F> GetInverted() const
    {
        assert(q.IsValid());
        QuatT_tpl<F> qpos;
        qpos.q = !q;
        qpos.t = -t * q;
        return qpos;
    }

    ILINE void SetTranslation(const Vec3_tpl<F>& trans)   {   t = trans;    }

    ILINE Vec3_tpl<F> GetColumn0() const {return q.GetColumn0(); }
    ILINE Vec3_tpl<F> GetColumn1() const {return q.GetColumn1(); }
    ILINE Vec3_tpl<F> GetColumn2() const {return q.GetColumn2(); }
    ILINE Vec3_tpl<F> GetColumn3() const {return t; }
    ILINE Vec3_tpl<F> GetRow0() const { return q.GetRow0(); }
    ILINE Vec3_tpl<F> GetRow1() const { return q.GetRow1(); }
    ILINE Vec3_tpl<F> GetRow2() const { return q.GetRow2(); }

    ILINE static bool IsEquivalent(const QuatT_tpl<F>& qt1, const QuatT_tpl<F>& qt2, F qe = RAD_EPSILON, F ve = VEC_EPSILON)
    {
        real rad = acos(min(1.0f, fabs_tpl(qt1.q | qt2.q)));
        bool qdif = rad <= qe;
        bool vdif   = fabs_tpl(qt1.t.x - qt2.t.x) <= ve && fabs_tpl(qt1.t.y - qt2.t.y) <= ve && fabs_tpl(qt1.t.z - qt2.t.z) <= ve;
        return (qdif && vdif);
    }

    ILINE bool IsValid() const
    {
        if (!t.IsValid())
        {
            return false;
        }
        if (!q.IsValid())
        {
            return false;
        }
        return true;
    }

    /*!
    * linear-interpolation between quaternions (lerp)
    *
    * Example:
    *  CQuaternion result,p,q;
    *  result=qlerp( p, q, 0.5f );
    */
    ILINE void SetNLerp(const QuatT_tpl<F>& p, const QuatT_tpl<F>& tq, F ti)
    {
        assert(p.q.IsValid());
        assert(tq.q.IsValid());
        Quat_tpl<F> d = tq.q;
        if ((p.q | d) < 0)
        {
            d = -d;
        }
        Vec3_tpl<F> vDiff = d.v - p.q.v;
        q.v = p.q.v + (vDiff * ti);
        q.w = p.q.w + ((d.w - p.q.w) * ti);
        q.Normalize();
        vDiff = tq.t - p.t;
        t = p.t + (vDiff * ti);
    }
    ILINE static QuatT_tpl<F> CreateNLerp(const QuatT_tpl<F>& p, const QuatT_tpl<F>& q, F t)
    {
        QuatT_tpl<F> d;
        d.SetNLerp(p, q, t);
        return d;
    }

    //NOTE: all vectors are stored in columns
    ILINE void SetFromVectors(const Vec3& vx, const Vec3& vy, const Vec3& vz, const Vec3& pos)
    {
        Matrix34 m34;
        m34.m00 = vx.x;
        m34.m01 = vy.x;
        m34.m02 = vz.x;
        m34.m03 = pos.x;
        m34.m10 = vx.y;
        m34.m11 = vy.y;
        m34.m12 = vz.y;
        m34.m13 = pos.y;
        m34.m20 = vx.z;
        m34.m21 = vy.z;
        m34.m22 = vz.z;
        m34.m23 = pos.z;
        *this = QuatT_tpl<F>(m34);
    }
    ILINE static QuatT_tpl<F> CreateFromVectors(const Vec3_tpl<F>& vx, const Vec3_tpl<F>& vy, const Vec3_tpl<F>& vz, const Vec3_tpl<F>& pos)
    {
        QuatT_tpl<F> qt;
        qt.SetFromVectors(vx, vy, vz, pos);
        return qt;
    }

    QuatT_tpl<F> GetScaled(F scale)
    {
        return QuatT_tpl<F>(t * scale, q.GetScaled(scale));
    }

    AUTO_STRUCT_INFO
};

typedef QuatT_tpl<f32>  QuatT; //always 32 bit
typedef QuatT_tpl<f64>  QuatTd;//always 64 bit
typedef QuatT_tpl<real> QuatTr;//variable float precision. depending on the target system it can be between 32, 64 or bit

// alligned versions
typedef DEFINE_ALIGNED_DATA (QuatT, QuatTA, 32);                 //wastest 4byte per quatT // typedef __declspec(align(16)) Quat_tpl<f32>       QuatTA;
typedef DEFINE_ALIGNED_DATA (QuatTd, QuatTrA, 16);           // typedef __declspec(align(16)) Quat_tpl<f32>     QuatTrA;


/*!
*
*  Implements the multiplication operator: QuatT=Quatpos*Quat
*
*  AxB = operation B followed by operation A.
*  A multiplication takes 16 muls and 12 adds (=28 float operations).
*
*  Example:
*   Quat    quat        =   Quat::CreateRotationX(1.94192f);;
*   QuatT quatpos   =   QuatT::CreateRotationZ(3.14192f,Vec3(11,22,33));
*     QuatT qp          =   quatpos*quat;
*/
template<class F1, class F2>
ILINE QuatT_tpl<F1> operator * (const QuatT_tpl<F1>& p, const Quat_tpl<F2>& q)
{
    assert(p.IsValid());
    assert(q.IsValid());
    return QuatT_tpl<F1>(p.q * q, p.t);
}

/*!
*  Implements the multiplication operator: QuatT=QuatposA*QuatposB
*
*  AxB = operation B followed by operation A.
*  A multiplication takes 31 muls and 30 adds  (=61 float operations).
*
*  Example:
*   QuatT quatposA  =   QuatT::CreateRotationX(1.94192f,Vec3(77,55,44));
*   QuatT quatposB  =   QuatT::CreateRotationZ(3.14192f,Vec3(11,22,33));
*     QuatT qp              =   quatposA*quatposB;
*/
template<class F1, class F2>
ILINE QuatT_tpl<F1> operator * (const QuatT_tpl<F1>& q, const QuatT_tpl<F2>& p)
{
    assert(q.IsValid());
    assert(p.IsValid());
    return QuatT_tpl<F1>(q.q * p.q, q.q * p.t + q.t);
}



/*!
* post-multiply of a QuatT and a Vec3 (3D rotations with quaternions)
*
* Example:
*  Quat q(1,0,0,0);
*  Vec3 v(33,44,55);
*    Vec3 result = q*v;
*/
template<class F, class F2>
Vec3_tpl<F> operator * (const QuatT_tpl<F>& q, const Vec3_tpl<F2>& v)
{
    assert(v.IsValid());
    assert(q.IsValid());
    //muls=15 / adds=15+3
    Vec3_tpl<F> out, r2;
    r2.x = (q.q.v.y * v.z - q.q.v.z * v.y) + q.q.w * v.x;
    r2.y = (q.q.v.z * v.x - q.q.v.x * v.z) + q.q.w * v.y;
    r2.z = (q.q.v.x * v.y - q.q.v.y * v.x) + q.q.w * v.z;
    out.x = (r2.z * q.q.v.y - r2.y * q.q.v.z);
    out.x += out.x + v.x + q.t.x;
    out.y = (r2.x * q.q.v.z - r2.z * q.q.v.x);
    out.y += out.y + v.y + q.t.y;
    out.z = (r2.y * q.q.v.x - r2.x * q.q.v.y);
    out.z += out.z + v.z + q.t.z;
    return out;
}
















//----------------------------------------------------------------------
// Quaternion with translation vector and scale
// Similar to QuatT, but s is not ignored.
// Most functions then differ, so we don't inherit.
//----------------------------------------------------------------------
template <typename F>
struct QuatTS_tpl
{
    Quat_tpl<F> q;
    Vec3_tpl<F> t;
    F                       s;

    //constructors
#if defined(_DEBUG)
    ILINE QuatTS_tpl()
    {
        if constexpr (sizeof(F) == 4)
        {
            uint32* p = alias_cast<uint32*>(&q.v.x);
            p[0] = F32NAN;
            p[1] = F32NAN;
            p[2] = F32NAN;
            p[3] = F32NAN;
            p[4] = F32NAN;
            p[5] = F32NAN;
            p[6] = F32NAN;
            p[7] = F32NAN;
        }
        if constexpr (sizeof(F) == 8)
        {
            uint64* p = alias_cast<uint64*>(&q.v.x);
            p[0] = F64NAN;
            p[1] = F64NAN;
            p[2] = F64NAN;
            p[3] = F64NAN;
            p[4] = F64NAN;
            p[5] = F64NAN;
            p[6] = F64NAN;
            p[7] = F64NAN;
        }
    }
#else
    ILINE QuatTS_tpl(){}
#endif

    ILINE QuatTS_tpl(const Quat_tpl<F>& quat, const Vec3_tpl<F>& trans, F scale = 1)    { q = quat; t = trans; s = scale; }
    ILINE QuatTS_tpl(type_identity) { SetIdentity(); }

    //CONSTRUCTOR: implement the copy/casting/assignment constructor:
    template <typename F1>
    ILINE QuatTS_tpl(const QuatTS_tpl<F1>& qts)
        :  q(qts.q)
        , t(qts.t)
        , s(qts.s)    {}

    ILINE QuatTS_tpl& operator = (const QuatT_tpl<F>& qt)
    {
        q = qt.q;
        t = qt.t;
        s = 1.0f;
        return *this;
    }
    ILINE QuatTS_tpl(const QuatT_tpl<F>& qp)  { q = qp.q; t = qp.t; s = 1.0f; }


    ILINE void SetIdentity()
    {
        q.SetIdentity();
        t = Vec3(ZERO);
        s = 1;
    }

    explicit ILINE QuatTS_tpl(const Matrix34_tpl<F>& m)
    {
        t = m.GetTranslation();

        // The determinant of a matrix is the volume spanned by its base vectors.
        // We need an approximate length scale, so we calculate the cube root of the determinant.
        s = pow(m.Determinant(), F(1.0 / 3.0));

        // Orthonormalize using X and Z as anchors.
        const Vec3_tpl<F>& r0 = m.GetRow(0);
        const Vec3_tpl<F>& r2 = m.GetRow(2);

        const Vec3_tpl<F> v0 = r0.GetNormalized();
        const Vec3_tpl<F> v1 = (r2 % r0).GetNormalized();
        const Vec3_tpl<F> v2 = (v0 % v1);

        Matrix33_tpl<F> m3;
        m3.SetRow(0, v0);
        m3.SetRow(1, v1);
        m3.SetRow(2, v2);

        q = Quat_tpl<F>(m3);
    }

    void Invert()
    {
        s = 1 / s;
        q = !q;
        t = q * t * -s;
    }
    QuatTS_tpl<F> GetInverted() const
    {
        QuatTS_tpl<F> inv;
        inv.s = 1 / s;
        inv.q = !q;
        inv.t = inv.q * t * -inv.s;
        return inv;
    }


    /*!
    * linear-interpolation between quaternions (nlerp)
    *
    * Example:
    *  CQuaternion result,p,q;
    *  result=qlerp( p, q, 0.5f );
    */
    ILINE void SetNLerp(const QuatTS_tpl<F>& p, const QuatTS_tpl<F>& tq, F ti)
    {
        assert(p.q.IsValid());
        assert(tq.q.IsValid());
        Quat_tpl<F> d = tq.q;
        if ((p.q | d) < 0)
        {
            d = -d;
        }
        Vec3_tpl<F> vDiff = d.v - p.q.v;
        q.v = p.q.v + (vDiff * ti);
        q.w = p.q.w + ((d.w - p.q.w) * ti);
        q.Normalize();

        vDiff = tq.t - p.t;
        t = p.t + (vDiff * ti);

        s     = p.s + ((tq.s - p.s) * ti);
    }

    ILINE static QuatTS_tpl<F> CreateNLerp(const QuatTS_tpl<F>& p, const QuatTS_tpl<F>& q, F t)
    {
        QuatTS_tpl<F> d;
        d.SetNLerp(p, q, t);
        return d;
    }



    ILINE static bool IsEquivalent(const QuatTS_tpl<F>& qts1, const QuatTS_tpl<F>& qts2, F qe = RAD_EPSILON, F ve = VEC_EPSILON)
    {
        f64 rad  =  acos(min(1.0f, fabs_tpl(qts1.q | qts2.q)));
        bool qdif = rad <= qe;
        bool vdif   = fabs_tpl(qts1.t.x - qts2.t.x) <= ve && fabs_tpl(qts1.t.y - qts2.t.y) <= ve && fabs_tpl(qts1.t.z - qts2.t.z) <= ve;
        bool sdif   = fabs_tpl(qts1.s - qts2.s) <= ve;
        return (qdif && vdif && sdif);
    }


    bool IsValid(F e = VEC_EPSILON) const
    {
        if (!q.v.IsValid())
        {
            return false;
        }
        if (!NumberValid(q.w))
        {
            return false;
        }
        if (!q.IsUnit(e))
        {
            return false;
        }
        if (!t.IsValid())
        {
            return false;
        }
        if (!NumberValid(s))
        {
            return false;
        }
        return true;
    }

    ILINE Vec3_tpl<F> GetColumn0() const {return q.GetColumn0(); }
    ILINE Vec3_tpl<F> GetColumn1() const {return q.GetColumn1(); }
    ILINE Vec3_tpl<F> GetColumn2() const {return q.GetColumn2(); }
    ILINE Vec3_tpl<F> GetColumn3() const {return t; }
    ILINE Vec3_tpl<F> GetRow0() const { return q.GetRow0(); }
    ILINE Vec3_tpl<F> GetRow1() const { return q.GetRow1(); }
    ILINE Vec3_tpl<F> GetRow2() const { return q.GetRow2(); }

    AUTO_STRUCT_INFO
};

typedef QuatTS_tpl<f32>  QuatTS; //always 64 bit
typedef QuatTS_tpl<f64>  QuatTSd;//always 64 bit
typedef QuatTS_tpl<real> QuatTSr;//variable float precision. depending on the target system it can be between 32, 64 or 80 bit

// alligned versions
typedef DEFINE_ALIGNED_DATA (QuatTS, QuatTSA, 16);               // typedef __declspec(align(16)) Quat_tpl<f32>     QuatTSA;
typedef DEFINE_ALIGNED_DATA (QuatTSd, QuatTSrA, 64);             // typedef __declspec(align(16)) QuatTS_tpl<f32>       QuatTSrA;

template<class F1, class F2>
ILINE QuatTS_tpl<F1> operator * (const QuatTS_tpl<F1>& a, const Quat_tpl<F2>& b)
{
    return QuatTS_tpl<F1>(a.q * b, a.t, a.s);
}


template<class F1, class F2>
ILINE QuatTS_tpl<F1> operator * (const QuatTS_tpl<F1>& a, const QuatT_tpl<F2>& b)
{
    return QuatTS_tpl<F1>(a.q * b.q, a.q * (b.t * a.s) + a.t, a.s);
}
template<class F1, class F2>
ILINE QuatTS_tpl<F1> operator * (const QuatT_tpl<F1>& a, const QuatTS_tpl<F2>& b)
{
    return QuatTS_tpl<F1>(a.q * b.q, a.q * b.t + a.t, b.s);
}
template<class F1, class F2>
ILINE QuatTS_tpl<F1> operator * (const Quat_tpl<F1>& a, const QuatTS_tpl<F2>& b)
{
    return QuatTS_tpl<F1>(a * b.q, a * b.t, b.s);
}

/*!
*  Implements the multiplication operator: QuatTS=QuatTS*QuatTS
*/
template<class F1, class F2>
ILINE QuatTS_tpl<F1> operator * (const QuatTS_tpl<F1>& a, const QuatTS_tpl<F2>& b)
{
    assert(a.IsValid());
    assert(b.IsValid());
    return QuatTS_tpl<F1>(a.q * b.q, a.q * (b.t * a.s) + a.t, a.s * b.s);
}

/*!
* post-multiply of a QuatT and a Vec3 (3D rotations with quaternions)
*/
template<class F, class F2>
ILINE Vec3_tpl<F> operator * (const QuatTS_tpl<F>& q, const Vec3_tpl<F2>& v)
{
    assert(q.IsValid());
    assert(v.IsValid());
    return q.q * v * q.s + q.t;
}


//----------------------------------------------------------------------
// Quaternion with translation vector and non-uniform scale
//----------------------------------------------------------------------
template <typename F>
struct QuatTNS_tpl
{
    Quat_tpl<F> q;
    Vec3_tpl<F> t;
    Vec3_tpl<F> s;

    //constructors
#if defined(_DEBUG)
    ILINE QuatTNS_tpl()
    {
        if constexpr (sizeof(F) == 4)
        {
            uint32* p = alias_cast<uint32*>(&q.v.x);
            p[0] = F32NAN;
            p[1] = F32NAN;
            p[2] = F32NAN;
            p[3] = F32NAN;
            p[4] = F32NAN;
            p[5] = F32NAN;
            p[6] = F32NAN;
            p[7] = F32NAN;
            p[8] = F32NAN;
            p[9] = F32NAN;
        }
        if constexpr (sizeof(F) == 8)
        {
            uint64* p = alias_cast<uint64*>(&q.v.x);
            p[0] = F64NAN;
            p[1] = F64NAN;
            p[2] = F64NAN;
            p[3] = F64NAN;
            p[4] = F64NAN;
            p[5] = F64NAN;
            p[6] = F64NAN;
            p[7] = F64NAN;
            p[8] = F64NAN;
            p[9] = F64NAN;
        }
    }
#else
    ILINE QuatTNS_tpl() {};
#endif

    ILINE QuatTNS_tpl(const Quat_tpl<F>& quat, const Vec3_tpl<F>& trans, const Vec3_tpl<F>& scale = Vec3_tpl<F>(1)) { q = quat; t = trans; s = scale; }
    ILINE QuatTNS_tpl(type_identity) { SetIdentity(); }

    //CONSTRUCTOR: implement the copy/casting/assignment constructor:
    template <typename F1>
    ILINE QuatTNS_tpl(const QuatTS_tpl<F1>& qts)
        :     q(qts.q)
        , t(qts.t)
        , s(qts.s)
    {
    }

    ILINE QuatTNS_tpl& operator = (const QuatT_tpl<F>& qt)
    {
        q = qt.q;
        t = qt.t;
        s = Vec3_tpl<F>(1);
        return *this;
    }
    ILINE QuatTNS_tpl(const QuatT_tpl<F>& qp) { q = qp.q; t = qp.t; s = Vec3_tpl<F>(1); }


    ILINE void SetIdentity()
    {
        q.SetIdentity();
        t = Vec3_tpl<F>(ZERO);
        s = Vec3_tpl<F>(1);
    }

    explicit ILINE QuatTNS_tpl(const Matrix34_tpl<F>& m)
    {
        t = m.GetTranslation();

        // Lengths of base vectors equates to scaling in matrix
        s.x = m.GetColumn0().GetLength();
        s.y = m.GetColumn1().GetLength();
        s.z = m.GetColumn2().GetLength();

        // Orthonormalize using X and Z as anchors.
        const Vec3_tpl<F>& r0 = m.GetRow(0);
        const Vec3_tpl<F>& r2 = m.GetRow(2);

        const Vec3_tpl<F> v0 = r0.GetNormalized();
        const Vec3_tpl<F> v1 = (r2 % r0).GetNormalized();
        const Vec3_tpl<F> v2 = (v0 % v1);

        Matrix33_tpl<F> m3;
        m3.SetRow(0, v0);
        m3.SetRow(1, v1);
        m3.SetRow(2, v2);

        q = Quat_tpl<F>(m3);
    }

    void Invert()
    {
        s = Vec3_tpl<F>(1) / s;
        q = !q;
        t = q * t * -s;
    }
    QuatTNS_tpl<F> GetInverted() const
    {
        QuatTNS_tpl<F> inv;
        inv.s = Vec3_tpl<F>(1) / s;
        inv.q = !q;
        inv.t = inv.q * t * -inv.s;
        return inv;
    }


    /*!
    * linear-interpolation between quaternions (nlerp)
    *
    * Example:
    *  CQuaternion result,p,q;
    *  result=qlerp( p, q, 0.5f );
    */
    ILINE void SetNLerp(const QuatTNS_tpl<F>& p, const QuatTNS_tpl<F>& tq, F ti)
    {
        assert(p.q.IsValid());
        assert(tq.q.IsValid());
        Quat_tpl<F> d = tq.q;
        if ((p.q | d) < 0)
        {
            d = -d;
        }
        Vec3_tpl<F> vDiff = d.v - p.q.v;
        q.v = p.q.v + (vDiff * ti);
        q.w = p.q.w + ((d.w - p.q.w) * ti);
        q.Normalize();

        vDiff = tq.t - p.t;
        t = p.t + (vDiff * ti);

        s     = p.s + ((tq.s - p.s) * ti);
    }

    ILINE static QuatTNS_tpl<F> CreateNLerp(const QuatTNS_tpl<F>& p, const QuatTNS_tpl<F>& q, F t)
    {
        QuatTNS_tpl<F> d;
        d.SetNLerp(p, q, t);
        return d;
    }



    ILINE static bool IsEquivalent(const QuatTNS_tpl<F>& qts1, const QuatTNS_tpl<F>& qts2, F qe = RAD_EPSILON, F ve = VEC_EPSILON)
    {
        real rad  = acos(min(1.0f, fabs_tpl(qts1.q | qts2.q)));
        bool qdif = rad <= qe;
        bool vdif   = fabs_tpl(qts1.t.x - qts2.t.x) <= ve && fabs_tpl(qts1.t.y - qts2.t.y) <= ve && fabs_tpl(qts1.t.z - qts2.t.z) <= ve;
        bool sdif   = fabs_tpl(qts1.s.x - qts2.s.x) <= ve && fabs_tpl(qts1.s.y - qts2.s.y) <= ve && fabs_tpl(qts1.s.z - qts2.s.z) <= ve;
        return (qdif && vdif && sdif);
    }


    bool IsValid(F e = VEC_EPSILON) const
    {
        if (!q.v.IsValid())
        {
            return false;
        }
        if (!NumberValid(q.w))
        {
            return false;
        }
        if (!q.IsUnit(e))
        {
            return false;
        }
        if (!t.IsValid())
        {
            return false;
        }
        if (!NumberValid(s.x))
        {
            return false;
        }
        if (!NumberValid(s.y))
        {
            return false;
        }
        if (!NumberValid(s.z))
        {
            return false;
        }
        return true;
    }

    ILINE Vec3_tpl<F> GetColumn0() const {return q.GetColumn0(); }
    ILINE Vec3_tpl<F> GetColumn1() const {return q.GetColumn1(); }
    ILINE Vec3_tpl<F> GetColumn2() const {return q.GetColumn2(); }
    ILINE Vec3_tpl<F> GetColumn3() const {return t; }
    ILINE Vec3_tpl<F> GetRow0() const { return q.GetRow0(); }
    ILINE Vec3_tpl<F> GetRow1() const { return q.GetRow1(); }
    ILINE Vec3_tpl<F> GetRow2() const { return q.GetRow2(); }

    AUTO_STRUCT_INFO
};

typedef QuatTNS_tpl<f32> QuatTNS;
typedef QuatTNS_tpl<f64> QuatTNSr;
typedef QuatTNS_tpl<f64> QuatTNS_f64;

// alligned versions
typedef DEFINE_ALIGNED_DATA (QuatTNS, QuatTNSA, 16);
typedef DEFINE_ALIGNED_DATA (QuatTNSr, QuatTNSrA, 64);
typedef DEFINE_ALIGNED_DATA (QuatTNS_f64, QuatTNS_f64A, 64);

template<class F1, class F2>
ILINE QuatTNS_tpl<F1> operator * (const QuatTNS_tpl<F1>& a, const Quat_tpl<F2>& b)
{
    return QuatTNS_tpl<F1>(a.q * b, a.t, a.s);
}

template<class F1, class F2>
ILINE QuatTNS_tpl<F1> operator * (const Quat_tpl<F1>& a, const QuatTNS_tpl<F2>& b)
{
    return QuatTNS_tpl<F1>(a * b.q, a * b.t, b.s);
}

template<class F1, class F2>
ILINE QuatTNS_tpl<F1> operator * (const QuatTNS_tpl<F1>& a, const QuatT_tpl<F2>& b)
{
    return QuatTNS_tpl<F1>(a.q * b.q, a.q * Vec3_tpl<F1>(b.x * a.s.x, b.y * a.s.y, b.z * a.s.z) + a.t, a.s);
}

template<class F1, class F2>
ILINE QuatTNS_tpl<F1> operator * (const QuatT_tpl<F1>& a, const QuatTNS_tpl<F2>& b)
{
    return QuatTNS_tpl<F1>(a.q * b.q, a.q * b.t + a.t, b.s);
}

/*!
*  Implements the multiplication operator: QuatTNS=QuatTNS*QuatTNS
*/
template<class F1, class F2>
ILINE QuatTNS_tpl<F1> operator * (const QuatTNS_tpl<F1>& a, const QuatTNS_tpl<F2>& b)
{
    assert(a.IsValid());
    assert(b.IsValid());
    return QuatTNS_tpl<F1>(
        a.q * b.q,
        a.q * Vec3_tpl<F1>(b.t.x * a.s.x, b.t.y * a.s.y, b.t.z * a.s.z) + a.t,
        Vec3_tpl<F1>(a.s.x * b.s.x, a.s.y * b.s.y, a.s.z * b.s.z)
        );
}

/*!
* post-multiply of a QuatTNS and a Vec3 (3D rotations with quaternions)
*/
template<class F1, class F2>
ILINE Vec3_tpl<F1> operator * (const QuatTNS_tpl<F1>& q, const Vec3_tpl<F2>& v)
{
    assert(q.IsValid());
    assert(v.IsValid());
    return q.q * Vec3_tpl<F1>(v.x * q.s.x, v.y * q.s.y, v.z * q.s.z) + q.t;
}

//----------------------------------------------------------------------
// Dual Quaternion
//----------------------------------------------------------------------
template <typename F>
struct DualQuat_tpl
{
    Quat_tpl<F> nq;
    Quat_tpl<F> dq;

    ILINE DualQuat_tpl()  {}

    ILINE DualQuat_tpl(const Quat_tpl<F>& q, const Vec3_tpl<F>& t)
    {
        //copy the quaternion part
        nq = q;
        //convert the translation into a dual quaternion part
        dq.w        = -0.5f * (t.x * q.v.x  + t.y * q.v.y + t.z * q.v.z);
        dq.v.x  = 0.5f * (t.x * q.w        + t.y * q.v.z - t.z * q.v.y);
        dq.v.y  = 0.5f * (-t.x * q.v.z  + t.y * q.w       + t.z * q.v.x);
        dq.v.z  = 0.5f * (t.x * q.v.y  - t.y * q.v.x + t.z * q.w);
    }

    ILINE DualQuat_tpl(const QuatT_tpl<F>& qt)
    {
        //copy the quaternion part
        nq = qt.q;
        //convert the translation into a dual quaternion part
        dq.w        = -0.5f * (qt.t.x * qt.q.v.x    + qt.t.y * qt.q.v.y   + qt.t.z * qt.q.v.z);
        dq.v.x  = 0.5f * (qt.t.x * qt.q.w      - qt.t.z * qt.q.v.y + qt.t.y * qt.q.v.z);
        dq.v.y  = 0.5f * (qt.t.y * qt.q.w      - qt.t.x * qt.q.v.z   + qt.t.z * qt.q.v.x);
        dq.v.z  = 0.5f * (qt.t.x * qt.q.v.y    - qt.t.y * qt.q.v.x   + qt.t.z * qt.q.w);
    }

    explicit ILINE DualQuat_tpl( const Matrix34_tpl<F>& m34 )
    {
        // non-dual part (just copy q0):
        nq = Quat_tpl<F>(m34);
        f32 tx = m34.m03;
        f32 ty = m34.m13;
        f32 tz = m34.m23;

        // dual part:
        dq.w        = -0.5f * (tx * nq.v.x  + ty * nq.v.y + tz * nq.v.z);
        dq.v.x  = 0.5f * (tx * nq.w        + ty * nq.v.z - tz * nq.v.y);
        dq.v.y  = 0.5f * (-tx * nq.v.z  + ty * nq.w       + tz * nq.v.x);
        dq.v.z  = 0.5f * (tx * nq.v.y  - ty * nq.v.x + tz * nq.w);
    }


    ILINE DualQuat_tpl(type_identity)
    {
        SetIdentity();
    }

    ILINE DualQuat_tpl(type_zero)
    {
        SetZero();
    }

    template <typename F1>
    ILINE DualQuat_tpl(const DualQuat_tpl<F1>& QDual)
        : nq(QDual.nq)
        , dq(QDual.dq) {}

    ILINE void SetIdentity()
    {
        nq.SetIdentity();
        dq.w = 0.0f;
        dq.v.x = 0.0f;
        dq.v.y = 0.0f;
        dq.v.z = 0.0f;
    }

    ILINE void SetZero()
    {
        nq.w = 0.0f;
        nq.v.x = 0.0f;
        nq.v.y = 0.0f;
        nq.v.z = 0.0f;
        dq.w = 0.0f;
        dq.v.x = 0.0f;
        dq.v.y = 0.0f;
        dq.v.z = 0.0f;
    }

    ILINE void Normalize()
    {
        // Normalize both components so that nq is unit
        F norm = isqrt_safe_tpl(nq.v.len2() + sqr(nq.w));
        nq *= norm;
        dq *= norm;
    }

    AUTO_STRUCT_INFO
};

#ifndef MAX_API_NUM
typedef DualQuat_tpl<f32>  DualQuat; //always 32 bit
typedef DualQuat_tpl<f64>  DualQuatd;//always 64 bit
typedef DualQuat_tpl<real> DualQuatr;//variable float precision. depending on the target system it can be between 32, 64 or 80 bit
#else
typedef DualQuat_tpl<f32> CryDualQuat;
#endif

template<class F1, class F2>
ILINE DualQuat_tpl<F1> operator*(const DualQuat_tpl<F1>& l, const F2 r)
{
    DualQuat_tpl<F1> dual;
    dual.nq.w  = l.nq.w  * r;
    dual.nq.v.x = l.nq.v.x * r;
    dual.nq.v.y = l.nq.v.y * r;
    dual.nq.v.z = l.nq.v.z * r;

    dual.dq.w  = l.dq.w  * r;
    dual.dq.v.x = l.dq.v.x * r;
    dual.dq.v.y = l.dq.v.y * r;
    dual.dq.v.z = l.dq.v.z * r;
    return dual;
}

template<class F1, class F2>
ILINE DualQuat_tpl<F1> operator+(const DualQuat_tpl<F1>& l, const DualQuat_tpl<F2>& r)
{
    DualQuat_tpl<F1> dual;
    dual.nq.w  = l.nq.w  + r.nq.w;
    dual.nq.v.x = l.nq.v.x + r.nq.v.x;
    dual.nq.v.y = l.nq.v.y + r.nq.v.y;
    dual.nq.v.z = l.nq.v.z + r.nq.v.z;

    dual.dq.w  = l.dq.w  + r.dq.w;
    dual.dq.v.x = l.dq.v.x + r.dq.v.x;
    dual.dq.v.y = l.dq.v.y + r.dq.v.y;
    dual.dq.v.z = l.dq.v.z + r.dq.v.z;
    return dual;
}

template<class F1, class F2>
ILINE void operator += (DualQuat_tpl<F1>& l, const DualQuat_tpl<F2>& r)
{
    l.nq.w  += r.nq.w;
    l.nq.v.x += r.nq.v.x;
    l.nq.v.y += r.nq.v.y;
    l.nq.v.z += r.nq.v.z;

    l.dq.w  += r.dq.w;
    l.dq.v.x += r.dq.v.x;
    l.dq.v.y += r.dq.v.y;
    l.dq.v.z += r.dq.v.z;
}

template<class F1, class F2>
ILINE Vec3_tpl<F1> operator*(const DualQuat_tpl<F1>& dq, const Vec3_tpl<F2>& v)
{
    F2 t;
    const F2 ax = dq.nq.v.y * v.z - dq.nq.v.z * v.y + dq.nq.w * v.x;
    const F2 ay = dq.nq.v.z * v.x - dq.nq.v.x * v.z + dq.nq.w * v.y;
    const F2 az = dq.nq.v.x * v.y - dq.nq.v.y * v.x + dq.nq.w * v.z;
    F2 x = dq.dq.v.x * dq.nq.w - dq.nq.v.x * dq.dq.w + dq.nq.v.y * dq.dq.v.z - dq.nq.v.z * dq.dq.v.y;
    x += x;
    t = (az * dq.nq.v.y - ay * dq.nq.v.z);
    x += t + t + v.x;
    F2 y = dq.dq.v.y * dq.nq.w - dq.nq.v.y * dq.dq.w + dq.nq.v.z * dq.dq.v.x - dq.nq.v.x * dq.dq.v.z;
    y += y;
    t = (ax * dq.nq.v.z - az * dq.nq.v.x);
    y += t + t + v.y;
    F2 z = dq.dq.v.z * dq.nq.w - dq.nq.v.z * dq.dq.w + dq.nq.v.x * dq.dq.v.y - dq.nq.v.y * dq.dq.v.x;
    z += z;
    t = (ay * dq.nq.v.x - ax * dq.nq.v.y);
    z += t + t + v.z;
    return Vec3_tpl<F2>(x, y, z);
}

template<class F1, class F2>
ILINE DualQuat_tpl<F1> operator*(const DualQuat_tpl<F1>& a, const DualQuat_tpl<F2>& b)
{
    DualQuat_tpl<F1> dual;

    dual.nq.v.x = a.nq.v.y * b.nq.v.z - a.nq.v.z * b.nq.v.y + a.nq.w * b.nq.v.x + a.nq.v.x * b.nq.w;
    dual.nq.v.y = a.nq.v.z * b.nq.v.x - a.nq.v.x * b.nq.v.z + a.nq.w * b.nq.v.y + a.nq.v.y * b.nq.w;
    dual.nq.v.z = a.nq.v.x * b.nq.v.y - a.nq.v.y * b.nq.v.x + a.nq.w * b.nq.v.z + a.nq.v.z * b.nq.w;
    dual.nq.w = a.nq.w * b.nq.w  - (a.nq.v.x * b.nq.v.x + a.nq.v.y * b.nq.v.y + a.nq.v.z * b.nq.v.z);

    //dual.dq   = a.nq*b.dq + a.dq*b.nq;
    dual.dq.v.x = a.nq.v.y * b.dq.v.z - a.nq.v.z * b.dq.v.y + a.nq.w * b.dq.v.x + a.nq.v.x * b.dq.w;
    dual.dq.v.y = a.nq.v.z * b.dq.v.x - a.nq.v.x * b.dq.v.z + a.nq.w * b.dq.v.y + a.nq.v.y * b.dq.w;
    dual.dq.v.z = a.nq.v.x * b.dq.v.y - a.nq.v.y * b.dq.v.x + a.nq.w * b.dq.v.z + a.nq.v.z * b.dq.w;
    dual.dq.w = a.nq.w * b.dq.w  - (a.nq.v.x * b.dq.v.x + a.nq.v.y * b.dq.v.y + a.nq.v.z * b.dq.v.z);

    dual.dq.v.x += a.dq.v.y * b.nq.v.z - a.dq.v.z * b.nq.v.y + a.dq.w * b.nq.v.x + a.dq.v.x * b.nq.w;
    dual.dq.v.y += a.dq.v.z * b.nq.v.x - a.dq.v.x * b.nq.v.z + a.dq.w * b.nq.v.y + a.dq.v.y * b.nq.w;
    dual.dq.v.z += a.dq.v.x * b.nq.v.y - a.dq.v.y * b.nq.v.x + a.dq.w * b.nq.v.z + a.dq.v.z * b.nq.w;
    dual.dq.w += a.dq.w * b.nq.w  - (a.dq.v.x * b.nq.v.x + a.dq.v.y * b.nq.v.y + a.dq.v.z * b.nq.v.z);

    return dual;
}


#endif // CRYINCLUDE_CRYCOMMON_CRY_QUAT_H


