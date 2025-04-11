/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector2.h>

namespace AZ
{
    AZ_MATH_INLINE Vector4::Vector4(float x)
        : m_value(Simd::Vec4::Splat(x))
    {
        ;
    }

    AZ_MATH_INLINE Vector4::Vector4(float x, float y, float z, float w)
        : m_value(Simd::Vec4::LoadImmediate(x, y, z, w))
    {
        ;
    }

    AZ_MATH_INLINE Vector4::Vector4(Simd::Vec4::FloatArgType value)
        : m_value(value)
    {
    }

    AZ_MATH_INLINE Vector4::Vector4(const Vector2& source)
        : m_value(Simd::Vec4::FromVec2(source.GetSimdValue()))
    {
        m_z = 0.0f;
        m_w = 1.0f;
    }

    AZ_MATH_INLINE Vector4::Vector4(const Vector2& source, float z)
        : m_value(Simd::Vec4::FromVec2(source.GetSimdValue()))
    {
        m_z = z;
        m_w = 1.0f;
    }

    AZ_MATH_INLINE Vector4::Vector4(const Vector2& source, float z, float w)
        : m_value(Simd::Vec4::FromVec2(source.GetSimdValue()))
    {
        m_z = z;
        m_w = w;
    }

    AZ_MATH_INLINE Vector4::Vector4(const Vector3& source)
        : m_value(Simd::Vec4::FromVec3(source.GetSimdValue()))
    {
        m_w = 1.0f;
    }

    AZ_MATH_INLINE Vector4::Vector4(const Vector3& source, float w)
        : m_value(Simd::Vec4::FromVec3(source.GetSimdValue()))
    {
        m_w = w;
    }

    AZ_MATH_INLINE Vector4 Vector4::CreateZero()
    {
        return Vector4(Simd::Vec4::ZeroFloat());
    }

    AZ_MATH_INLINE Vector4 Vector4::CreateOne()
    {
        return Vector4(1.0f);
    }

    AZ_MATH_INLINE Vector4 Vector4::CreateAxisX(float length)
    {
        return Vector4(length, 0.0f, 0.0f, 0.0f);
    }

    AZ_MATH_INLINE Vector4 Vector4::CreateAxisY(float length)
    {
        return Vector4(0.0f, length, 0.0f, 0.0f);
    }

    AZ_MATH_INLINE Vector4 Vector4::CreateAxisZ(float length)
    {
        return Vector4(0.0f, 0.0f, length, 0.0f);
    }

    AZ_MATH_INLINE Vector4 Vector4::CreateAxisW(float length)
    {
        return Vector4(0.0f, 0.0f, 0.0f, length);
    }

    AZ_MATH_INLINE Vector4 Vector4::CreateFromFloat4(const float values[])
    {
        Vector4 result;
        result.Set(values);
        return result;
    }

    AZ_MATH_INLINE Vector4 Vector4::CreateFromVector3(const Vector3& v)
    {
        Vector4 result;
        result.Set(v);
        return result;
    }

    AZ_MATH_INLINE Vector4 Vector4::CreateFromVector3AndFloat(const Vector3& v, float w)
    {
        Vector4 result;
        result.Set(v, w);
        return result;
    }

    AZ_MATH_INLINE Vector4 Vector4::CreateSelectCmpEqual(const Vector4& cmp1, const Vector4& cmp2, const Vector4& vA, const Vector4& vB)
    {
#if AZ_TRAIT_USE_PLATFORM_SIMD_SCALAR
        return Vector4((cmp1.m_x == cmp2.m_x) ? vA.m_x : vB.m_x
                     , (cmp1.m_y == cmp2.m_y) ? vA.m_y : vB.m_y
                     , (cmp1.m_z == cmp2.m_z) ? vA.m_z : vB.m_z
                     , (cmp1.m_w == cmp2.m_w) ? vA.m_w : vB.m_w);
#else
        Simd::Vec4::FloatType mask = Simd::Vec4::CmpEq(cmp1.m_value, cmp2.m_value);
        return Vector4(Simd::Vec4::Select(vA.m_value, vB.m_value, mask));
#endif
    }

