/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Common vector class


#ifndef CRYINCLUDE_CRYCOMMON_CRY_VECTOR3_H
#define CRYINCLUDE_CRYCOMMON_CRY_VECTOR3_H
#pragma once


#include "CryEndian.h"
#include <AzCore/Math/Vector3.h>

template<typename T>
struct VecPrecisionValues
{
    ILINE static bool CheckGreater(const T value)
    {
        return value > 0;
    }
};

template<>
struct VecPrecisionValues<float>
{
    ILINE static bool CheckGreater(const float value)
    {
        return value > FLT_EPSILON;
    }
};


template <typename F>
struct Vec3s_tpl
{
    F x, y, z;

    ILINE Vec3s_tpl(F vx, F vy, F vz)
        : x(vx)
        , y(vy)
        , z(vz){}
    ILINE F& operator [] (int32 index)        { assert(index >= 0 && index <= 2); return ((F*)this)[index]; }
    ILINE F operator [] (int32 index) const { assert(index >= 0 && index <= 2); return ((F*)this)[index]; }
};


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// class Vec3_tpl
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
template <typename F>
struct Vec3_tpl
{
    typedef F value_type;
    enum
    {
        component_count = 3
    };

    F x, y, z;

#if defined(_DEBUG)
    ILINE Vec3_tpl()
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
    ILINE Vec3_tpl()    {};
#endif

    /*!
    * template specialization to initialize a vector
    *
    * Example:
    *  Vec3 v0=Vec3(ZERO);
    *  Vec3 v1=Vec3(MIN);
    *  Vec3 v2=Vec3(MAX);
    */
    Vec3_tpl(type_zero)
        : x(0)
        , y(0)
        , z(0) {}
    Vec3_tpl(type_min);
    Vec3_tpl(type_max);

    /*!
    * constructors and bracket-operator to initialize a vector
    *
    * Example:
    *  Vec3 v0=Vec3(1,2,3);
    *  Vec3 v1(1,2,3);
    *  v2.Set(1,2,3);
    */
    ILINE Vec3_tpl(F vx, F vy, F vz)
        : x(vx)
        , y(vy)
        , z(vz){ assert(this->IsValid()); }
    ILINE void operator () (F vx, F vy, F vz) { x = vx; y = vy; z = vz; assert(this->IsValid()); }
    ILINE Vec3_tpl<F>& Set(const F xval, const F yval, const F zval) { x = xval; y = yval; z = zval; assert(this->IsValid()); return *this; }

    explicit ILINE Vec3_tpl(F f)
        : x(f)
        , y(f)
        , z(f) { assert(this->IsValid()); }

    explicit ILINE Vec3_tpl(const AZ::Vector3& v)
    {
        x = v.GetX();
        y = v.GetY();
        z = v.GetZ();
    }

    /*!
    * the copy/casting/assignement constructor
    *
    * Example:
    *  Vec3 v0=v1;
    *  Vec3 v0=Vec3(angle);
    *  Vec3 v0=Vec3(vector4);
    */
    ILINE Vec3_tpl(const Vec3_tpl& v)   {   x = v.x; y = v.y; z = v.z; }
    template<class F1>
    ILINE  Vec3_tpl<F>(const Vec3_tpl<F1>&v)  {
        x = F(v.x);
        y = F(v.y);
        z = F(v.z);
        assert(IsValid());
    }

    ILINE Vec3_tpl<F>(const Vec2_tpl<F>&v) {
        x = v.x;
        y = v.y;
        z = 0;
        assert(IsValid());
    }
    template<class T>
    ILINE Vec3_tpl<F>(const Vec2_tpl<T>&v) {
        x = F(v.x);
        y = F(v.y);
        z = 0;
        assert(IsValid());
    }

    explicit ILINE Vec3_tpl<F>(const Ang3_tpl<F>&v) {
        x = v.x;
        y = v.y;
        z = v.z;
        assert(IsValid());
    }
    template<class T>
    explicit ILINE Vec3_tpl<F>(const Ang3_tpl<T>&v) {
        x = v.x;
        y = v.y;
        z = v.z;
        assert(IsValid());
    }

    /*!
    * overloaded arithmetic operator
    *
    * Example:
    *  Vec3 v0=v1*4;
    */
    ILINE Vec3_tpl<F> operator * (F k) const
    {
        const Vec3_tpl<F> v = *this;
        return Vec3_tpl<F>(v.x * k, v.y * k, v.z * k);
    }
    ILINE Vec3_tpl<F> operator / (F k) const
    {
        k = (F)1.0 / k;
        return Vec3_tpl<F>(x * k, y * k, z * k);
    }
    ILINE friend Vec3_tpl<F> operator * (F f, const Vec3_tpl& vec)
    {
        return Vec3_tpl((F)(f * vec.x), (F)(f * vec.y), (F)(f * vec.z));
    }

    ILINE Vec3_tpl<F>& operator *= (F k)
    {
        x *= k;
        y *= k;
        z *= k;
        return *this;
    }
    ILINE Vec3_tpl<F>& operator /= (F k)
    {
        k = (F)1.0 / k;
        x *= k;
        y *= k;
        z *= k;
        return *this;
    }

    ILINE Vec3_tpl<F> operator - (void) const
    {
        return Vec3_tpl<F>(-x, -y, -z);
    }
    ILINE Vec3_tpl<F>& Flip()
    {
        x = -x;
        y = -y;
        z = -z;
        return *this;
    }


    /*!
    * bracked-operator
    *
    * Example:
    *  uint32 t=v[0];
    */
    ILINE F& operator [] (int32 index)        { assert(index >= 0 && index <= 2); return ((F*)this)[index]; }
    ILINE F operator [] (int32 index) const { assert(index >= 0 && index <= 2); return ((F*)this)[index]; }


    /*!
    * functions and operators to compare vector
    *
    * Example:
    *  if (v0==v1) dosomething;
    */
    ILINE bool operator==(const Vec3_tpl<F>& vec)
    {
        return x == vec.x && y == vec.y && z == vec.z;
    }
    ILINE bool operator!=(const Vec3_tpl<F>& vec)
    {
        return !(*this == vec);
    }

