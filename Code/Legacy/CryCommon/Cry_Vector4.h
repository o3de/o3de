/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Common vector class


#pragma once


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// class Vec4_tpl
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template <typename F>
struct Vec4_tpl
{
    typedef F value_type;
    enum
    {
        component_count = 4
    };

    F x, y, z, w;

#if defined(_DEBUG)
    ILINE Vec4_tpl()
    {
        if constexpr (sizeof(F) == 4)
        {
            uint32* p = alias_cast<uint32*>(&x);
            p[0] = F32NAN;
            p[1] = F32NAN;
            p[2] = F32NAN;
            p[3] = F32NAN;
        }
        if constexpr (sizeof(F) == 8)
        {
            uint64* p = alias_cast<uint64*>(&x);
            p[0] = F64NAN;
            p[1] = F64NAN;
            p[2] = F64NAN;
            p[3] = F64NAN;
        }
    }
#else
    ILINE Vec4_tpl() {}
#endif

    template<typename F2>
    ILINE Vec4_tpl<F>& operator = (const Vec4_tpl<F2>& v1)
    {
        x = F(v1.x);
        y = F(v1.y);
        z = F(v1.z);
        w = F(v1.w);
        return (*this);
    }

    ILINE Vec4_tpl(F vx, F vy, F vz, F vw) { x = vx; y = vy; z = vz; w = vw; }
    ILINE Vec4_tpl(const Vec3_tpl<F>& v, F vw) {  x = v.x; y = v.y; z = v.z; w = vw; }
    explicit ILINE Vec4_tpl(F m) { x = y = z = w = m; }
    ILINE Vec4_tpl(type_zero) { x = y = z = w = F(0); }

    ILINE void operator () (F vx, F vy, F vz, F vw) { x = vx; y = vy; z = vz; w = vw; }
    ILINE void operator () (const Vec3_tpl<F>& v, F vw) {  x = v.x; y = v.y; z = v.z; w = vw; }

    ILINE F& operator [] (int index)          { assert(index >= 0 && index <= 3);  return ((F*)this)[index]; }
    ILINE F operator [] (int index) const { assert(index >= 0 && index <= 3);  return ((F*)this)[index]; }
    template <class T>
    ILINE  Vec4_tpl(const Vec4_tpl<T>& v)
        : x((F)v.x)
        , y((F)v.y)
        , z((F)v.z)
        , w((F)v.w) { assert(this->IsValid()); }

    ILINE Vec4_tpl& zero() { x = y = z = w = 0; return *this; }

    ILINE bool IsEquivalent(const Vec4_tpl<F>& v1, F epsilon = VEC_EPSILON) const
    {
        assert(v1.IsValid());
        assert(this->IsValid());
        return  ((fabs_tpl(x - v1.x) <= epsilon) &&   (fabs_tpl(y - v1.y) <= epsilon) && (fabs_tpl(z - v1.z) <= epsilon) && (fabs_tpl(w - v1.w) <= epsilon));
    }


    ILINE Vec4_tpl& operator=(const Vec4_tpl& src)
    {
        x = src.x;
        y = src.y;
        z = src.z;
        w = src.w;
        return *this;
    }

    ILINE Vec4_tpl<F> operator * (F k) const
    {
        return Vec4_tpl<F>(x * k, y * k, z * k, w * k);
    }
    ILINE Vec4_tpl<F> operator / (F k) const
    {
        k = (F)1.0 / k;
        return Vec4_tpl<F>(x * k, y * k, z * k, w * k);
    }

    ILINE Vec4_tpl<F>& operator *= (F k)
    {
        x *= k;
        y *= k;
        z *= k;
        w *= k;
        return *this;
    }
    ILINE Vec4_tpl<F>& operator /= (F k)
    {
        k = (F)1.0 / k;
        x *= k;
        y *= k;
        z *= k;
        w *= k;
        return *this;
    }

    ILINE void operator += (const Vec4_tpl<F>& v)
    {
        x += v.x;
        y += v.y;
        z += v.z;
        w += v.w;
    }

    ILINE void operator -= (const Vec4_tpl<F>& v)
    {
        x -= v.x;
        y -= v.y;
        z -= v.z;
        w -= v.w;
    }


    ILINE F Dot (const Vec4_tpl<F>& vec2)   const
    {
        return x * vec2.x + y * vec2.y + z * vec2.z + w * vec2.w;
    }