    AZ_MATH_INLINE Vector4 Vector4::CreateSelectCmpGreaterEqual(const Vector4& cmp1, const Vector4& cmp2, const Vector4& vA, const Vector4& vB)
    {
#if AZ_TRAIT_USE_PLATFORM_SIMD_SCALAR
        return Vector4((cmp1.m_x >= cmp2.m_x) ? vA.m_x : vB.m_x
                     , (cmp1.m_y >= cmp2.m_y) ? vA.m_y : vB.m_y
                     , (cmp1.m_z >= cmp2.m_z) ? vA.m_z : vB.m_z
                     , (cmp1.m_w >= cmp2.m_w) ? vA.m_w : vB.m_w);
#else
        Simd::Vec4::FloatType mask = Simd::Vec4::CmpGtEq(cmp1.m_value, cmp2.m_value);
        return Vector4(Simd::Vec4::Select(vA.m_value, vB.m_value, mask));
#endif
    }

    AZ_MATH_INLINE Vector4 Vector4::CreateSelectCmpGreater(const Vector4& cmp1, const Vector4& cmp2, const Vector4& vA, const Vector4& vB)
    {
#if AZ_TRAIT_USE_PLATFORM_SIMD_SCALAR
        return Vector4((cmp1.m_x > cmp2.m_x) ? vA.m_x : vB.m_x
                     , (cmp1.m_y > cmp2.m_y) ? vA.m_y : vB.m_y
                     , (cmp1.m_z > cmp2.m_z) ? vA.m_z : vB.m_z
                     , (cmp1.m_w > cmp2.m_w) ? vA.m_w : vB.m_w);
#else
        Simd::Vec4::FloatType mask = Simd::Vec4::CmpGt(cmp1.m_value, cmp2.m_value);
        return Vector4(Simd::Vec4::Select(vA.m_value, vB.m_value, mask));
#endif
    }

    AZ_MATH_INLINE void Vector4::StoreToFloat4(float* values) const
    {
        Simd::Vec4::StoreUnaligned(values, m_value);
    }

    AZ_MATH_INLINE float Vector4::GetX() const
    {
        return m_x;
    }

    AZ_MATH_INLINE float Vector4::GetY() const
    {
        return m_y;
    }

    AZ_MATH_INLINE float Vector4::GetZ() const
    {
        return m_z;
    }

    AZ_MATH_INLINE float Vector4::GetW() const
    {
        return m_w;
    }

    AZ_MATH_INLINE float Vector4::GetElement(int32_t index) const
    {
        AZ_MATH_ASSERT((index >= 0) && (index < Simd::Vec4::ElementCount), "Invalid index for component access.\n");
        return m_values[index];
    }

    AZ_MATH_INLINE void Vector4::SetX(float x)
    {
        m_x = x;
    }

    AZ_MATH_INLINE void Vector4::SetY(float y)
    {
        m_y = y;
    }

    AZ_MATH_INLINE void Vector4::SetZ(float z)
    {
        m_z = z;
    }

    AZ_MATH_INLINE void Vector4::SetW(float w)
    {
        m_w = w;
    }

    AZ_MATH_INLINE void Vector4::Set(float x)
    {
        m_value = Simd::Vec4::Splat(x);
    }

    AZ_MATH_INLINE void Vector4::Set(float x, float y, float z, float w)
    {
        m_value = Simd::Vec4::LoadImmediate(x, y, z, w);
    }

    AZ_MATH_INLINE void Vector4::Set(const float values[])
    {
        m_value = Simd::Vec4::LoadUnaligned(values);
    }