    ILINE friend bool operator ==(const Vec3_tpl<F>& v0, const Vec3_tpl<F>& v1)
    {
        return ((v0.x == v1.x) && (v0.y == v1.y) && (v0.z == v1.z));
    }
    ILINE friend bool operator !=(const Vec3_tpl<F>& v0, const Vec3_tpl<F>& v1)
    {
        return !(v0 == v1);
    }

    ILINE bool IsZero(F e = (F) 0.0) const
    {
        return (fabs_tpl(x) <= e) && (fabs_tpl(y) <= e) && (fabs_tpl(z) <= e);
    }

    ILINE bool IsZeroFast(F e = (F) 0.0003) const
    {
        return (fabs_tpl(x) + fabs_tpl(y) + fabs_tpl(z)) <= e;
    }

    // Chebyshev distance (axis aligned)
    ILINE bool IsEquivalent(const Vec3_tpl<F>& v1, F epsilon = VEC_EPSILON) const
    {
        assert(v1.IsValid());
        assert(this->IsValid());
        return  ((fabs_tpl(x - v1.x) <= epsilon) &&   (fabs_tpl(y - v1.y) <= epsilon) && (fabs_tpl(z - v1.z) <= epsilon));
    }
    ILINE static bool IsEquivalent(const Vec3_tpl<F>& v0, const Vec3_tpl<F>& v1, F epsilon = VEC_EPSILON)
    {
        assert(v0.IsValid());
        assert(v1.IsValid());
        return  ((fabs_tpl(v0.x - v1.x) <= epsilon) &&    (fabs_tpl(v0.y - v1.y) <= epsilon) &&  (fabs_tpl(v0.z - v1.z) <= epsilon));
    }

    // Euclidean distance L2
    ILINE bool IsEquivalentL2(const Vec3_tpl<F>& v1, F epsilon = VEC_EPSILON) const
    {
        assert(v1.IsValid());
        assert(this->IsValid());
        return (*this - v1).GetLengthSquared() <= (epsilon * epsilon);
    }
    ILINE static bool IsEquivalentL2(const Vec3_tpl<F>& v0, const Vec3_tpl<F>& v1, F epsilon = VEC_EPSILON)
    {
        assert(v0.IsValid());
        assert(v1.IsValid());
        return (v0 - v1).GetLengthSquared() <= (epsilon * epsilon);
    }

    ILINE bool IsUnit(F epsilon = VEC_EPSILON) const
    {
        return (fabs_tpl(1 - GetLengthSquared()) <= epsilon);
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
        return true;
    }

    //! force vector length by normalizing it
    ILINE void SetLength(F fLen)
    {
        F fLenMe = GetLengthSquared();
        if (fLenMe < 0.00001f * 0.00001f)
        {
            return;
        }
        fLenMe = fLen * isqrt_tpl(fLenMe);
        x *= fLenMe;
        y *= fLenMe;
        z *= fLenMe;
    }

    ILINE void ClampLength(F maxLength)
    {
        F sqrLength = GetLengthSquared();
        if (sqrLength > (maxLength * maxLength))
        {
            F scale = maxLength * isqrt_tpl(sqrLength);
            x *= scale;
            y *= scale;
            z *= scale;
        }
    }

    //! calculate the length of the vector
    ILINE F GetLength() const
    {
        return sqrt_tpl(x * x + y * y + z * z);
    }

    ILINE F GetLengthFloat() const
    {
        return GetLength();
    }

    ILINE F GetLengthFast() const
    {
        return sqrt_fast_tpl(x * x + y * y + z * z);
    }

    //! calculate the squared length of the vector
    ILINE F GetLengthSquared() const
    {
        return x * x + y * y + z * z;
    }

    ILINE F GetLengthSquaredFloat() const
    {
        return GetLengthSquared();
    }

    //! calculate the length of the vector ignoring the z component
    ILINE F GetLength2D() const
    {
        return sqrt_tpl(x * x + y * y);
    }

    //! calculate the squared length of the vector ignoring the z component
    ILINE F GetLengthSquared2D() const
    {
        return x * x + y * y;
    }

    ILINE F GetDistance(const Vec3_tpl<F>& vec1) const
    {
        return sqrt_tpl((x - vec1.x) * (x - vec1.x) + (y - vec1.y) * (y - vec1.y) + (z - vec1.z) * (z - vec1.z));
    }
    ILINE F GetSquaredDistance (const Vec3_tpl<F>& v) const
    {
        return (x - v.x) * (x - v.x) + (y - v.y) * (y - v.y) + (z - v.z) * (z - v.z);
    }
    ILINE F GetSquaredDistance2D (const Vec3_tpl<F>& v) const
    {
        return (x - v.x) * (x - v.x) + (y - v.y) * (y - v.y);
    }

    //! Normalize the vector.
    // The default Normalize function is in fact "safe". 0 vectors remain unchanged.
    ILINE void  Normalize()
    {
        assert(this->IsValid());
        F fInvLen = isqrt_safe_tpl(x * x + y * y + z * z);
        x *= fInvLen;
        y *= fInvLen;
        z *= fInvLen;
    }

    //! May be faster and less accurate.
    ILINE void NormalizeFast()
    {
        assert(this->IsValid());
        F fInvLen = isqrt_fast_tpl(x * x + y * y + z * z);
        x *= fInvLen;
        y *= fInvLen;
        z *= fInvLen;
    }

    //! Normalize the vector to a scale.
    ILINE void Normalize(F scale)
    {
        assert(this->IsValid());
        F fInvLen = isqrt_safe_tpl(x * x + y * y + z * z) * scale;
        x *= fInvLen;
        y *= fInvLen;
        z *= fInvLen;
    }
    ILINE void NormalizeFast(F scale)
    {
        assert(this->IsValid());
        F fInvLen = isqrt_fast_tpl(x * x + y * y + z * z) * scale;
        x *= fInvLen;
        y *= fInvLen;
        z *= fInvLen;
    }

