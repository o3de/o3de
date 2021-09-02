/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

struct Vec4_tpl
{
    using value_type = f32;
    enum
    {
        component_count = 4
    };

    f32 x, y, z, w;

#if defined(_DEBUG)
    ILINE Vec4_tpl()
    {
        if constexpr (sizeof(f32) == 4)
        {
            uint32* p = alias_cast<uint32*>(&x);
            p[0] = F32NAN;
            p[1] = F32NAN;
            p[2] = F32NAN;
            p[3] = F32NAN;
        }
    }
#else
    ILINE Vec4_tpl() {}
#endif

    ILINE Vec4_tpl(f32 vx, f32 vy, f32 vz, f32 vw) { x = vx; y = vy; z = vz; w = vw; }
    ILINE Vec4_tpl(const Vec3_tpl<f32>& v, f32 vw) {  x = v.x; y = v.y; z = v.z; w = vw; }
    explicit ILINE Vec4_tpl(f32 m) { x = y = z = w = m; }
    ILINE Vec4_tpl(type_zero) { x = y = z = w = f32(0); }

    ILINE void operator () (f32 vx, f32 vy, f32 vz, f32 vw) { x = vx; y = vy; z = vz; w = vw; }
    ILINE void operator () (const Vec3_tpl<f32>& v, f32 vw) {  x = v.x; y = v.y; z = v.z; w = vw; }

    ILINE f32& operator [] (int index)          { assert(index >= 0 && index <= 3);  return ((f32*)this)[index]; }
    ILINE f32 operator [] (int index) const { assert(index >= 0 && index <= 3);  return ((f32*)this)[index]; }

    ILINE Vec4_tpl& zero() { x = y = z = w = 0; return *this; }

    ILINE bool IsEquivalent(const Vec4_tpl& v1, f32 epsilon = VEC_EPSILON) const
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

    ILINE Vec4_tpl operator * (f32 k) const
    {
        return Vec4_tpl(x * k, y * k, z * k, w * k);
    }
    ILINE Vec4_tpl operator / (f32 k) const
    {
        k = (f32)1.0 / k;
        return Vec4_tpl(x * k, y * k, z * k, w * k);
    }

    ILINE Vec4_tpl& operator *= (f32 k)
    {
        x *= k;
        y *= k;
        z *= k;
        w *= k;
        return *this;
    }
    ILINE Vec4_tpl& operator /= (f32 k)
    {
        k = (f32)1.0 / k;
        x *= k;
        y *= k;
        z *= k;
        w *= k;
        return *this;
    }

    ILINE void operator += (const Vec4_tpl& v)
    {
        x += v.x;
        y += v.y;
        z += v.z;
        w += v.w;
    }

    ILINE void operator -= (const Vec4_tpl& v)
    {
        x -= v.x;
        y -= v.y;
        z -= v.z;
        w -= v.w;
    }


    ILINE f32 Dot (const Vec4_tpl& vec2)   const
    {
        return x * vec2.x + y * vec2.y + z * vec2.z + w * vec2.w;
    }

    ILINE f32 GetLength() const
    {
        return sqrt_tpl(Dot(*this));
    }

    ILINE f32 GetLengthSquared() const
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
    ILINE bool operator==(const Vec4_tpl& vec)
    {
        return x == vec.x && y == vec.y && z == vec.z && w == vec.w;
    }
    ILINE bool operator!=(const Vec4_tpl& vec) { return !(*this == vec); }

    ILINE friend bool operator ==(const Vec4_tpl& v0, const Vec4_tpl& v1)
    {
        return ((v0.x == v1.x) && (v0.y == v1.y) && (v0.z == v1.z) && (v0.w == v1.w));
    }
    ILINE friend bool operator !=(const Vec4_tpl& v0, const Vec4_tpl& v1) {   return !(v0 == v1);   }

    //! normalize the vector
    // The default Normalize function is in fact "safe". 0 vectors remain unchanged.
    ILINE void  Normalize()
    {
        assert(this->IsValid());
        f32 fInvLen = isqrt_safe_tpl(x * x + y * y + z * z + w * w);
        x *= fInvLen;
        y *= fInvLen;
        z *= fInvLen;
        w *= fInvLen;
    }

    //! may be faster and less accurate
    ILINE void NormalizeFast()
    {
        assert(this->IsValid());
        f32 fInvLen = isqrt_fast_tpl(x * x + y * y + z * z + w * w);
        x *= fInvLen;
        y *= fInvLen;
        z *= fInvLen;
        w *= fInvLen;
    }
};


typedef Vec4_tpl    Vec4;  // always 32 bit

////vector self-addition
//template<class F1, class F2>
//ILINE Vec4_tpl<F1>& operator += (Vec4_tpl<F1>& v0, const Vec4_tpl<F2>& v1)
//{
//    v0 = v0 + v1;
//    return v0;
//}

//vector addition
ILINE Vec4_tpl operator + (const Vec4_tpl& v0, const Vec4_tpl& v1)
{
    return Vec4_tpl(v0.x + v1.x, v0.y + v1.y, v0.z + v1.z, v0.w + v1.w);
}
//vector subtraction
ILINE Vec4_tpl operator - (const Vec4_tpl& v0, const Vec4_tpl& v1)
{
    return Vec4_tpl(v0.x - v1.x, v0.y - v1.y, v0.z - v1.z, v0.w - v1.w);
}

////vector multiplication
//template<class F1, class F2>
//ILINE Vec4_tpl<F1> operator * (const Vec4_tpl<F1> v0, const Vec4_tpl<F2>& v1)
//{
//    return Vec4_tpl<F1>(v0.x * v1.x, v0.y * v1.y, v0.z * v1.z, v0.w * v1.w);
//}

////vector division
//template<class F1, class F2>
//ILINE Vec4_tpl<F1> operator / (const Vec4_tpl<F1>& v0, const Vec4_tpl<F2>& v1)
//{
//    return Vec4_tpl<F1>(v0.x / v1.x, v0.y / v1.y, v0.z / v1.z, v0.w / v1.w);
//}