    AZ_MATH_INLINE void Vector4::Set(const Vector3& v)
    {
        m_value = Simd::Vec4::FromVec3(v.GetSimdValue());
        m_w = 1.0f;
    }

    AZ_MATH_INLINE void Vector4::Set(const Vector3& v, float w)
    {
        m_value = Simd::Vec4::FromVec3(v.GetSimdValue());
        m_w = w;
    }

    AZ_MATH_INLINE void Vector4::Set(Simd::Vec4::FloatArgType v)
    {
        m_value = v;
    }

    AZ_MATH_INLINE void Vector4::SetElement(int32_t index, float v)
    {
        AZ_MATH_ASSERT((index >= 0) && (index < Simd::Vec4::ElementCount), "Invalid index for component access.\n");
        m_values[index] = v;
    }

    AZ_MATH_INLINE Vector3 Vector4::GetAsVector3() const
    {
        return Vector3(Simd::Vec4::ToVec3(m_value));
    }

    AZ_MATH_INLINE float Vector4::operator()(int32_t index) const
    {
        return GetElement(index);
    }

    AZ_MATH_INLINE float Vector4::GetLengthSq() const
    {
        return Dot(*this);
    }

    AZ_MATH_INLINE float Vector4::GetLength() const
    {
        const Simd::Vec1::FloatType lengthSq = Simd::Vec4::Dot(m_value, m_value);
        return Simd::Vec1::SelectIndex0(Simd::Vec1::Sqrt(lengthSq));
    }

    AZ_MATH_INLINE float Vector4::GetLengthEstimate() const
    {
        const Simd::Vec1::FloatType lengthSq = Simd::Vec4::Dot(m_value, m_value);
        return Simd::Vec1::SelectIndex0(Simd::Vec1::SqrtEstimate(lengthSq));
    }

    AZ_MATH_INLINE float Vector4::GetLengthReciprocal() const
    {
        const Simd::Vec1::FloatType lengthSq = Simd::Vec4::Dot(m_value, m_value);
        return Simd::Vec1::SelectIndex0(Simd::Vec1::SqrtInv(lengthSq));
    }

    AZ_MATH_INLINE float Vector4::GetLengthReciprocalEstimate() const
    {
        const Simd::Vec1::FloatType lengthSq = Simd::Vec4::Dot(m_value, m_value);
        return Simd::Vec1::SelectIndex0(Simd::Vec1::SqrtInvEstimate(lengthSq));
    }

    AZ_MATH_INLINE Vector4 Vector4::GetNormalized() const
    {
        return Vector4(Simd::Vec4::Normalize(m_value));
    }

    AZ_MATH_INLINE Vector4 Vector4::GetNormalizedEstimate() const
    {
        return Vector4(Simd::Vec4::NormalizeEstimate(m_value));
    }

    AZ_MATH_INLINE void Vector4::Normalize()
    {
        *this = GetNormalized();
    }

    AZ_MATH_INLINE void Vector4::NormalizeEstimate()
    {
        *this = GetNormalizedEstimate();
    }

    AZ_MATH_INLINE Vector4 Vector4::GetNormalizedSafe(float tolerance) const
    {
        return Vector4(Simd::Vec4::NormalizeSafe(m_value, tolerance));
    }

    AZ_MATH_INLINE Vector4 Vector4::GetNormalizedSafeEstimate(float tolerance) const
    {
        return Vector4(Simd::Vec4::NormalizeSafeEstimate(m_value, tolerance));
    }

    AZ_MATH_INLINE void Vector4::NormalizeSafe(float tolerance)
    {
        *this = GetNormalizedSafe(tolerance);
    }

    AZ_MATH_INLINE void Vector4::NormalizeSafeEstimate(float tolerance)
    {
        *this = GetNormalizedSafeEstimate(tolerance);
    }