    //! Normalize the vector.
    // check for null vector - set to the passed in vector (which should be normalised!) if it is null vector
    // returns the original length of the vector
    ILINE F NormalizeSafe(const struct Vec3_tpl<F>& safe = Vec3_tpl<F>(0, 0, 0))
    {
        assert(this->IsValid());
        F fLen2 = x * x + y * y + z * z;
        IF (VecPrecisionValues<F>::CheckGreater(fLen2), 1)
        {
            F fInvLen = isqrt_tpl(fLen2);
            x *= fInvLen;
            y *= fInvLen;
            z *= fInvLen;
            return F(1) / fInvLen;
        }
        else
        {
            *this = safe;
            return F(0);
        }
    }

    ILINE Vec3_tpl GetNormalizedFloat() const
    {
        return GetNormalized();
    }

    //! Return a normalized vector.
    ILINE Vec3_tpl GetNormalized() const
    {
        F fInvLen = isqrt_safe_tpl(x * x + y * y + z * z);
        return *this * fInvLen;
    }

    //! Return a normalized vector.
    ILINE Vec3_tpl GetNormalizedFast() const
    {
        F fInvLen = isqrt_fast_tpl(x * x + y * y + z * z);
        return *this * fInvLen;
    }

    //! Return a safely normalized vector - returns safe vector (should be normalised) if original is zero length.
    ILINE Vec3_tpl GetNormalizedSafe(const struct Vec3_tpl<F>& safe = Vec3_tpl<F>(1, 0, 0)) const
    {
        F fLen2 = x * x + y * y + z * z;
        IF (VecPrecisionValues<F>::CheckGreater(fLen2), 1)
        {
            F fInvLen = isqrt_tpl(fLen2);
            return *this * fInvLen;
        }
        else
        {
            return safe;
        }
    }

    //! Return a safely normalized vector - returns safe vector (should be normalised) if original is zero length.
    ILINE Vec3_tpl GetNormalizedSafeFloat(const struct Vec3_tpl<F>& safe = Vec3_tpl<F>(1, 0, 0)) const
    {
        return GetNormalizedSafe(safe);
    }

    //! Return a normalized and scaled vector.
    ILINE Vec3_tpl GetNormalized(F scale) const
    {
        F fInvLen = isqrt_safe_tpl(x * x + y * y + z * z);
        return *this * (fInvLen * scale);
    }
    ILINE Vec3_tpl GetNormalizedFast(F scale) const
    {
        F fInvLen = isqrt_fast_tpl(x * x + y * y + z * z);
        return *this * (fInvLen * scale);
    }

    //! Permutate coordinates so that z goes to new_z slot.
    ILINE Vec3_tpl GetPermutated(int new_z) const { return Vec3_tpl(*(&x + inc_mod3[new_z]), *(&x + dec_mod3[new_z]), *(&x + new_z)); }

    //! Returns volume of a box with this vector as diagonal.
    ILINE F GetVolume() const { return x * y * z; }

    //! Returns a vector that consists of absolute values of this one's coordinates.
    ILINE Vec3_tpl<F> abs() const
    {
        return Vec3_tpl(fabs_tpl(x), fabs_tpl(y), fabs_tpl(z));
    }

    //! Check for min bounds.
    ILINE void CheckMin(const Vec3_tpl<F> other)
    {
        x = min(other.x, x);
        y = min(other.y, y);
        z = min(other.z, z);
    }
    //! Check for max bounds.
    ILINE void CheckMax(const Vec3_tpl<F> other)
    {
        x = max(other.x, x);
        y = max(other.y, y);
        z = max(other.z, z);
    }



    /*!
    * Sets a vector orthogonal to the input vector.
    *
    * Example:
    *  Vec3 v;
    *  v.SetOrthogonal( i );
    */
    ILINE void SetOrthogonal(const Vec3_tpl<F>& v)
    {
        sqr(F(0.9)) * (v | v) - v.x * v.x < 0 ? (x = -v.z, y = 0, z = v.x) : (x = 0, y = v.z, z = -v.y);
    }
    //! Returns a vector orthogonal to this one.
    ILINE Vec3_tpl GetOrthogonal() const
    {
        return sqr(F(0.9)) * (x * x + y * y + z * z) - x * x < 0 ? Vec3_tpl<F>(-z, 0, x) : Vec3_tpl<F>(0, z, -y);
    }

    /*!
    * Project a point/vector on a (virtual) plane
    * Consider we have a plane going through the origin.
    * Because d=0 we need just the normal. The vector n is assumed to be a unit-vector.
    *
    * Example:
    *  Vec3 result=Vec3::CreateProjection( incident, normal );
    */
    ILINE void SetProjection(const Vec3_tpl& i, const Vec3_tpl& n)
    {
        *this = i - n * (n | i);
    }
    ILINE static Vec3_tpl<F> CreateProjection(const Vec3_tpl& i, const Vec3_tpl& n)
    {
        return i - n * (n | i);
    }

    /*!
    * Calculate a reflection vector. Vec3 n is assumed to be a unit-vector.
    *
    * Example:
    *  Vec3 result=Vec3::CreateReflection( incident, normal );
    */
    ILINE void SetReflection(const Vec3_tpl<F>& i, const Vec3_tpl<F>& n)
    {
        *this = (n * (i | n) * 2) - i;
    }
    ILINE static Vec3_tpl<F> CreateReflection(const Vec3_tpl<F>& i, const Vec3_tpl<F>& n)
    {
        return (n * (i | n) * 2) - i;
    }

    /*!
    * Linear-Interpolation between Vec3 (lerp)
    *
    * Example:
    *  Vec3 r=Vec3::CreateLerp( p, q, 0.345f );
    */
    ILINE void SetLerp(const Vec3_tpl<F>& p, const Vec3_tpl<F>& q, F t)
    {
        const Vec3_tpl<F> diff = q - p;
        *this = p + (diff * t);
    }
    ILINE static Vec3_tpl<F> CreateLerp(const Vec3_tpl<F>& p, const Vec3_tpl<F>& q, F t)
    {
        const Vec3_tpl<F> diff = q - p;
        return p + (diff * t);
    }


