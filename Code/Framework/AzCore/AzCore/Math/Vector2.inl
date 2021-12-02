/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace AZ
{
    AZ_MATH_INLINE Vector2::Vector2(const Vector2& v)
        : m_value(v.m_value)
    {
        ;
    }


    AZ_MATH_INLINE Vector2::Vector2(float x)
        : m_value(Simd::Vec2::Splat(x))
    {
        ;
    }


    AZ_MATH_INLINE Vector2::Vector2(float x, float y)
        : m_value(Simd::Vec2::LoadImmediate(x, y))
    {
        ;
    }


    AZ_MATH_INLINE Vector2::Vector2(Simd::Vec2::FloatArgType value)
        : m_value(value)
    {
        ;
    }

    AZ_MATH_INLINE Vector2 Vector2::CreateZero()
    {
        return Vector2(0.0f);
    }


    AZ_MATH_INLINE Vector2 Vector2::CreateOne()
    {
        return Vector2(1.0f);
    }


    AZ_MATH_INLINE Vector2 Vector2::CreateAxisX(float length)
    {
        return Vector2(length, 0.0f);
    }


    AZ_MATH_INLINE Vector2 Vector2::CreateAxisY(float length)
    {
        return Vector2(0.0f, length);
    }


    AZ_MATH_INLINE Vector2 Vector2::CreateFromFloat2(const float* values)
    {
        return Vector2(values[0], values[1]);
    }


    AZ_MATH_INLINE Vector2 Vector2::CreateFromAngle(float angle)
    {
        float sin, cos;
        SinCos(angle, sin, cos);
        return Vector2(sin, cos);
    }


    AZ_MATH_INLINE Vector2 Vector2::CreateSelectCmpEqual(const Vector2& cmp1, const Vector2& cmp2, const Vector2& vA, const Vector2& vB)
    {
#if AZ_TRAIT_USE_PLATFORM_SIMD_SCALAR
        return Vector2((cmp1.m_x == cmp2.m_x) ? vA.m_x : vB.m_x, (cmp1.m_y == cmp2.m_y) ? vA.m_y : vB.m_y);
#else
        Simd::Vec2::FloatType mask = Simd::Vec2::CmpEq(cmp1.m_value, cmp2.m_value);
        return Vector2(Simd::Vec2::Select(vA.m_value, vB.m_value, mask));
#endif
    }


    AZ_MATH_INLINE Vector2 Vector2::CreateSelectCmpGreaterEqual(const Vector2& cmp1, const Vector2& cmp2, const Vector2& vA, const Vector2& vB)
    {
#if AZ_TRAIT_USE_PLATFORM_SIMD_SCALAR
        return Vector2((cmp1.m_x >= cmp2.m_x) ? vA.m_x : vB.m_x, (cmp1.m_y >= cmp2.m_y) ? vA.m_y : vB.m_y);
#else
        Simd::Vec2::FloatType mask = Simd::Vec2::CmpGtEq(cmp1.m_value, cmp2.m_value);
        return Vector2(Simd::Vec2::Select(vA.m_value, vB.m_value, mask));
#endif
    }


    AZ_MATH_INLINE Vector2 Vector2::CreateSelectCmpGreater(const Vector2& cmp1, const Vector2& cmp2, const Vector2& vA, const Vector2& vB)
    {
#if AZ_TRAIT_USE_PLATFORM_SIMD_SCALAR
        return Vector2((cmp1.m_x > cmp2.m_x) ? vA.m_x : vB.m_x, (cmp1.m_y > cmp2.m_y) ? vA.m_y : vB.m_y);
#else
        Simd::Vec2::FloatType mask = Simd::Vec2::CmpGt(cmp1.m_value, cmp2.m_value);
        return Vector2(Simd::Vec2::Select(vA.m_value, vB.m_value, mask));
#endif
    }


    AZ_MATH_INLINE void Vector2::StoreToFloat2(float* values) const
    {
        values[0] = m_x;
        values[1] = m_y;
    }


    AZ_MATH_INLINE float Vector2::GetX() const
    {
        return m_x;
    }


    AZ_MATH_INLINE float Vector2::GetY() const
    {
        return m_y;
    }


    AZ_MATH_INLINE void Vector2::SetX(float x)
    {
        m_x = x;
    }


    AZ_MATH_INLINE void Vector2::SetY(float y)
    {
        m_y = y;
    }


    AZ_MATH_INLINE float Vector2::GetElement(int index) const
    {
        AZ_MATH_ASSERT((index >= 0) && (index < Simd::Vec2::ElementCount), "Invalid index for component access.\n");
        return m_values[index];
    }


    AZ_MATH_INLINE void Vector2::SetElement(int index, float value)
    {
        AZ_MATH_ASSERT((index >= 0) && (index < Simd::Vec2::ElementCount), "Invalid index for component access.\n");
        m_values[index] = value;
    }


    AZ_MATH_INLINE void Vector2::Set(float x)
    {
        m_value = Simd::Vec2::Splat(x);
    }


    AZ_MATH_INLINE void Vector2::Set(float x, float y)
    {
        m_value = Simd::Vec2::LoadImmediate(x, y);
    }


    AZ_MATH_INLINE float Vector2::operator()(int index) const
    {
        return GetElement(index);
    }


    AZ_MATH_INLINE float Vector2::GetLengthSq() const
    {
        return Dot(*this);
    }


    AZ_MATH_INLINE float Vector2::GetLength() const
    {
        return Simd::Vec1::SelectFirst(Simd::Vec1::Sqrt(Simd::Vec2::Dot(m_value, m_value)));
    }


    AZ_MATH_INLINE float Vector2::GetLengthEstimate() const
    {
        return Simd::Vec1::SelectFirst(Simd::Vec1::SqrtEstimate(Simd::Vec2::Dot(m_value, m_value)));
    }


    AZ_MATH_INLINE float Vector2::GetLengthReciprocal() const
    {
        return Simd::Vec1::SelectFirst(Simd::Vec1::SqrtInv(Simd::Vec2::Dot(m_value, m_value)));
    }


    AZ_MATH_INLINE float Vector2::GetLengthReciprocalEstimate() const
    {
        return Simd::Vec1::SelectFirst(Simd::Vec1::SqrtInvEstimate(Simd::Vec2::Dot(m_value, m_value)));
    }


    AZ_MATH_INLINE Vector2 Vector2::GetNormalized() const
    {
        return Vector2(Simd::Vec2::Normalize(m_value));
    }


    AZ_MATH_INLINE Vector2 Vector2::GetNormalizedEstimate() const
    {
        return Vector2(Simd::Vec2::NormalizeEstimate(m_value));
    }


    AZ_MATH_INLINE void Vector2::Normalize()
    {
        m_value = Simd::Vec2::Normalize(m_value);
    }


    AZ_MATH_INLINE void Vector2::NormalizeEstimate()
    {
        m_value = Simd::Vec2::NormalizeEstimate(m_value);
    }


    AZ_MATH_INLINE float Vector2::NormalizeWithLength()
    {
        float length = GetLength();
        (*this) *= (1.0f / length);
        return length;
    }


    AZ_MATH_INLINE float Vector2::NormalizeWithLengthEstimate()
    {
        float length = GetLengthEstimate();
        (*this) *= (1.0f / length);
        return length;
    }


    AZ_MATH_INLINE Vector2 Vector2::GetNormalizedSafe(float tolerance) const
    {
        return Vector2(Simd::Vec2::NormalizeSafe(m_value, tolerance));
    }


    AZ_MATH_INLINE Vector2 Vector2::GetNormalizedSafeEstimate(float tolerance) const
    {
        return Vector2(Simd::Vec2::NormalizeSafeEstimate(m_value, tolerance));
    }


    AZ_MATH_INLINE void Vector2::NormalizeSafe(float tolerance)
    {
        m_value = Simd::Vec2::NormalizeSafe(m_value, tolerance);
    }


    AZ_MATH_INLINE void Vector2::NormalizeSafeEstimate(float tolerance)
    {
        m_value = Simd::Vec2::NormalizeSafeEstimate(m_value, tolerance);
    }


    AZ_MATH_INLINE float Vector2::NormalizeSafeWithLength(float tolerance)
    {
        const Simd::Vec1::FloatType length = Simd::Vec1::Sqrt(Simd::Vec2::Dot(m_value, m_value));
        m_value = (Simd::Vec1::SelectFirst(length) < tolerance) ? Simd::Vec2::ZeroFloat() : Simd::Vec2::Div(m_value, Simd::Vec2::SplatFirst(Simd::Vec2::FromVec1(length)));
        return Simd::Vec1::SelectFirst(length);
    }


    AZ_MATH_INLINE float Vector2::NormalizeSafeWithLengthEstimate(float tolerance)
    {
        const Simd::Vec1::FloatType length = Simd::Vec1::SqrtEstimate(Simd::Vec2::Dot(m_value, m_value));
        m_value = (Simd::Vec1::SelectFirst(length) < tolerance) ? Simd::Vec2::ZeroFloat() : Simd::Vec2::Div(m_value, Simd::Vec2::SplatFirst(Simd::Vec2::FromVec1(length)));
        return Simd::Vec1::SelectFirst(length);
    }


    AZ_MATH_INLINE bool Vector2::IsNormalized(float tolerance) const
    {
        return (Abs(GetLengthSq() - 1.0f) <= tolerance);
    }


    AZ_MATH_INLINE void Vector2::SetLength(float length)
    {
        float scale = length / GetLength();
        m_value = Simd::Vec2::Mul(m_value, Simd::Vec2::Splat(scale));
    }


    AZ_MATH_INLINE void Vector2::SetLengthEstimate(float length)
    {
        float scale = length / GetLengthEstimate();
        m_value = Simd::Vec2::Mul(m_value, Simd::Vec2::Splat(scale));
    }


    AZ_MATH_INLINE float Vector2::GetDistanceSq(const Vector2& v) const
    {
        return ((*this) - v).GetLengthSq();
    }


    AZ_MATH_INLINE float Vector2::GetDistance(const Vector2& v) const
    {
        return ((*this) - v).GetLength();
    }


    AZ_MATH_INLINE float Vector2::GetDistanceEstimate(const Vector2& v) const
    {
        return ((*this) - v).GetLengthEstimate();
    }


    AZ_MATH_INLINE Vector2 Vector2::Lerp(const Vector2& dest, float t) const
    {
        return Vector2(Simd::Vec2::Madd(Simd::Vec2::Sub(dest.m_value, m_value), Simd::Vec2::Splat(t), m_value));
    }


    AZ_MATH_INLINE Vector2 Vector2::Slerp(const Vector2& dest, float t) const
    {
        // Dot product - the cosine of the angle between 2 vectors and clamp it to be in the range of Acos()
        const Simd::Vec1::FloatType dot = Simd::Vec1::Clamp(Simd::Vec2::Dot(m_value, dest.m_value), Simd::Vec1::Splat(-1.0f), Simd::Vec1::Splat(1.0f));
        // Acos(dot) returns the angle between start and end, and multiplying that by proportion returns the angle between start and the final result
        const Simd::Vec1::FloatType theta = Simd::Vec1::Mul(Simd::Vec1::Acos(dot), Simd::Vec1::Splat(t));
        const Simd::Vec2::FloatType relativeVec = Simd::Vec2::Sub(dest.GetSimdValue(), Simd::Vec2::Mul(GetSimdValue(), Simd::Vec2::FromVec1(dot)));
        const Simd::Vec2::FloatType relVecNorm = Simd::Vec2::NormalizeSafe(relativeVec, Constants::Tolerance);
        const Simd::Vec2::FloatType sinCos = Simd::Vec2::SinCos(theta);
        const Simd::Vec2::FloatType relVecSinTheta = Simd::Vec2::Mul(relVecNorm, Simd::Vec2::SplatFirst(sinCos));
        return Vector2(Simd::Vec2::Madd(GetSimdValue(), Simd::Vec2::SplatSecond(sinCos), relVecSinTheta));
    }


    AZ_MATH_INLINE Vector2 Vector2::Nlerp(const Vector2& dest, float t) const
    {
        return Lerp(dest, t).GetNormalizedSafe(Constants::Tolerance);
    }


    AZ_MATH_INLINE Vector2 Vector2::GetPerpendicular() const
    {
        return Vector2(-m_y, m_x);
    }


    AZ_MATH_INLINE bool Vector2::IsClose(const Vector2& v, float tolerance) const
    {
        Vector2 dist = (v - (*this)).GetAbs();
        return dist.IsLessEqualThan(Vector2(tolerance));
    }


    AZ_MATH_INLINE bool Vector2::IsZero(float tolerance) const
    {
        return IsClose(Vector2::CreateZero(), tolerance);
    }


    AZ_MATH_INLINE bool Vector2::operator==(const Vector2& rhs) const
    {
        return Simd::Vec2::CmpAllEq(m_value, rhs.m_value);
    }


    AZ_MATH_INLINE bool Vector2::operator!=(const Vector2& rhs) const
    {
        return !Simd::Vec2::CmpAllEq(m_value, rhs.m_value);
    }


    AZ_MATH_INLINE bool Vector2::IsLessThan(const Vector2& v) const
    {
        return Simd::Vec2::CmpAllLt(m_value, v.m_value);
    }


    AZ_MATH_INLINE bool Vector2::IsLessEqualThan(const Vector2& v) const
    {
        return Simd::Vec2::CmpAllLtEq(m_value, v.m_value);
    }


    AZ_MATH_INLINE bool Vector2::IsGreaterThan(const Vector2& v) const
    {
        return Simd::Vec2::CmpAllGt(m_value, v.m_value);
    }


    AZ_MATH_INLINE bool Vector2::IsGreaterEqualThan(const Vector2& v) const
    {
        return Simd::Vec2::CmpAllGtEq(m_value, v.m_value);
    }


    AZ_MATH_INLINE Vector2 Vector2::GetFloor() const
    {
        return Vector2(Simd::Vec2::Floor(m_value));
    }


    AZ_MATH_INLINE Vector2 Vector2::GetCeil() const
    {
        return Vector2(Simd::Vec2::Ceil(m_value));
    }


    AZ_MATH_INLINE Vector2 Vector2::GetRound() const
    {
        return Vector2(Simd::Vec2::Round(m_value));
    }


    AZ_MATH_INLINE Vector2 Vector2::GetMin(const Vector2& v) const
    {
#if AZ_TRAIT_USE_PLATFORM_SIMD_SCALAR
        return Vector2(AZ::GetMin(m_x, v.m_x), AZ::GetMin(m_y, v.m_y));
#else
        return Vector2(Simd::Vec2::Min(m_value, v.m_value));
#endif
    }


    AZ_MATH_INLINE Vector2 Vector2::GetMax(const Vector2& v) const
    {
#if AZ_TRAIT_USE_PLATFORM_SIMD_SCALAR
        return Vector2(AZ::GetMax(m_x, v.m_x), AZ::GetMax(m_y, v.m_y));
#else
        return Vector2(Simd::Vec2::Max(m_value, v.m_value));
#endif
    }


    AZ_MATH_INLINE Vector2 Vector2::GetClamp(const Vector2& min, const Vector2& max) const
    {
        return GetMin(max).GetMax(min);
    }


    AZ_MATH_INLINE Vector2 Vector2::GetSelect(const Vector2& vCmp, const Vector2& vB)
    {
#if AZ_TRAIT_USE_PLATFORM_SIMD_SCALAR
        return Vector2((vCmp.m_x == 0.0f) ? m_x : vB.m_x, (vCmp.m_y == 0.0f) ? m_y : vB.m_y);
#else
        Simd::Vec2::FloatType mask = Simd::Vec2::CmpEq(vCmp.m_value, Simd::Vec2::ZeroFloat());
        return Vector2(Simd::Vec2::Select(m_value, vB.m_value, mask));
#endif
    }


    AZ_MATH_INLINE void Vector2::Select(const Vector2& vCmp, const Vector2& vB)
    {
        *this = GetSelect(vCmp, vB);
    }


    AZ_MATH_INLINE Vector2 Vector2::GetAbs() const
    {
        return Vector2(Simd::Vec2::Abs(m_value));
    }


    AZ_MATH_INLINE Vector2& Vector2::operator+=(const Vector2& rhs)
    {
        m_value = Simd::Vec2::Add(m_value, rhs.m_value);
        return *this;
    }


    AZ_MATH_INLINE Vector2& Vector2::operator-=(const Vector2& rhs)
    {
        m_value = Simd::Vec2::Sub(m_value, rhs.m_value);
        return *this;
    }


    AZ_MATH_INLINE Vector2& Vector2::operator*=(const Vector2& rhs)
    {
        m_value = Simd::Vec2::Mul(m_value, rhs.m_value);
        return *this;
    }


    AZ_MATH_INLINE Vector2& Vector2::operator/=(const Vector2& rhs)
    {
        m_value = Simd::Vec2::Div(m_value, rhs.m_value);
        return *this;
    }


    AZ_MATH_INLINE Vector2& Vector2::operator*=(float multiplier)
    {
        m_value = Simd::Vec2::Mul(m_value, Simd::Vec2::Splat(multiplier));
        return *this;
    }


    AZ_MATH_INLINE Vector2& Vector2::operator/=(float divisor)
    {
        m_value = Simd::Vec2::Div(m_value, Simd::Vec2::Splat(divisor));
        return *this;
    }


    AZ_MATH_INLINE Vector2 Vector2::operator-() const
    {
        return Vector2(-m_x, -m_y);
    }


    AZ_MATH_INLINE Vector2 Vector2::operator+(const Vector2& rhs) const
    {
        return Vector2(Simd::Vec2::Add(m_value, rhs.m_value));
    }


    AZ_MATH_INLINE Vector2 Vector2::operator-(const Vector2& rhs) const
    {
        return Vector2(Simd::Vec2::Sub(m_value, rhs.m_value));
    }


    AZ_MATH_INLINE Vector2 Vector2::operator*(const Vector2& rhs) const
    {
        return Vector2(Simd::Vec2::Mul(m_value, rhs.m_value));
    }


    AZ_MATH_INLINE Vector2 Vector2::operator*(float multiplier) const
    {
        return Vector2(Simd::Vec2::Mul(m_value, Simd::Vec2::Splat(multiplier)));
    }


    AZ_MATH_INLINE Vector2 Vector2::operator/(float divisor) const
    {
        return Vector2(Simd::Vec2::Div(m_value, Simd::Vec2::Splat(divisor)));
    }


    AZ_MATH_INLINE Vector2 Vector2::operator/(const Vector2& rhs) const
    {
        return Vector2(Simd::Vec2::Div(m_value, rhs.m_value));
    }


    AZ_MATH_INLINE Vector2 Vector2::GetSin() const
    {
        return Vector2(Simd::Vec2::Sin(m_value));
    }


    AZ_MATH_INLINE Vector2 Vector2::GetCos() const
    {
        return Vector2(Simd::Vec2::Cos(m_value));
    }


    AZ_MATH_INLINE void Vector2::GetSinCos(Vector2& sin, Vector2& cos) const
    {
        Simd::Vec2::FloatType sinValues, cosValues;
        Simd::Vec2::SinCos(m_value, sinValues, cosValues);
        sin = Vector2(sinValues);
        cos = Vector2(cosValues);
    }


    AZ_MATH_INLINE Vector2 Vector2::GetAcos() const
    {
        return Vector2(Simd::Vec2::Acos(m_value));
    }


    AZ_MATH_INLINE Vector2 Vector2::GetAtan() const
    {
        return Vector2(Simd::Vec2::Atan(m_value));
    }


    AZ_MATH_INLINE float Vector2::GetAtan2() const
    {
        return Simd::Vec1::SelectFirst(Simd::Vec2::Atan2(m_value));
    }


    AZ_MATH_INLINE Vector2 Vector2::GetAngleMod() const
    {
        return Vector2(Simd::Vec2::AngleMod(m_value));
    }


    AZ_MATH_INLINE float Vector2::Angle(const Vector2& v) const
    {
        const float cos = Dot(v) * InvSqrt(GetLengthSq() * v.GetLengthSq());
        // secure against any float precision error, cosine must be between [-1, 1]
        const float res = Acos(AZ::GetClamp(cos, -1.0f, 1.0f));
        AZ_MATH_ASSERT(std::isfinite(res) && (res >= 0.0f) && (res <= Constants::Pi), "Calculated an invalid angle");
        return res;
    }


    AZ_MATH_INLINE float Vector2::AngleDeg(const Vector2& v) const
    {
        return RadToDeg(Angle(v));
    }


    AZ_MATH_INLINE float Vector2::AngleSafe(const Vector2& v) const
    {
        return (!IsZero() && !v.IsZero()) ? Angle(v) : 0.0f;
    }


    AZ_MATH_INLINE float Vector2::AngleSafeDeg(const Vector2& v) const
    {
        return (!IsZero() && !v.IsZero()) ? AngleDeg(v) : 0.0f;
    }


    AZ_MATH_INLINE Vector2 Vector2::GetReciprocal() const
    {
        return Vector2(Simd::Vec2::Reciprocal(m_value));
    }


    AZ_MATH_INLINE Vector2 Vector2::GetReciprocalEstimate() const
    {
        return Vector2(Simd::Vec2::ReciprocalEstimate(m_value));
    }


    AZ_MATH_INLINE float Vector2::Dot(const Vector2& rhs) const
    {
        return Simd::Vec1::SelectFirst(Simd::Vec2::Dot(m_value, rhs.m_value));
    }


    AZ_MATH_INLINE Vector2 Vector2::GetMadd(const Vector2& mul, const Vector2& add)
    {
        return Vector2(Simd::Vec2::Madd(m_value, mul.m_value, add.m_value));
    }


    AZ_MATH_INLINE void Vector2::Madd(const Vector2& mul, const Vector2& add)
    {
        *this = GetMadd(mul, add);
    }


    AZ_MATH_INLINE void Vector2::Project(const Vector2& rhs)
    {
        *this = rhs * (Dot(rhs) / rhs.Dot(rhs));
    }


    AZ_MATH_INLINE void Vector2::ProjectOnNormal(const Vector2& normal)
    {
        AZ_MATH_ASSERT(normal.IsNormalized(), "The provided input is not normalized.");
        *this = normal * Dot(normal);
    }


    AZ_MATH_INLINE Vector2 Vector2::GetProjected(const Vector2& rhs) const
    {
        return rhs * (Dot(rhs) / rhs.Dot(rhs));
    }


    AZ_MATH_INLINE Vector2 Vector2::GetProjectedOnNormal(const Vector2& normal)
    {
        AZ_MATH_ASSERT(normal.IsNormalized(), "The provided input is not normalized.");
        return normal * Dot(normal);
    }


    AZ_MATH_INLINE bool Vector2::IsFinite() const
    {
        return IsFiniteFloat(GetX()) && IsFiniteFloat(GetY());
    }


    AZ_MATH_INLINE Simd::Vec2::FloatType Vector2::GetSimdValue() const
    {
        return m_value;
    }


    AZ_MATH_INLINE Vector2 operator*(float multiplier, const Vector2& rhs)
    {
        return rhs * multiplier;
    }
}