    AZ_MATH_INLINE float Vector4::NormalizeWithLength()
    {
        const float length = Simd::Vec1::SelectIndex0(
            Simd::Vec1::Sqrt(Simd::Vec4::Dot(m_value, m_value)));
        m_value = Simd::Vec4::Div(m_value, Simd::Vec4::Splat(length));
        return length;
    }

    AZ_MATH_INLINE float Vector4::NormalizeWithLengthEstimate()
    {
        const float length = Simd::Vec1::SelectIndex0(
            Simd::Vec1::SqrtEstimate(Simd::Vec4::Dot(m_value, m_value)));
        m_value = Simd::Vec4::Div(m_value, Simd::Vec4::Splat(length));
        return length;
    }

    AZ_MATH_INLINE float Vector4::NormalizeSafeWithLength(float tolerance)
    {
        const Simd::Vec1::FloatType length = Simd::Vec1::Sqrt(Simd::Vec4::Dot(m_value, m_value));
        m_value = (Simd::Vec1::SelectIndex0(length) < tolerance) ? Simd::Vec4::ZeroFloat() : Simd::Vec4::Div(m_value, Simd::Vec4::SplatIndex0(Simd::Vec4::FromVec1(length)));
        return Simd::Vec1::SelectIndex0(length);
    }

    AZ_MATH_INLINE float Vector4::NormalizeSafeWithLengthEstimate(float tolerance)
    {
        const Simd::Vec1::FloatType length = Simd::Vec1::SqrtEstimate(Simd::Vec4::Dot(m_value, m_value));
        m_value = (Simd::Vec1::SelectIndex0(length) < tolerance) ? Simd::Vec4::ZeroFloat() : Simd::Vec4::Div(m_value, Simd::Vec4::SplatIndex0(Simd::Vec4::FromVec1(length)));
        return Simd::Vec1::SelectIndex0(length);
    }

    AZ_MATH_INLINE bool Vector4::IsNormalized(float tolerance) const
    {
        return (Abs(GetLengthSq() - 1.0f) <= tolerance);
    }

    AZ_MATH_INLINE void Vector4::SetLength(float length)
    {
        float scale(length * GetLengthReciprocal());
        (*this) *= scale;
    }

    AZ_MATH_INLINE void Vector4::SetLengthEstimate(float length)
    {
        float scale(length * GetLengthReciprocalEstimate());
        (*this) *= scale;
    }

    AZ_MATH_INLINE float Vector4::GetDistanceSq(const Vector4& v) const
    {
        return ((*this) - v).GetLengthSq();
    }

    AZ_MATH_INLINE float Vector4::GetDistance(const Vector4& v) const
    {
        return ((*this) - v).GetLength();
    }

    AZ_MATH_INLINE float Vector4::GetDistanceEstimate(const Vector4& v) const
    {
        return ((*this) - v).GetLengthEstimate();
    }

    AZ_MATH_INLINE bool Vector4::IsClose(const Vector4& v, float tolerance) const
    {
        Vector4 dist = (v - (*this)).GetAbs();
        return dist.IsLessEqualThan(Vector4(tolerance));
    }

    AZ_MATH_INLINE bool Vector4::IsZero(float tolerance) const
    {
        Vector4 dist = GetAbs();
        return dist.IsLessEqualThan(Vector4(tolerance));
    }

    AZ_MATH_INLINE bool Vector4::operator==(const Vector4& rhs) const
    {
        return Simd::Vec4::CmpAllEq(m_value, rhs.m_value);
    }

    AZ_MATH_INLINE bool Vector4::operator!=(const Vector4& rhs) const
    {
        return !Simd::Vec4::CmpAllEq(m_value, rhs.m_value);
    }

    AZ_MATH_INLINE bool Vector4::IsLessThan(const Vector4& rhs) const
    {
        return Simd::Vec4::CmpAllLt(m_value, rhs.m_value);
    }

    AZ_MATH_INLINE bool Vector4::IsLessEqualThan(const Vector4& rhs) const
    {
        return Simd::Vec4::CmpAllLtEq(m_value, rhs.m_value);
    }