    /*!
    * Spherical-Interpolation between 3d-vectors (geometrical slerp)
    * both vectors are assumed to be normalized.
    *
    * Example:
    *  Vec3 r=Vec3::CreateSlerp( p, q, 0.5674f );
    */
    void SetSlerp(const Vec3_tpl<F>& p, const Vec3_tpl<F>& q, F t)
    {
        assert(p.IsUnit(0.005f));
        assert(q.IsUnit(0.005f));
        // calculate cosine using the "inner product" between two vectors: p*q=cos(radiant)
        F cosine = clamp_tpl((p | q), F(-1), F(1));
        //we explore the special cases where the both vectors are very close together,
        //in which case we approximate using the more economical LERP and avoid "divisions by zero" since sin(Angle) = 0  as   Angle=0
        if (cosine >= (F)0.99)
        {
            SetLerp(p, q, t); //perform LERP:
            Normalize();
        }
        else
        {
            //perform SLERP: because of the LERP-check above, a "division by zero" is not possible
            F rad               = acos_tpl(cosine);
            F scale_0   = sin_tpl((1 - t) * rad);
            F scale_1   = sin_tpl(t * rad);
            *this = (p * scale_0 + q * scale_1) / sin_tpl(rad);
            Normalize();
        }
    }
    ILINE static Vec3_tpl<F> CreateSlerp(const Vec3_tpl<F>& p, const Vec3_tpl<F>& q, F t)
    {
        Vec3_tpl<F> v;
        v.SetSlerp(p, q, t);
        return v;
    }




    /*!
        * Quadratic-Interpolation between vectors v0,v1,v2.
        * This is repeated linear interpolation from 3 points and the resulting curve is a parabola.
        * If t is in the range [0...1], then the curve goes only through v0 and v2.
        *
        * Example:
        *  Vec3 ip; ip.SetQuadraticCurve( v0,v1,v2, 0.345f );
        */
    ILINE void SetQuadraticCurve(const Vec3_tpl<F>& v0, const Vec3_tpl<F>& v1, const Vec3_tpl<F>& v2, F t1)
    {
        F t0 = 1.0f - t1;
        *this = t0 * t0 * v0 + t0 * t1 * 2.0f * v1 + t1 * t1 * v2;
    }
    ILINE static Vec3_tpl<F> CreateQuadraticCurve(const Vec3_tpl<F>& v0, const Vec3_tpl<F>& v1, const Vec3_tpl<F>& v2, F t)
    {
        Vec3_tpl<F> ip;
        ip.SetQuadraticCurve(v0, v1, v2, t);
        return ip;
    }

    /*!
    * Cubic-Interpolation between vectors v0,v1,v2,v3.
    * This is repeated linear interpolation from 4 points.
    * If t is in the range [0...1], then the curve goes only through v0 and v3.
    *
    * Example:
    *  Vec3 ip; ip.SetCubicCurve( v0,v1,v2,v3, 0.345f );
    */
    ILINE void SetCubicCurve(const Vec3_tpl<F>& v0, const Vec3_tpl<F>& v1, const Vec3_tpl<F>& v2, const Vec3_tpl<F>& v3, F t1)
    {
        F t0 = 1.0f - t1;
        *this = t0 * t0 * t0 * v0 + 3 * t0 * t0 * t1 * v1 + 3 * t0 * t1 * t1 * v2 + t1 * t1 * t1 * v3;
    }
    ILINE static Vec3_tpl<F> CreateCubicCurve(const Vec3_tpl<F>& v0, const Vec3_tpl<F>& v1, const Vec3_tpl<F>& v2, const Vec3_tpl<F>& v3, F t)
    {
        Vec3_tpl<F> ip;
        ip.SetCubicCurve(v0, v1, v2, v3, t);
        return ip;
    }

    /*!
       * Spline-Interpolation between vectors v0,v1,v2.
       * This is a variation of a quadratic curve.
       * If t is in the range [0...1], then the spline goes through all 3 points
       *
       * Example:
       *  Vec3 ip; ip.SetSplineInterpolation( v0,v1,v2, 0.345f );
       */
    ILINE void SetQuadraticSpline(const Vec3_tpl<F>& v0, const Vec3_tpl<F>& v1, const Vec3_tpl<F>& v2, F t)
    {
        SetQuadraticCurve(v0, v1 - (v0 * 0.5f + v1 + v2 * 0.5f - v1 * 2.0f), v2, t);
    }
    ILINE static Vec3_tpl<F> CreateQuadraticSpline(const Vec3_tpl<F>& v0, const Vec3_tpl<F>& v1, const Vec3_tpl<F>& v2, F t)
    {
        Vec3_tpl<F> ip;
        ip.SetQuadraticSpline(v0, v1, v2, t);
        return ip;
    }

    /*!
    * Rotate a vector using angle&axis.
    *
    * Example:
    *  Vec3 r=v.GetRotated(axis,angle);
    */
    ILINE Vec3_tpl<F> GetRotated(const Vec3_tpl<F>& axis, F angle) const
    {
        return GetRotated(axis, cos_tpl(angle), sin_tpl(angle));
    }
    ILINE Vec3_tpl<F> GetRotated(const Vec3_tpl<F>& axis, F cosa, F sina) const
    {
        Vec3_tpl<F> zax = axis * (*this | axis);
        Vec3_tpl<F> xax = *this - zax;
        Vec3_tpl<F> yax = axis % xax;
        return xax * cosa + yax * sina + zax;
    }

    /*!
    * Rotate a vector around a center using angle&axis.
    *
    * Example:
    *  Vec3 r=v.GetRotated(axis,angle);
    */
    ILINE Vec3_tpl<F> GetRotated(const Vec3_tpl& center, const Vec3_tpl<F>& axis, F angle) const
    {
        return center + (*this - center).GetRotated(axis, angle);
    }
    ILINE Vec3_tpl<F> GetRotated(const Vec3_tpl<F>& center, const Vec3_tpl<F>& axis, F cosa, F sina) const
    {
        return center + (*this - center).GetRotated(axis, cosa, sina);
    }

