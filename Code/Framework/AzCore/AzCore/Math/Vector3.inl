/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/MathUtils.h>

namespace AZ
{
    AZ_MATH_INLINE Vector3::Vector3(float x)
        : m_value(Simd::Vec3::Splat(x))
    {
        ;
    }


    AZ_MATH_INLINE Vector3::Vector3(float x, float y, float z)
        : m_value(Simd::Vec3::LoadImmediate(x, y, z))
    {
        ;
    }


    AZ_MATH_INLINE Vector3::Vector3(const Vector3& v)
        : m_value(v.m_value)
    {
        ;
    }


    AZ_MATH_INLINE Vector3::Vector3(Simd::Vec3::FloatArgType value)
        : m_value(value)
    {
        ;
    }


    AZ_MATH_INLINE Vector3 Vector3::CreateZero()
    {
        return Vector3(Simd::Vec3::ZeroFloat());
    }


    AZ_MATH_INLINE Vector3 Vector3::CreateOne()
    {
        return Vector3(1.0f);
    }


    AZ_MATH_INLINE Vector3 Vector3::CreateAxisX(float length)
    {
        return Vector3(length, 0.0f, 0.0f);
    }


    AZ_MATH_INLINE Vector3 Vector3::CreateAxisY(float length)
    {
        return Vector3(0.0f, length, 0.0f);
    }


    AZ_MATH_INLINE Vector3 Vector3::CreateAxisZ(float length)
    {
        return Vector3(0.0f, 0.0f, length);
    }


    AZ_MATH_INLINE Vector3 Vector3::CreateFromFloat3(const float* values)
    {
        return Vector3(values[0], values[1], values[2]);
    }


    // operation ( r.x = (cmp1.x == cmp2.x) ? vA.x : vB.x ) per component
    AZ_MATH_INLINE Vector3 Vector3::CreateSelectCmpEqual(const Vector3& cmp1, const Vector3& cmp2, const Vector3& vA, const Vector3& vB)
    {
#if AZ_TRAIT_USE_PLATFORM_SIMD_SCALAR
        return Vector3((cmp1.m_x == cmp2.m_x) ? vA.m_x : vB.m_x, (cmp1.m_y == cmp2.m_y) ? vA.m_y : vB.m_y, (cmp1.m_z == cmp2.m_z) ? vA.m_z : vB.m_z);
#else
        Simd::Vec3::FloatType mask = Simd::Vec3::CmpEq(cmp1.m_value, cmp2.m_value);
        return Vector3(Simd::Vec3::Select(vA.m_value, vB.m_value, mask));
#endif
    }


    // operation ( r.x = (cmp1.x >= cmp2.x) ? vA.x : vB.x ) per component
    AZ_MATH_INLINE Vector3 Vector3::CreateSelectCmpGreaterEqual(const Vector3& cmp1, const Vector3& cmp2, const Vector3& vA, const Vector3& vB)
    {
#if AZ_TRAIT_USE_PLATFORM_SIMD_SCALAR
        return Vector3((cmp1.m_x >= cmp2.m_x) ? vA.m_x : vB.m_x, (cmp1.m_y >= cmp2.m_y) ? vA.m_y : vB.m_y, (cmp1.m_z >= cmp2.m_z) ? vA.m_z : vB.m_z);
#else
        Simd::Vec3::FloatType mask = Simd::Vec3::CmpGtEq(cmp1.m_value, cmp2.m_value);
        return Vector3(Simd::Vec3::Select(vA.m_value, vB.m_value, mask));
#endif
    }