    AZ_MATH_INLINE bool Vector4::IsGreaterThan(const Vector4& rhs) const
    {
        return Simd::Vec4::CmpAllGt(m_value, rhs.m_value);
    }

    AZ_MATH_INLINE bool Vector4::IsGreaterEqualThan(const Vector4& rhs) const
    {
        return Simd::Vec4::CmpAllGtEq(m_value, rhs.m_value);
    }

    AZ_MATH_INLINE Vector4 Vector4::GetFloor() const
    {
        return Vector4(Simd::Vec4::Floor(m_value));
    }

    AZ_MATH_INLINE Vector4 Vector4::GetCeil() const
    {
        return Vector4(Simd::Vec4::Ceil(m_value));
    }

    AZ_MATH_INLINE Vector4 Vector4::GetRound() const
    {
        return Vector4(Simd::Vec4::Round(m_value));
    }

    AZ_MATH_INLINE Vector4 Vector4::GetMin(const Vector4& v) const
    {
#if AZ_TRAIT_USE_PLATFORM_SIMD_SCALAR
        return Vector4(AZ::GetMin(m_x, v.m_x), AZ::GetMin(m_y, v.m_y), AZ::GetMin(m_z, v.m_z), AZ::GetMin(m_w, v.m_w));
#else
        return Vector4(Simd::Vec4::Min(m_value, v.m_value));
#endif
    }

    AZ_MATH_INLINE Vector4 Vector4::GetMax(const Vector4& v) const
    {
#if AZ_TRAIT_USE_PLATFORM_SIMD_SCALAR
        return Vector4(AZ::GetMax(m_x, v.m_x), AZ::GetMax(m_y, v.m_y), AZ::GetMax(m_z, v.m_z), AZ::GetMax(m_w, v.m_w));
#else
        return Vector4(Simd::Vec4::Max(m_value, v.m_value));
#endif
    }

    AZ_MATH_INLINE Vector4 Vector4::GetClamp(const Vector4& min, const Vector4& max) const
    {
        return GetMin(max).GetMax(min);
    }

    AZ_MATH_INLINE Vector4 Vector4::Lerp(const Vector4& dest, float t) const
    {
        return Vector4(Simd::Vec4::Madd(Simd::Vec4::Sub(dest.m_value, m_value), Simd::Vec4::Splat(t), m_value));
    }

    AZ_MATH_INLINE Vector4 Vector4::Slerp(const Vector4& dest, float t) const
    {
        // Dot product - the cosine of the angle between 2 vectors and clamp it to be in the range of Acos()
        const Simd::Vec1::FloatType dot = Simd::Vec1::Clamp(Simd::Vec4::Dot(m_value, dest.m_value), Simd::Vec1::Splat(-1.0f), Simd::Vec1::Splat(1.0f));
        // Acos(dot) returns the angle between start and end, and multiplying that by proportion returns the angle between start and the final result
        const Simd::Vec1::FloatType theta = Simd::Vec1::Mul(Simd::Vec1::Acos(dot), Simd::Vec1::Splat(t));
        const Simd::Vec4::FloatType relativeVec = Simd::Vec4::Sub(dest.GetSimdValue(), Simd::Vec4::Mul(GetSimdValue(), Simd::Vec4::FromVec1(dot)));
        const Simd::Vec4::FloatType relVecNorm = Simd::Vec4::NormalizeSafe(relativeVec, Constants::Tolerance);
        const Simd::Vec4::FloatType sinCos = Simd::Vec4::FromVec2(Simd::Vec2::SinCos(theta));
        const Simd::Vec4::FloatType relVecSinTheta = Simd::Vec4::Mul(relVecNorm, Simd::Vec4::SplatIndex0(sinCos));
        return Vector4(Simd::Vec4::Madd(GetSimdValue(), Simd::Vec4::SplatIndex1(sinCos), relVecSinTheta));
    }