    /*!
    * Component wise multiplication of two vectors.
    */
    ILINE Vec3_tpl CompMul(const Vec3_tpl<F> rhs) const
    {
        return(Vec3_tpl(x * rhs.x, y * rhs.y, z * rhs.z));
    }

    //! Three methods for a "dot-product" operation.
    ILINE F Dot (const Vec3_tpl<F> v)   const
    {
        return x * v.x + y * v.y + z * v.z;
    }
    //! Two methods for a "cross-product" operation.
    ILINE Vec3_tpl<F> Cross (const Vec3_tpl<F> vec2) const
    {
        return Vec3_tpl<F>(y * vec2.z  -  z * vec2.y,     z * vec2.x -    x * vec2.z,   x * vec2.y  -  y * vec2.x);
    }

    //f32* fptr=vec;
    DEPRECATED operator F* ()                   { return (F*)this; }
    template <class T>
    explicit DEPRECATED Vec3_tpl(const T* src) { x = F(src[0]); y = F(src[1]); z = F(src[2]); }

    ILINE Vec3_tpl& zero() { x = y = z = 0; return *this; }
    ILINE F len() const
    {
        return sqrt_tpl(x * x + y * y + z * z);
    }
    ILINE F len2() const
    {
        return x * x + y * y + z * z;
    }

    ILINE Vec3_tpl& normalize()
    {
        F len2 = x * x + y * y + z * z;
        if (len2 > (F)1e-20f)
        {
            F rlen = isqrt_tpl(len2);
            x *= rlen;
            y *= rlen;
            z *= rlen;
        }
        else
        {
            Set(0, 0, 1);
        }
        return *this;
    }
    ILINE Vec3_tpl normalized() const
    {
        F len2 = x * x + y * y + z * z;
        if (len2 > (F)1e-20f)
        {
            F rlen = isqrt_tpl(len2);
            return Vec3_tpl(x * rlen, y * rlen, z * rlen);
        }
        else
        {
            return Vec3_tpl(0, 0, 1);
        }
    }

    //vector subtraction
    template<class F1>
    ILINE Vec3_tpl<F1> sub(const Vec3_tpl<F1>& v) const
    {
        return Vec3_tpl<F1>(x - v.x, y - v.y, z - v.z);
    }
    //vector scale
    template<class F1>
    ILINE Vec3_tpl<F1> scale(const F1 k) const
    {
        return Vec3_tpl<F>(x * k, y * k, z * k);
    }

    //vector dot product
    template<class F1>
    ILINE F1 dot(const Vec3_tpl<F1>& v) const
    {
        return (F1)(x * v.x + y * v.y + z * v.z);
    }
    //vector cross product
    template<class F1>
    ILINE Vec3_tpl<F1> cross(const Vec3_tpl<F1>& v) const
    {
        return Vec3_tpl<F1>(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
    }
};

using Vec3i = Vec3_tpl<int32>;

// dot product (2 versions)
template<class F1, class F2>
ILINE F1 operator * (const Vec3_tpl<F1>& v0, const Vec3_tpl<F2>& v1)
{
    return v0.Dot(v1);
}
template<class F1, class F2>
ILINE F1 operator | (const Vec3_tpl<F1>& v0, const Vec3_tpl<F2>& v1)
{
    return v0.Dot(v1);
}
// cross product (2 versions)
template<class F1, class F2>
ILINE Vec3_tpl<F1> operator ^ (const Vec3_tpl<F1>& v0, const Vec3_tpl<F2>& v1)
{
    return v0.Cross(v1);
}
template<class F1, class F2>
ILINE Vec3_tpl<F1> operator % (const Vec3_tpl<F1>& v0, const Vec3_tpl<F2>& v1)
{
    return v0.Cross(v1);
}


//vector addition
template<class F1, class F2>
ILINE Vec3_tpl<F1> operator + (const Vec3_tpl<F1>& v0, const Vec3_tpl<F2>& v1)
{
    return Vec3_tpl<F1>(static_cast<F1>(v0.x + v1.x), static_cast<F1>(v0.y + v1.y), static_cast<F1>(v0.z + v1.z));
}
//vector addition
template<class F1, class F2>
ILINE Vec3_tpl<F1> operator + (const Vec2_tpl<F1>& v0, const Vec3_tpl<F2>& v1)
{
    return Vec3_tpl<F1>(v0.x + v1.x, v0.y + v1.y, v1.z);
}
//vector addition
template<class F1, class F2>
ILINE Vec3_tpl<F1> operator + (const Vec3_tpl<F1>& v0, const Vec2_tpl<F2>& v1)
{
    return Vec3_tpl<F1>(v0.x + v1.x, v0.y + v1.y, v0.z);
}

//vector subtraction
template<class F1, class F2>
ILINE Vec3_tpl<F1> operator - (const Vec3_tpl<F1>& v0, const Vec3_tpl<F2>& v1)
{
    return Vec3_tpl<F1>((F1)(v0.x - v1.x), (F1)(v0.y - v1.y), (F1)(v0.z - v1.z));
}
template<class F1, class F2>
ILINE Vec3_tpl<F1> operator - (const Vec2_tpl<F1>& v0, const Vec3_tpl<F2>& v1)
{
    return Vec3_tpl<F1>(v0.x - v1.x, v0.y - v1.y, 0.0f - v1.z);
}
template<class F1, class F2>
ILINE Vec3_tpl<F1> operator - (const Vec3_tpl<F1>& v0, const Vec2_tpl<F2>& v1)
{
    return Vec3_tpl<F1>(v0.x - v1.x, v0.y - v1.y, v0.z);
}


//---------------------------------------------------------------------------


//vector self-addition
template<class F1, class F2>
ILINE Vec3_tpl<F1>& operator += (Vec3_tpl<F1>& v0, const Vec3_tpl<F2>& v1)
{
    v0 = v0 + v1;
    return v0;
}
//vector self-subtraction
template<class F1, class F2>
ILINE Vec3_tpl<F1>& operator -= (Vec3_tpl<F1>& v0, const Vec3_tpl<F2>& v1)
{
    v0 = v0 - v1;
    return v0;
}
template<class F1, class F2>
ILINE Vec3_tpl<F1> operator / (const Vec3_tpl<F1>& v0, const Vec3_tpl<F2>& v1)
{
    return Vec3_tpl<F1>(v0.x / v1.x, v0.y / v1.y, v0.z / v1.z);
}

template <class F>
ILINE bool IsEquivalent(const Vec3_tpl<F>& v0, const Vec3_tpl<F>& v1, f32 epsilon = VEC_EPSILON)
{
    return  ((fabs_tpl(v0.x - v1.x) <= epsilon) &&    (fabs_tpl(v0.y - v1.y) <= epsilon) &&  (fabs_tpl(v0.z - v1.z) <= epsilon));
}


///////////////////////////////////////////////////////////////////////////////
// Typedefs                                                                  //
///////////////////////////////////////////////////////////////////////////////
typedef Vec3_tpl<f32>    Vec3;  // always 32 bit

template<>
inline Vec3_tpl<f32>::Vec3_tpl(type_min) { x = y = z = -3.3E38f; }
template<>
inline Vec3_tpl<f32>::Vec3_tpl(type_max) { x = y = z = 3.3E38f; }


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// struct Ang3_tpl
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template <typename F>
struct Ang3_tpl
{
    F x, y, z;

#if defined(_DEBUG)
    ILINE Ang3_tpl()
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
    ILINE Ang3_tpl()    {};
#endif


