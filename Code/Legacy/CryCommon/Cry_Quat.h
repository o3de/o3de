/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Common quaternion class
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
};


///////////////////////////////////////////////////////////////////////////////
// Typedefs                                                                  //
///////////////////////////////////////////////////////////////////////////////

typedef Quat_tpl<f32>  Quat;  //always 32 bit

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