    AZ_MATH_INLINE Vector4 Vector4::Nlerp(const Vector4& dest, float t) const
    {
        return Lerp(dest, t).GetNormalizedSafe(Constants::Tolerance);
    }

    AZ_MATH_INLINE float Vector4::Dot(const Vector4& rhs) const
    {
#if AZ_TRAIT_USE_PLATFORM_SIMD_SCALAR
        return (m_x * rhs.m_x + m_y * rhs.m_y + m_z * rhs.m_z + m_w * rhs.m_w);
#else
        return Simd::Vec1::SelectIndex0(Simd::Vec4::Dot(m_value, rhs.m_value));
#endif
    }

    AZ_MATH_INLINE float Vector4::Dot3(const Vector3& rhs) const
    {
#if AZ_TRAIT_USE_PLATFORM_SIMD_SCALAR
        return (m_x * rhs.GetX() + m_y * rhs.GetY() + m_z * rhs.GetZ());
#else
        return Simd::Vec1::SelectIndex0(Simd::Vec3::Dot(Simd::Vec4::ToVec3(m_value), rhs.GetSimdValue()));
#endif
    }

    AZ_MATH_INLINE void Vector4::Homogenize()
    {
        const Simd::Vec4::FloatType divisor = Simd::Vec4::SplatIndex3(m_value);
        m_value = Simd::Vec4::Div(m_value, divisor);
    }

    AZ_MATH_INLINE Vector3 Vector4::GetHomogenized() const
    {
        const Simd::Vec3::FloatType divisor = Simd::Vec4::ToVec3(Simd::Vec4::SplatIndex3(m_value));
        return Vector3(Simd::Vec3::Div(Simd::Vec4::ToVec3(m_value), divisor));
    }

    AZ_MATH_INLINE Vector4 Vector4::operator-() const
    {
        return Vector4(Simd::Vec4::Sub(Simd::Vec4::ZeroFloat(), m_value));
    }

    AZ_MATH_INLINE Vector4 Vector4::operator+(const Vector4& rhs) const
    {
        return Vector4(Simd::Vec4::Add(m_value, rhs.m_value));
    }

    AZ_MATH_INLINE Vector4 Vector4::operator-(const Vector4& rhs) const
    {
        return Vector4(Simd::Vec4::Sub(m_value, rhs.m_value));
    }

    AZ_MATH_INLINE Vector4 Vector4::operator*(const Vector4& rhs) const
    {
        return Vector4(Simd::Vec4::Mul(m_value, rhs.m_value));
    }

    AZ_MATH_INLINE Vector4 Vector4::operator/(const Vector4& rhs) const
    {
        return Vector4(Simd::Vec4::Div(m_value, rhs.m_value));
    }

    AZ_MATH_INLINE Vector4 Vector4::operator*(float multiplier) const
    {
        return Vector4(Simd::Vec4::Mul(m_value, Simd::Vec4::Splat(multiplier)));
    }

    AZ_MATH_INLINE Vector4 Vector4::operator/(float divisor) const
    {
        return Vector4(Simd::Vec4::Div(m_value, Simd::Vec4::Splat(divisor)));
    }

    AZ_MATH_INLINE Vector4& Vector4::operator+=(const Vector4& rhs)
    {
        *this = (*this) + rhs;
        return *this;
    }

    AZ_MATH_INLINE Vector4& Vector4::operator-=(const Vector4& rhs)
    {
        *this = (*this) - rhs;
        return *this;
    }

    AZ_MATH_INLINE Vector4& Vector4::operator*=(const Vector4& rhs)
    {
        *this = (*this) * rhs;
        return *this;
    }

    AZ_MATH_INLINE Vector4& Vector4::operator/=(const Vector4& rhs)
    {
        *this = (*this) / rhs;
        return *this;
    }