    Ang3_tpl(type_zero) { x = y = z = 0; }

    void operator () (F vx, F vy, F vz) { x = vx; y = vy; z = vz; };
    ILINE Ang3_tpl<F>(F vx, F vy, F vz)   {
        x = vx;
        y = vy;
        z = vz;
    }

    explicit ILINE Ang3_tpl(const Vec3_tpl<F>& v)
        : x((F)v.x)
        , y((F)v.y)
        , z((F)v.z) { assert(this->IsValid()); }

    ILINE bool operator==(const Ang3_tpl<F>& vec) { return x == vec.x && y == vec.y && z == vec.z; }
    ILINE bool operator!=(const Ang3_tpl<F>& vec) { return !(*this == vec); }

    ILINE Ang3_tpl<F> operator * (F k) const { return Ang3_tpl<F>(x * k, y * k, z * k); }
    ILINE Ang3_tpl<F> operator / (F k) const { k = (F)1.0 / k; return Ang3_tpl<F>(x * k, y * k, z * k); }


    ILINE Ang3_tpl<F>& operator *= (F k) { x *= k; y *= k; z *= k; return *this; }
    //explicit ILINE Ang3_tpl<F>& operator = (const Vec3_tpl<F>& v)  { x=v.x; y=v.y; z=v.z; return *this;   }

    ILINE Ang3_tpl<F> operator - (void) const { return Ang3_tpl<F>(-x, -y, -z); }

    ILINE friend bool operator ==(const Ang3_tpl<F>& v0, const Ang3_tpl<F>& v1)
    {
        return ((v0.x == v1.x) && (v0.y == v1.y) && (v0.z == v1.z));
    }
    ILINE void Set(F xval, F yval, F zval) { x = xval; y = yval; z = zval; }

    ILINE bool IsEquivalent(const Ang3_tpl<F>& v1, F epsilon = VEC_EPSILON) const
    {
        return  ((fabs_tpl(x - v1.x) <= epsilon) &&   (fabs_tpl(y - v1.y) <= epsilon) && (fabs_tpl(z - v1.z) <= epsilon));
    }
    ILINE bool IsInRangePI() const
    {
        F pi = (F)(gf_PI + 0.001); //we need this to fix fp-precision problem
        return  ((x > -pi) && (x < pi) && (y > -pi) && (y < pi) && (z > -pi) && (z < pi));
    }

    //! Normalize angle to -pi and +pi range.
    ILINE void RangePI()
    {
        const F modX = fmod(x + gf_PI, gf_PI2);
        x = if_neg_else(modX, modX + gf_PI, modX - gf_PI);

        const F modY = fmod(y + gf_PI, gf_PI2);
        y = if_neg_else(modY, modY + gf_PI, modY - gf_PI);

        const F modZ = fmod(z + gf_PI, gf_PI2);
        z = if_neg_else(modZ, modZ + gf_PI, modZ - gf_PI);
    }

    //! Convert unit Quat to Euler Angles (xyz).
    template<class F1>
    explicit Ang3_tpl(const Quat_tpl<F1>& q)
    {
        assert(q.IsValid());
        y = F(asin_tpl(max((F)-1.0, min((F)1.0, -(q.v.x * q.v.z - q.w * q.v.y) * 2))));
        if (fabs_tpl(fabs_tpl(y) - (F)((F)g_PI * (F)0.5)) < (F)0.01)
        {
            x = F(0);
            z = F(atan2_tpl(-2 * (q.v.x * q.v.y - q.w * q.v.z), 1 - (q.v.x * q.v.x + q.v.z * q.v.z) * 2));
        }
        else
        {
            x = F(atan2_tpl((q.v.y * q.v.z + q.w * q.v.x) * 2, 1 - (q.v.x * q.v.x + q.v.y * q.v.y) * 2));
            z = F(atan2_tpl((q.v.x * q.v.y + q.w * q.v.z) * 2, 1 - (q.v.z * q.v.z + q.v.y * q.v.y) * 2));
        }
    }

    //! Convert Matrix33 to Euler Angles (xyz).
    template<class F1>
    explicit Ang3_tpl(const Matrix33_tpl<F1>& m)
    {
        assert(m.IsOrthonormalRH(0.001f));
        y = (F)asin_tpl(max((F)-1.0, min((F)1.0, -m.m20)));
        if (fabs_tpl(fabs_tpl(y) - (F)((F)g_PI * (F)0.5)) < (F)0.01)
        {
            x = F(0);
            z = F(atan2_tpl(-m.m01, m.m11));
        }
        else
        {
            x = F(atan2_tpl(m.m21, m.m22));
            z = F(atan2_tpl(m.m10, m.m00));
        }
    }