    // operation ( r.x = (cmp1.x > cmp2.x) ? vA.x : vB.x ) per component
    AZ_MATH_INLINE Vector3 Vector3::CreateSelectCmpGreater(const Vector3& cmp1, const Vector3& cmp2, const Vector3& vA, const Vector3& vB)
    {
#if AZ_TRAIT_USE_PLATFORM_SIMD_SCALAR
        return Vector3((cmp1.m_x > cmp2.m_x) ? vA.m_x : vB.m_x, (cmp1.m_y > cmp2.m_y) ? vA.m_y : vB.m_y, (cmp1.m_z > cmp2.m_z) ? vA.m_z : vB.m_z);
#else
        Simd::Vec3::FloatType mask = Simd::Vec3::CmpGt(cmp1.m_value, cmp2.m_value);
        return Vector3(Simd::Vec3::Select(vA.m_value, vB.m_value, mask));
#endif
    }


    AZ_MATH_INLINE void Vector3::StoreToFloat3(float* values) const
    {
        values[0] = m_values[0];
        values[1] = m_values[1];
        values[2] = m_values[2];
    }


    AZ_MATH_INLINE void Vector3::StoreToFloat4(float* values) const
    {
        Simd::Vec3::StoreUnaligned(values, m_value);
    }


    AZ_MATH_INLINE float Vector3::GetX() const
    {
        return m_x;
    }


    AZ_MATH_INLINE float Vector3::GetY() const
    {
        return m_y;
    }


    AZ_MATH_INLINE float Vector3::GetZ() const
    {
        return m_z;
    }


    AZ_MATH_INLINE float Vector3::GetElement(int32_t index) const
    {
        AZ_MATH_ASSERT((index >= 0) && (index < Simd::Vec3::ElementCount), "Invalid index for component access.\n");
        return m_values[index];
    }


    AZ_MATH_INLINE void Vector3::SetX(float x)
    {
        m_x = x;
    }


    AZ_MATH_INLINE void Vector3::SetY(float y)
    {
        m_y = y;
    }


    AZ_MATH_INLINE void Vector3::SetZ(float z)
    {
        m_z = z;
    }


    AZ_MATH_INLINE void Vector3::Set(float x)
    {
        m_value = Simd::Vec3::Splat(x);
    }


    AZ_MATH_INLINE void Vector3::SetElement(int32_t index, float v)
    {
        AZ_MATH_ASSERT((index >= 0) && (index < Simd::Vec3::ElementCount), "Invalid index for component access.\n");
        m_values[index] = v;
    }


    AZ_MATH_INLINE void Vector3::Set(float x, float y, float z)
    {
        m_value = Simd::Vec3::LoadImmediate(x, y, z);
    }


    AZ_MATH_INLINE void Vector3::Set(float values[])
    {
        m_value = Simd::Vec3::LoadImmediate(values[0], values[1], values[2]);
    }


    AZ_MATH_INLINE float Vector3::operator()(int32_t index) const
    {
        return GetElement(index);
    }


    AZ_MATH_INLINE float Vector3::GetLengthSq() const
    {
        return Dot(*this);
    }


    AZ_MATH_INLINE float Vector3::GetLength() const
    {
        const Simd::Vec1::FloatType lengthSq = Simd::Vec3::Dot(m_value, m_value);
        return Simd::Vec1::SelectFirst(Simd::Vec1::Sqrt(lengthSq));
    }


    AZ_MATH_INLINE float Vector3::GetLengthEstimate() const
    {
        const Simd::Vec1::FloatType lengthSq = Simd::Vec3::Dot(m_value, m_value);
        return Simd::Vec1::SelectFirst(Simd::Vec1::SqrtEstimate(lengthSq));
    }


    AZ_MATH_INLINE float Vector3::GetLengthReciprocal() const
    {
        const Simd::Vec1::FloatType lengthSq = Simd::Vec3::Dot(m_value, m_value);
        return Simd::Vec1::SelectFirst(Simd::Vec1::SqrtInv(lengthSq));
    }


    AZ_MATH_INLINE float Vector3::GetLengthReciprocalEstimate() const
    {
        const Simd::Vec1::FloatType lengthSq = Simd::Vec3::Dot(m_value, m_value);
        return Simd::Vec1::SelectFirst(Simd::Vec1::SqrtInvEstimate(lengthSq));
    }