    ILINE F GetLength() const
    {
        return sqrt_tpl(Dot(*this));
    }

    ILINE F GetLengthSquared() const
    {
        return Dot(*this);
    }


    bool IsValid() const
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
        if (!NumberValid(w))
        {
            return false;
        }
        return true;
    }

    /*!
    * functions and operators to compare vector
    *
    * Example:
    *  if (v0==v1) dosomething;
    */
    ILINE bool operator==(const Vec4_tpl<F>& vec)
    {
        return x == vec.x && y == vec.y && z == vec.z && w == vec.w;
    }
    ILINE bool operator!=(const Vec4_tpl<F>& vec) { return !(*this == vec); }

    ILINE friend bool operator ==(const Vec4_tpl<F>& v0, const Vec4_tpl<F>& v1)
    {
        return ((v0.x == v1.x) && (v0.y == v1.y) && (v0.z == v1.z) && (v0.w == v1.w));
    }
    ILINE friend bool operator !=(const Vec4_tpl<F>& v0, const Vec4_tpl<F>& v1) {   return !(v0 == v1);   }

    //! normalize the vector
    // The default Normalize function is in fact "safe". 0 vectors remain unchanged.
    ILINE void  Normalize()
    {
        assert(this->IsValid());
        F fInvLen = isqrt_safe_tpl(x * x + y * y + z * z + w * w);
        x *= fInvLen;
        y *= fInvLen;
        z *= fInvLen;
        w *= fInvLen;
    }

    //! may be faster and less accurate
    ILINE void NormalizeFast()
    {
        assert(this->IsValid());
        F fInvLen = isqrt_fast_tpl(x * x + y * y + z * z + w * w);
        x *= fInvLen;
        y *= fInvLen;
        z *= fInvLen;
        w *= fInvLen;
    }

    ILINE void SetLerp(const Vec4_tpl<F>& p, const Vec4_tpl<F>& q, F t) { *this = p * (1.0f - t) + q * t; }
    ILINE static Vec4_tpl<F> CreateLerp(const Vec4_tpl<F>& p, const Vec4_tpl<F>& q, F t) {    return p * (1.0f - t) + q * t; }

    AUTO_STRUCT_INFO
};


typedef Vec4_tpl<f32>    Vec4;  // always 32 bit
typedef Vec4_tpl<f64>    Vec4d; // always 64 bit
typedef Vec4_tpl<int32>  Vec4i;
typedef Vec4_tpl<uint32> Vec4ui;
typedef Vec4_tpl<real> Vec4r; // variable float precision. depending on the target system it can be 32, 64 or 80 bit

#if defined(WIN32) || defined(WIN64) || defined(LINUX) || defined(APPLE)
typedef Vec4_tpl<f32> Vec4A;
#elif defined(AZ_RESTRICTED_PLATFORM)
    #include AZ_RESTRICTED_FILE(Cry_Vector4_h)
#endif

//vector self-addition
template<class F1, class F2>
ILINE Vec4_tpl<F1>& operator += (Vec4_tpl<F1>& v0, const Vec4_tpl<F2>& v1)
{
    v0 = v0 + v1;
    return v0;
}

//vector addition
template<class F1, class F2>
ILINE Vec4_tpl<F1> operator + (const Vec4_tpl<F1>& v0, const Vec4_tpl<F2>& v1)
{
    return Vec4_tpl<F1>(v0.x + v1.x, v0.y + v1.y, v0.z + v1.z, v0.w + v1.w);
}
//vector subtraction
template<class F1, class F2>
ILINE Vec4_tpl<F1> operator - (const Vec4_tpl<F1>& v0, const Vec4_tpl<F2>& v1)
{
    return Vec4_tpl<F1>(v0.x - v1.x, v0.y - v1.y, v0.z - v1.z, v0.w - v1.w);
}

//vector multiplication
template<class F1, class F2>
ILINE Vec4_tpl<F1> operator * (const Vec4_tpl<F1> v0, const Vec4_tpl<F2>& v1)
{
    return Vec4_tpl<F1>(v0.x * v1.x, v0.y * v1.y, v0.z * v1.z, v0.w * v1.w);
}

//vector division
template<class F1, class F2>
ILINE Vec4_tpl<F1> operator / (const Vec4_tpl<F1>& v0, const Vec4_tpl<F2>& v1)
{
    return Vec4_tpl<F1>(v0.x / v1.x, v0.y / v1.y, v0.z / v1.z, v0.w / v1.w);
}