    //! Convert Matrix34 to Euler Angles (xyz).
    template<class F1>
    explicit Ang3_tpl(const Matrix34_tpl<F1>& m)
    {
        assert(m.IsOrthonormalRH(0.001f));
        y = F(asin_tpl(max((F)-1.0, min((F)1.0, -m.m20))));
        if (fabs_tpl(fabs_tpl(y) - (F)((F)g_PI * (F)0.5)) < (F)0.01)
        {
            x = F(0);
            z = F(atan2_tpl(-m.m01, m.m11));
        }
        else
        {
            x = F(atan2_tpl(m.m21, m.m22));
            z = F(atan2_tpl(m.m10, m.m00));
        }
    }

    //! Convert Matrix44 to Euler Angles (xyz).
    template<class F1>
    explicit Ang3_tpl(const Matrix44_tpl<F1>& m)
    {
        assert(Matrix33(m).IsOrthonormalRH(0.001f));
        y = F(asin_tpl(max((F)-1.0, min((F)1.0, -m.m20))));
        if (fabs_tpl(fabs_tpl(y) - (F)((F)g_PI * (F)0.5)) < (F)0.01)
        {
            x = F(0);
            z = F(atan2_tpl(-m.m01, m.m11));
        }
        else
        {
            x = F(atan2_tpl(m.m21, m.m22));
            z = F(atan2_tpl(m.m10, m.m00));
        }
    }

    template<typename F1>
    static ILINE F CreateRadZ(const Vec2_tpl<F1>& v0, const Vec2_tpl<F1>& v1)
    {
        F cz    = v0.x * v1.y - v0.y * v1.x;
        F c     =   v0.x * v1.x + v0.y * v1.y;
        return F(atan2_tpl(cz, c));
    }

    template<typename F1>
    static ILINE F CreateRadZ(const Vec3_tpl<F1>& v0, const Vec3_tpl<F1>& v1)
    {
        F cz    = v0.x * v1.y - v0.y * v1.x;
        F c     =   v0.x * v1.x + v0.y * v1.y;
        return F(atan2_tpl(cz, c));
    }

    template<typename F1>
    ILINE static Ang3_tpl<F> GetAnglesXYZ(const Quat_tpl<F1>& q) {    return Ang3_tpl<F>(q); }
    template<typename F1>
    ILINE void SetAnglesXYZ(const Quat_tpl<F1>& q)    {   *this = Ang3_tpl<F>(q);   }

    template<typename F1>
    ILINE static Ang3_tpl<F> GetAnglesXYZ(const Matrix33_tpl<F1>& m) {    return Ang3_tpl<F>(m); }
    template<typename F1>
    ILINE void SetAnglesXYZ(const Matrix33_tpl<F1>& m)    {   *this = Ang3_tpl<F>(m);   }

    template<typename F1>
    ILINE static Ang3_tpl<F> GetAnglesXYZ(const Matrix34_tpl<F1>& m) {    return Ang3_tpl<F>(m); }
    template<typename F1>
    ILINE void SetAnglesXYZ(const Matrix34_tpl<F1>& m)    {   *this = Ang3_tpl<F>(m);   }

    //---------------------------------------------------------------
    ILINE F& operator [] (int index)          { assert(index >= 0 && index <= 2); return ((F*)this)[index]; }
    ILINE F operator [] (int index) const { assert(index >= 0 && index <= 2); return ((F*)this)[index]; }


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
        return true;
    }
};

typedef Ang3_tpl<f32>       Ang3;

//---------------------------------------

//vector addition
template<class F1, class F2>
ILINE Ang3_tpl<F1> operator + (const Ang3_tpl<F1>& v0, const Ang3_tpl<F2>& v1)
{
    return Ang3_tpl<F1>(v0.x + v1.x, v0.y + v1.y, v0.z + v1.z);
}
//vector subtraction
template<class F1, class F2>
ILINE Ang3_tpl<F1> operator - (const Ang3_tpl<F1>& v0, const Ang3_tpl<F2>& v1)
{
    return Ang3_tpl<F1>(v0.x - v1.x, v0.y - v1.y, v0.z - v1.z);
}

//---------------------------------------