    AZ_MATH_INLINE Vector3 Vector3::GetNormalized() const
    {
        return Vector3(Simd::Vec3::Normalize(m_value));
    }


    AZ_MATH_INLINE Vector3 Vector3::GetNormalizedEstimate() const
    {
        return Vector3(Simd::Vec3::NormalizeEstimate(m_value));
    }


    AZ_MATH_INLINE void Vector3::Normalize()
    {
        *this = GetNormalized();
    }


    AZ_MATH_INLINE void Vector3::NormalizeEstimate()
    {
        *this = GetNormalizedEstimate();
    }


    AZ_MATH_INLINE float Vector3::NormalizeWithLength()
    {
        const Simd::Vec1::FloatType length = Simd::Vec1::Sqrt(Simd::Vec3::Dot(m_value, m_value));
        (*this) /= Vector3(Simd::Vec3::FromVec1(length));
        return Simd::Vec1::SelectFirst(length);
    }


    AZ_MATH_INLINE float Vector3::NormalizeWithLengthEstimate()
    {
        const Simd::Vec1::FloatType length = Simd::Vec1::SqrtEstimate(Simd::Vec3::Dot(m_value, m_value));
        (*this) /= Vector3(Simd::Vec3::FromVec1(length));
        return Simd::Vec1::SelectFirst(length);
    }


    AZ_MATH_INLINE Vector3 Vector3::GetNormalizedSafe(float tolerance) const
    {
        return Vector3(Simd::Vec3::NormalizeSafe(m_value, tolerance));
    }


    AZ_MATH_INLINE Vector3 Vector3::GetNormalizedSafeEstimate(float tolerance) const
    {
        return Vector3(Simd::Vec3::NormalizeSafeEstimate(m_value, tolerance));
    }


    AZ_MATH_INLINE void Vector3::NormalizeSafe(float tolerance)
    {
        m_value = Simd::Vec3::NormalizeSafe(m_value, tolerance);
    }


    AZ_MATH_INLINE void Vector3::NormalizeSafeEstimate(float tolerance)
    {
        m_value = Simd::Vec3::NormalizeSafeEstimate(m_value, tolerance);
    }


    AZ_MATH_INLINE float Vector3::NormalizeSafeWithLength(float tolerance)
    {
        const Simd::Vec1::FloatType length = Simd::Vec1::Sqrt(Simd::Vec3::Dot(m_value, m_value));
        m_value = (Simd::Vec1::SelectFirst(length) < tolerance) ? Simd::Vec3::ZeroFloat() : Simd::Vec3::Div(m_value, Simd::Vec3::SplatFirst(Simd::Vec3::FromVec1(length)));
        return Simd::Vec1::SelectFirst(length);
    }


    AZ_MATH_INLINE float Vector3::NormalizeSafeWithLengthEstimate(float tolerance)
    {
        const Simd::Vec1::FloatType length = Simd::Vec1::SqrtEstimate(Simd::Vec3::Dot(m_value, m_value));
        m_value = (Simd::Vec1::SelectFirst(length) < tolerance) ? Simd::Vec3::ZeroFloat() : Simd::Vec3::Div(m_value, Simd::Vec3::SplatFirst(Simd::Vec3::FromVec1(length)));
        return Simd::Vec1::SelectFirst(length);
    }


    AZ_MATH_INLINE bool Vector3::IsNormalized(float tolerance) const
    {
        return (Abs(GetLengthSq() - 1.0f) <= tolerance);
    }


    AZ_MATH_INLINE void Vector3::SetLength(float length)
    {
        float scale(length * GetLengthReciprocal());
        (*this) *= scale;
    }


    AZ_MATH_INLINE void Vector3::SetLengthEstimate(float length)
    {
        float scale(length * GetLengthReciprocalEstimate());
        (*this) *= scale;
    }


    AZ_MATH_INLINE float Vector3::GetDistanceSq(const Vector3& v) const
    {
        return ((*this) - v).GetLengthSq();
    }