    AZ_MATH_INLINE Vector4& Vector4::operator*=(float multiplier)
    {
        *this = (*this) * multiplier;
        return *this;
    }

    AZ_MATH_INLINE Vector4& Vector4::operator/=(float divisor)
    {
        *this = (*this) / divisor;
        return *this;
    }

    AZ_MATH_INLINE Vector4 Vector4::GetSin() const
    {
        return Vector4(Simd::Vec4::Sin(m_value));
    }

    AZ_MATH_INLINE Vector4 Vector4::GetCos() const
    {
        return Vector4(Simd::Vec4::Cos(m_value));
    }

    AZ_MATH_INLINE void Vector4::GetSinCos(Vector4& sin, Vector4& cos) const
    {
        Simd::Vec4::FloatType sinValues, cosValues;
        Simd::Vec4::SinCos(m_value, sinValues, cosValues);
        sin = Vector4(sinValues);
        cos = Vector4(cosValues);
    }

    AZ_MATH_INLINE Vector4 Vector4::GetAcos() const
    {
        return Vector4(Simd::Vec4::Acos(m_value));
    }

    AZ_MATH_INLINE Vector4 Vector4::GetAtan() const
    {
        return Vector4(Simd::Vec4::Atan(m_value));
    }

    AZ_MATH_INLINE Vector4 Vector4::GetExpEstimate() const
    {
        return Vector4(Simd::Vec4::ExpEstimate(m_value));
    }

    AZ_MATH_INLINE Vector4 Vector4::GetAngleMod() const
    {
        return Vector4(Simd::Vec4::AngleMod(m_value));
    }

    AZ_MATH_INLINE float Vector4::Angle(const Vector4& v) const
    {
        const float cos = Dot(v) * InvSqrt(GetLengthSq() * v.GetLengthSq());
        // secure against any float precision error, cosine must be between [-1, 1]
        const float res = Acos(AZ::GetClamp(cos, -1.0f, 1.0f));
        AZ_MATH_ASSERT(std::isfinite(res) && (res >= 0.0f) && (res <= Constants::Pi), "Calculated an invalid angle");
        return res;
    }

    AZ_MATH_INLINE float Vector4::AngleDeg(const Vector4& v) const
    {
        return RadToDeg(Angle(v));
    }

    AZ_MATH_INLINE float Vector4::AngleSafe(const Vector4& v) const
    {
        return (!IsZero() && !v.IsZero()) ? Angle(v) : 0.0f;
    }

    AZ_MATH_INLINE float Vector4::AngleSafeDeg(const Vector4& v) const
    {
        return (!IsZero() && !v.IsZero()) ? AngleDeg(v) : 0.0f;
    }

    AZ_MATH_INLINE Vector4 Vector4::GetAbs() const
    {
        return Vector4(Simd::Vec4::Abs(m_value));
    }

    AZ_MATH_INLINE Vector4 Vector4::GetReciprocal() const
    {
        return Vector4(Simd::Vec4::Reciprocal(m_value));
    }

    AZ_MATH_INLINE Vector4 Vector4::GetReciprocalEstimate() const
    {
        return Vector4(Simd::Vec4::ReciprocalEstimate(m_value));
    }

    AZ_MATH_INLINE bool Vector4::IsFinite() const
    {
        return IsFiniteFloat(GetX()) && IsFiniteFloat(GetY()) && IsFiniteFloat(GetZ()) && IsFiniteFloat(GetW());
    }

    AZ_MATH_INLINE Simd::Vec4::FloatType Vector4::GetSimdValue() const
    {
        return m_value;
    }

    AZ_MATH_INLINE void Vector4::SetSimdValue(Simd::Vec4::FloatArgType value)
    {
        m_value = value;
    }

    AZ_MATH_INLINE Vector4 operator*(float multiplier, const Vector4& rhs)
    {
        return rhs * multiplier;
    }
}