//vector self-addition
template<class F1, class F2>
ILINE Ang3_tpl<F1>& operator += (Ang3_tpl<F1>& v0, const Ang3_tpl<F2>& v1)
{
    v0 = v0 + v1;
    return v0;
}
//vector self-subtraction
template<class F1, class F2>
ILINE Ang3_tpl<F1>& operator -= (Ang3_tpl<F1>& v0, const Ang3_tpl<F2>& v1)
{
    v0 = v0 - v1;
    return v0;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// struct CAngleAxis
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
template <typename F>
struct AngleAxis_tpl
{
    //! storage for the Angle&Axis coordinates.
    F angle;
    Vec3_tpl<F> axis;

    // default quaternion constructor
    AngleAxis_tpl(void) { };
    AngleAxis_tpl(F a, F ax, F ay, F az) {  angle = a; axis.x = ax; axis.y = ay; axis.z = az; }
    AngleAxis_tpl(F a, const Vec3_tpl<F>& n) { angle = a; axis = n; }
    void operator () (F a, const Vec3_tpl<F>& n) {  angle = a; axis = n; }
    AngleAxis_tpl(const AngleAxis_tpl<F>& aa);   //CAngleAxis aa=angleaxis
    const Vec3_tpl<F> operator * (const Vec3_tpl<F>& v) const;

    AngleAxis_tpl(const Quat_tpl<F>& q)
    {
        angle = acos_tpl(q.w) * 2;
        axis    = q.v;
        axis.Normalize();
        F s = sin_tpl(angle * (F)0.5);
        if (s == 0)
        {
            angle = 0;
            axis.x = 0;
            axis.y = 0;
            axis.z = 1;
        }
    }
};

typedef AngleAxis_tpl<f32> AngleAxis;

template<typename F>
ILINE const Vec3_tpl<F> AngleAxis_tpl<F>::operator * (const Vec3_tpl<F>& v) const
{
    Vec3_tpl<F> origin  = axis * (axis | v);
    return origin +  (v - origin) * cos_tpl(angle)  +  (axis % v) * sin_tpl(angle);
}

//////////////////////////////////////////////////////////////////////
template<typename F>
struct Plane_tpl
{
    //plane-equation: n.x*x + n.y*y + n.z*z + d > 0 is in front of the plane
    Vec3_tpl<F> n;  //!< normal
    F   d;                      //!< distance

    //----------------------------------------------------------------

#if defined(_DEBUG)
    ILINE Plane_tpl()
    {
        if constexpr (sizeof(F) == 4)
        {
            uint32* p = alias_cast<uint32*>(&n.x);
            p[0] = F32NAN;
            p[1] = F32NAN;
            p[2] = F32NAN;
            p[3] = F32NAN;
        }
        if constexpr (sizeof(F) == 8)
        {
            uint64* p = alias_cast<uint64*>(&n.x);
            p[0] = F64NAN;
            p[1] = F64NAN;
            p[2] = F64NAN;
            p[3] = F64NAN;
        }
    }
#else
    ILINE Plane_tpl()   {};
#endif


    ILINE Plane_tpl(const Plane_tpl<F>& p) {  n = p.n; d = p.d; }
    ILINE Plane_tpl(const Vec3_tpl<F>& normal, const F& distance) {  n = normal; d = distance; }

    //! set normal and dist for this plane and  then calculate plane type
    ILINE void Set(const Vec3_tpl<F>& vNormal, const F fDist)
    {
        n = vNormal;
        d = fDist;
    }

    ILINE void SetPlane(const Vec3_tpl<F>& normal, const Vec3_tpl<F>& point)
    {
        n = normal;
        d = -(point | normal);
    }
    ILINE static Plane_tpl<F> CreatePlane(const Vec3_tpl<F>& normal, const Vec3_tpl<F>& point)
    {
        return Plane_tpl<F>(normal, -(point | normal));
    }

    ILINE Plane_tpl<F> operator - (void) const { return Plane_tpl<F>(-n, -d); }

    /*!
    * Constructs the plane by tree given Vec3s (=triangle) with a right-hand (anti-clockwise) winding
    *
    * Example 1:
    *  Vec3 v0(1,2,3),v1(4,5,6),v2(6,5,6);
    *  Plane_tpl<F>  plane;
    *  plane.SetPlane(v0,v1,v2);
    *
    * Example 2:
    *  Vec3 v0(1,2,3),v1(4,5,6),v2(6,5,6);
    *  Plane_tpl<F>  plane=Plane_tpl<F>::CreatePlane(v0,v1,v2);
    */
    ILINE void SetPlane(const Vec3_tpl<F>& v0, const Vec3_tpl<F>& v1, const Vec3_tpl<F>& v2)
    {
        n = ((v1 - v0) % (v2 - v0)).GetNormalized();  //vector cross-product
        d   =   -(n | v0);              //calculate d-value
    }
    ILINE static Plane_tpl<F> CreatePlane(const Vec3_tpl<F>& v0, const Vec3_tpl<F>& v1, const Vec3_tpl<F>& v2)
    {
        Plane_tpl<F> p;
        p.SetPlane(v0, v1, v2);
        return p;
    }

    /*!
    * Computes signed distance from point to plane.
    * This is the standard plane-equation: d=Ax*By*Cz+D.
    * The normal-vector is assumed to be normalized.
    *
    * Example:
    *  Vec3 v(1,2,3);
    *  Plane_tpl<F>  plane=CalculatePlane(v0,v1,v2);
    *  f32 distance = plane|v;
    */
    ILINE F operator | (const Vec3_tpl<F>& point) const { return (n | point) + d; }
    ILINE F DistFromPlane(const Vec3_tpl<F>& vPoint) const  {   return (n * vPoint + d); }

    ILINE Plane_tpl<F> operator - (const Plane_tpl<F>& p) const { return Plane_tpl<F>(n - p.n, d - p.d); }
    ILINE Plane_tpl<F> operator + (const Plane_tpl<F>& p) const { return Plane_tpl<F>(n + p.n, d + p.d); }
    ILINE void operator -= (const Plane_tpl<F>& p) { d -= p.d; n -= p.n; }
    ILINE Plane_tpl<F> operator * (F s) const {   return Plane_tpl<F>(n * s, d * s);   }
    ILINE Plane_tpl<F> operator / (F s) const {   return Plane_tpl<F>(n / s, d / s); }

    //! check for equality between two planes
    friend  bool operator ==(const Plane_tpl<F>& p1, const Plane_tpl<F>& p2)
    {
        if (fabsf(p1.n.x - p2.n.x) > 0.0001f)
        {
            return (false);
        }
        if (fabsf(p1.n.y - p2.n.y) > 0.0001f)
        {
            return (false);
        }
        if (fabsf(p1.n.z - p2.n.z) > 0.0001f)
        {
            return (false);
        }
        if (fabsf(p1.d - p2.d) < 0.01f)
        {
            return(true);
        }
        return (false);
    }

    Vec3_tpl<F> MirrorVector(const Vec3_tpl<F>& i)   {  return n * (2 * (n | i)) - i;  }
    Vec3_tpl<F> MirrorPosition(const Vec3_tpl<F>& i) {  return i - n * (2 * ((n | i) + d)); }

    ILINE bool IsValid() const { return !n.IsZeroFast(); } //A plane with a zero normal isn't valid.
};

typedef Plane_tpl<f32>  Plane; //always 32 bit


// declare common constants.  Must be done after the class for compiler conformance
// (msvc and clang handle instantiation differently)
const Vec3_tpl<float> Vec3_Zero(0, 0, 0);
const Vec3_tpl<float> Vec3_OneX(1, 0, 0);
const Vec3_tpl<float> Vec3_OneY(0, 1, 0);
const Vec3_tpl<float> Vec3_OneZ(0, 0, 1);
const Vec3_tpl<float> Vec3_One(1, 1, 1);

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(Vec3, "{DFA993FB-4E92-4A13-BDB3-4E9285A5346F}");
}
#endif // CRYINCLUDE_CRYCOMMON_CRY_VECTOR3_H