    AZ_MATH_INLINE float Vector3::GetDistance(const Vector3& v) const
    {
        return ((*this) - v).GetLength();
    }


    AZ_MATH_INLINE float Vector3::GetDistanceEstimate(const Vector3& v) const
    {
        return ((*this) - v).GetLengthEstimate();
    }


    AZ_MATH_INLINE Vector3 Vector3::Lerp(const Vector3& dest, float t) const
    {
        return Vector3(Simd::Vec3::Madd(Simd::Vec3::Sub(dest.m_value, m_value), Simd::Vec3::Splat(t), m_value));
    }


    AZ_MATH_INLINE Vector3 Vector3::Slerp(const Vector3& dest, float t) const
    {
        // Dot product - the cosine of the angle between 2 vectors and clamp it to be in the range of Acos()
        const Simd::Vec1::FloatType dot = Simd::Vec1::Clamp(Simd::Vec3::Dot(m_value, dest.m_value), Simd::Vec1::Splat(-1.0f), Simd::Vec1::Splat(1.0f));
        // Acos(dot) returns the angle between start and end, and multiplying that by proportion returns the angle between start and the final result
        const Simd::Vec1::FloatType theta = Simd::Vec1::Mul(Simd::Vec1::Acos(dot), Simd::Vec1::Splat(t));
        const Simd::Vec3::FloatType relativeVec = Simd::Vec3::Sub(dest.GetSimdValue(), Simd::Vec3::Mul(GetSimdValue(), Simd::Vec3::FromVec1(dot)));
        const Simd::Vec3::FloatType relVecNorm = Simd::Vec3::NormalizeSafe(relativeVec, Constants::Tolerance);
        const Simd::Vec3::FloatType sinCos = Simd::Vec3::FromVec2(Simd::Vec2::SinCos(theta));
        const Simd::Vec3::FloatType relVecSinTheta = Simd::Vec3::Mul(relVecNorm, Simd::Vec3::SplatFirst(sinCos));
        return Vector3(Simd::Vec3::Madd(GetSimdValue(), Simd::Vec3::SplatSecond(sinCos), relVecSinTheta));
    }


    AZ_MATH_INLINE Vector3 Vector3::Nlerp(const Vector3& dest, float t) const
    {
        return Lerp(dest, t).GetNormalizedSafe(Constants::Tolerance);
    }


    AZ_MATH_INLINE float Vector3::Dot(const Vector3& rhs) const
    {
#if AZ_TRAIT_USE_PLATFORM_SIMD_SCALAR
        return (m_x * rhs.m_x + m_y * rhs.m_y + m_z * rhs.m_z);
#else
        return Simd::Vec1::SelectFirst(Simd::Vec3::Dot(GetSimdValue(), rhs.GetSimdValue()));
#endif
    }


    AZ_MATH_INLINE Vector3 Vector3::Cross(const Vector3& rhs) const
    {
#if AZ_TRAIT_USE_PLATFORM_SIMD_SCALAR
        return Vector3(m_y * rhs.m_z - m_z * rhs.m_y, m_z * rhs.m_x - m_x * rhs.m_z, m_x * rhs.m_y - m_y * rhs.m_x);
#else
        return Vector3(Simd::Vec3::Cross(GetSimdValue(), rhs.GetSimdValue()));
#endif
    }


    AZ_MATH_INLINE Vector3 Vector3::CrossXAxis() const
    {
        return Vector3(0.0f, m_z, -m_y);
    }


    AZ_MATH_INLINE Vector3 Vector3::CrossYAxis() const
    {
        return Vector3(-m_z, 0.0f, m_x);
    }


    AZ_MATH_INLINE Vector3 Vector3::CrossZAxis() const
    {
        return Vector3(m_y, -m_x, 0.0f);
    }


    AZ_MATH_INLINE Vector3 Vector3::XAxisCross() const
    {
        return Vector3(0.0f, -m_z, m_y);
    }


    AZ_MATH_INLINE Vector3 Vector3::YAxisCross() const
    {
        return Vector3(m_z, 0.0f, -m_x);
    }


    AZ_MATH_INLINE Vector3 Vector3::ZAxisCross() const
    {
        return Vector3(-m_y, m_x, 0.0f);
    }


    AZ_MATH_INLINE bool Vector3::IsClose(const Vector3& v, float tolerance) const
    {
        Vector3 dist = (v - (*this)).GetAbs();
        return dist.IsLessEqualThan(Vector3(tolerance));
    }


    AZ_MATH_INLINE bool Vector3::IsZero(float tolerance) const
    {
        return IsClose(CreateZero(), tolerance);
    }


    AZ_MATH_INLINE bool Vector3::operator==(const Vector3& rhs) const
    {
        return Simd::Vec3::CmpAllEq(m_value, rhs.m_value);
    }


    AZ_MATH_INLINE bool Vector3::operator!=(const Vector3& rhs) const
    {
        return !Simd::Vec3::CmpAllEq(m_value, rhs.m_value);
    }


    AZ_MATH_INLINE bool Vector3::IsLessThan(const Vector3& rhs) const
    {
        return Simd::Vec3::CmpAllLt(m_value, rhs.m_value);
    }


    AZ_MATH_INLINE bool Vector3::IsLessEqualThan(const Vector3& rhs) const
    {
        return Simd::Vec3::CmpAllLtEq(m_value, rhs.m_value);
    }


    AZ_MATH_INLINE bool Vector3::IsGreaterThan(const Vector3& rhs) const
    {
        return Simd::Vec3::CmpAllGt(m_value, rhs.m_value);
    }


    AZ_MATH_INLINE bool Vector3::IsGreaterEqualThan(const Vector3& rhs) const
    {
        return Simd::Vec3::CmpAllGtEq(m_value, rhs.m_value);
    }


    AZ_MATH_INLINE Vector3 Vector3::GetFloor() const
    {
        return Vector3(Simd::Vec3::Floor(m_value));
    }


    AZ_MATH_INLINE Vector3 Vector3::GetCeil() const
    {
        return Vector3(Simd::Vec3::Ceil(m_value));
    }


    AZ_MATH_INLINE Vector3 Vector3::GetRound() const
    {
        return Vector3(Simd::Vec3::Round(m_value));
    }


    AZ_MATH_INLINE Vector3 Vector3::GetMin(const Vector3& v) const
    {
#if AZ_TRAIT_USE_PLATFORM_SIMD_SCALAR
        return Vector3(AZ::GetMin(m_x, v.m_x), AZ::GetMin(m_y, v.m_y), AZ::GetMin(m_z, v.m_z));
#else
        return Vector3(Simd::Vec3::Min(GetSimdValue(), v.GetSimdValue()));
#endif
    }


    AZ_MATH_INLINE Vector3 Vector3::GetMax(const Vector3& v) const
    {
#if AZ_TRAIT_USE_PLATFORM_SIMD_SCALAR
        return Vector3(AZ::GetMax(m_x, v.m_x), AZ::GetMax(m_y, v.m_y), AZ::GetMax(m_z, v.m_z));
#else
        return Vector3(Simd::Vec3::Max(GetSimdValue(), v.GetSimdValue()));
#endif
    }


    AZ_MATH_INLINE Vector3 Vector3::GetClamp(const Vector3& min, const Vector3& max) const
    {
        return GetMin(max).GetMax(min);
    }


    AZ_MATH_INLINE float Vector3::GetMaxElement() const
    {
        return AZStd::max<float>(m_x, AZStd::max<float>(m_y, m_z));
    }


    AZ_MATH_INLINE float Vector3::GetMinElement() const
    {
        return AZStd::min<float>(m_x, AZStd::min<float>(m_y, m_z));
    }


    AZ_MATH_INLINE Vector3& Vector3::operator=(const Vector3& rhs)
    {
        m_value = rhs.m_value;
        return *this;
    }


    AZ_MATH_INLINE Vector3 Vector3::operator-() const
    {
        return Vector3(Simd::Vec3::Sub(Simd::Vec3::ZeroFloat(), m_value));
    }


    AZ_MATH_INLINE Vector3 Vector3::operator+(const Vector3& rhs) const
    {
        return Vector3(Simd::Vec3::Add(m_value, rhs.m_value));
    }


    AZ_MATH_INLINE Vector3 Vector3::operator-(const Vector3& rhs) const
    {
        return Vector3(Simd::Vec3::Sub(m_value, rhs.m_value));
    }


    AZ_MATH_INLINE Vector3 Vector3::operator*(const Vector3& rhs) const
    {
        return Vector3(Simd::Vec3::Mul(m_value, rhs.m_value));
    }


    AZ_MATH_INLINE Vector3 Vector3::operator/(const Vector3& rhs) const
    {
        return Vector3(Simd::Vec3::Div(m_value, rhs.m_value));
    }


    AZ_MATH_INLINE Vector3 Vector3::operator*(float multiplier) const
    {
        return Vector3(Simd::Vec3::Mul(m_value, Simd::Vec3::Splat(multiplier)));
    }


    AZ_MATH_INLINE Vector3 Vector3::operator/(float divisor) const
    {
        return Vector3(Simd::Vec3::Div(m_value, Simd::Vec3::Splat(divisor)));
    }


    AZ_MATH_INLINE Vector3& Vector3::operator+=(const Vector3& rhs)
    {
        *this = (*this) + rhs;
        return *this;
    }


    AZ_MATH_INLINE Vector3& Vector3::operator-=(const Vector3& rhs)
    {
        *this = (*this) - rhs;
        return *this;
    }


    AZ_MATH_INLINE Vector3& Vector3::operator*=(const Vector3& rhs)
    {
        *this = (*this) * rhs;
        return *this;
    }


    AZ_MATH_INLINE Vector3& Vector3::operator/=(const Vector3& rhs)
    {
        *this = (*this) / rhs;
        return *this;
    }


    AZ_MATH_INLINE Vector3& Vector3::operator*=(float multiplier)
    {
        *this = (*this) * multiplier;
        return *this;
    }


    AZ_MATH_INLINE Vector3& Vector3::operator/=(float divisor)
    {
        *this = (*this) / divisor;
        return *this;
    }


    AZ_MATH_INLINE Vector3 Vector3::GetSin() const
    {
        return Vector3(Simd::Vec3::Sin(m_value));
    }


    AZ_MATH_INLINE Vector3 Vector3::GetCos() const
    {
        return Vector3(Simd::Vec3::Cos(m_value));
    }


    AZ_MATH_INLINE void Vector3::GetSinCos(Vector3& sin, Vector3& cos) const
    {
        Simd::Vec3::SinCos(m_value, sin.m_value, cos.m_value);
    }


    AZ_MATH_INLINE Vector3 Vector3::GetAcos() const
    {
        return Vector3(Simd::Vec3::Acos(m_value));
    }


    AZ_MATH_INLINE Vector3 Vector3::GetAtan() const
    {
        return Vector3(Simd::Vec3::Atan(m_value));
    }


    AZ_MATH_INLINE Vector3 Vector3::GetAngleMod() const
    {
        return Vector3(Simd::Vec3::AngleMod(m_value));
    }


    AZ_MATH_INLINE float Vector3::Angle(const Vector3& v) const
    {
        const float cos = Dot(v) * InvSqrt(GetLengthSq() * v.GetLengthSq());
        // secure against any float precision error, cosine must be between [-1, 1]
        const float res = Acos(AZ::GetClamp(cos, -1.0f, 1.0f));
        AZ_MATH_ASSERT(std::isfinite(res) && (res >= 0.0f) && (res <= Constants::Pi), "Calculated an invalid angle");
        return res;
    }


    AZ_MATH_INLINE float Vector3::AngleDeg(const Vector3& v) const
    {
        return RadToDeg(Angle(v));
    }


    AZ_MATH_INLINE float Vector3::AngleSafe(const Vector3& v) const
    {
        return (!IsZero() && !v.IsZero()) ? Angle(v) : 0.0f;
    }


    AZ_MATH_INLINE float Vector3::AngleSafeDeg(const Vector3& v) const
    {
        return (!IsZero() && !v.IsZero()) ? AngleDeg(v) : 0.0f;
    }


    AZ_MATH_INLINE Vector3 Vector3::GetAbs() const
    {
        return Vector3(Simd::Vec3::Abs(m_value));
    }


    AZ_MATH_INLINE Vector3 Vector3::GetReciprocal() const
    {
        return Vector3(Simd::Vec3::Reciprocal(m_value));
    }


    AZ_MATH_INLINE Vector3 Vector3::GetReciprocalEstimate() const
    {
        return Vector3(Simd::Vec3::ReciprocalEstimate(m_value));
    }


    AZ_MATH_INLINE Vector3 Vector3::GetMadd(const Vector3& mul, const Vector3& add)
    {
        return Vector3(Simd::Vec3::Madd(GetSimdValue(), mul.GetSimdValue(), add.GetSimdValue()));
    }


    AZ_MATH_INLINE void Vector3::Madd(const Vector3& mul, const Vector3& add)
    {
        *this = GetMadd(mul, add);
    }


    AZ_MATH_INLINE bool Vector3::IsPerpendicular(const Vector3& v, float tolerance) const
    {
        Simd::Vec1::FloatType absLengthSq = Simd::Vec1::Abs(Simd::Vec3::Dot(m_value, v.m_value));
        return Simd::Vec1::SelectFirst(absLengthSq) < tolerance;
    }


    AZ_MATH_INLINE Vector3 Vector3::GetOrthogonalVector() const
    {
        // for stability choose an axis which has a small component in this vector
        const Vector3 axis = (GetX() * GetX() < 0.5f * GetLengthSq()) ? Vector3::CreateAxisX() : Vector3::CreateAxisY();
        return Cross(axis);
    }


    AZ_MATH_INLINE void Vector3::Project(const Vector3& rhs)
    {
        *this = rhs * (Dot(rhs) / rhs.Dot(rhs));
    }


    AZ_MATH_INLINE void Vector3::ProjectOnNormal(const Vector3& normal)
    {
        AZ_MATH_ASSERT(normal.IsNormalized(), "This normal is not a normalized");
        *this = normal * Dot(normal);
    }


    AZ_MATH_INLINE Vector3 Vector3::GetProjected(const Vector3& rhs) const
    {
        return rhs * (Dot(rhs) / rhs.Dot(rhs));
    }


    AZ_MATH_INLINE Vector3 Vector3::GetProjectedOnNormal(const Vector3& normal)
    {
        AZ_MATH_ASSERT(normal.IsNormalized(), "This provided normal is not normalized");
        return normal * Dot(normal);
    }


    AZ_MATH_INLINE bool Vector3::IsFinite() const
    {
        return IsFiniteFloat(m_x) && IsFiniteFloat(m_y) && IsFiniteFloat(m_z);
    }


    AZ_MATH_INLINE Simd::Vec3::FloatType Vector3::GetSimdValue() const
    {
        return m_value;
    }


    AZ_MATH_INLINE Vector3 operator*(float multiplier, const Vector3& rhs)
    {
        return rhs * multiplier;
    }


    AZ_MATH_INLINE Vector3 Vector3RadToDeg(const Vector3& radians)
    {
        return Vector3(Simd::Vec3::Mul(radians.GetSimdValue(), Simd::Vec3::Splat(180.0f / Constants::Pi)));
    }


    AZ_MATH_INLINE Vector3 Vector3DegToRad(const Vector3& degrees)
    {
        return Vector3(Simd::Vec3::Mul(degrees.GetSimdValue(), Simd::Vec3::Splat(Constants::Pi / 180.0f)));
    }
}
