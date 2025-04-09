/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace AZ
{
    AZ_MATH_INLINE Quaternion::Quaternion(const Quaternion& q)
        : m_value(q.m_value)
    {
        ;
    }


    AZ_MATH_INLINE Quaternion::Quaternion(float x)
        : m_value(Simd::Vec4::Splat(x))
    {
        ;
    }


    AZ_MATH_INLINE Quaternion::Quaternion(float x, float y, float z, float w)
        : m_value(Simd::Vec4::LoadImmediate(x, y, z, w))
    {
        ;
    }


    AZ_MATH_INLINE Quaternion::Quaternion(const Vector3& v, float w)
        : m_value(Simd::Vec4::FromVec3(v.GetSimdValue()))
    {
        m_w = w;
    }


    AZ_MATH_INLINE Quaternion::Quaternion(Simd::Vec4::FloatArgType value)
        : m_value(value)
    {
        ;
    }


    AZ_MATH_INLINE Quaternion Quaternion::CreateIdentity()
    {
        return Quaternion(Simd::Vec4::LoadImmediate(0.0f, 0.0f, 0.0f, 1.0f));
    }


    AZ_MATH_INLINE Quaternion Quaternion::CreateZero()
    {
        return Quaternion(Simd::Vec4::ZeroFloat());
    }


    AZ_MATH_INLINE Quaternion Quaternion::CreateFromFloat4(const float* values)
    {
        return Quaternion(Simd::Vec4::LoadUnaligned(values));
    }


    AZ_MATH_INLINE Quaternion Quaternion::CreateFromVector3(const Vector3& v)
    {
        return Quaternion(Simd::Vec4::FromVec3(v.GetSimdValue()));
    }

    AZ_MATH_INLINE Quaternion Quaternion::CreateFromEulerDegreesXYZ(const Vector3& eulerDegrees)
    {
        return CreateFromEulerRadiansXYZ(Vector3DegToRad(eulerDegrees));
    }

    AZ_MATH_INLINE Quaternion Quaternion::CreateFromEulerDegreesYXZ(const Vector3& eulerDegrees)
    {
        return CreateFromEulerRadiansYXZ(Vector3DegToRad(eulerDegrees));
    }

    AZ_MATH_INLINE Quaternion Quaternion::CreateFromEulerDegreesZYX(const Vector3& eulerDegrees)
    {
        return CreateFromEulerRadiansZYX(Vector3DegToRad(eulerDegrees));
    }

    AZ_MATH_INLINE Quaternion Quaternion::CreateFromVector3AndValue(const Vector3& v, float w)
    {
        return Quaternion(v, w);
    }

    AZ_MATH_INLINE Quaternion Quaternion::CreateRotationX(float angleInRadians)
    {
        const float halfAngle = 0.5f * angleInRadians;
        float sin, cos;
        SinCos(halfAngle, sin, cos);
        return Quaternion(sin, 0.0f, 0.0f, cos);
    }


    AZ_MATH_INLINE Quaternion Quaternion::CreateRotationY(float angleInRadians)
    {
        const float halfAngle = 0.5f * angleInRadians;
        float sin, cos;
        SinCos(halfAngle, sin, cos);
        return Quaternion(0.0f, sin, 0.0f, cos);
    }


    AZ_MATH_INLINE Quaternion Quaternion::CreateRotationZ(float angleInRadians)
    {
        const float halfAngle = 0.5f * angleInRadians;
        float sin, cos;
        SinCos(halfAngle, sin, cos);
        return Quaternion(0.0f, 0.0f, sin, cos);
    }


    AZ_MATH_INLINE Quaternion Quaternion::CreateFromAxisAngle(const Vector3& axis, float angle)
    {
        const float halfAngle = 0.5f * angle;
        float sin, cos;
        SinCos(halfAngle, sin, cos);
        return CreateFromVector3AndValue(sin * axis, cos);
    }


    AZ_MATH_INLINE Quaternion Quaternion::CreateFromScaledAxisAngle(const Vector3& scaledAxisAngle)
    {
        const AZ::Vector3 exponentialMap = scaledAxisAngle / 2.0f;
        const float halfAngle = exponentialMap.GetLength();

        if (halfAngle < AZ::Constants::FloatEpsilon)
        {
            return AZ::Quaternion::CreateFromVector3AndValue(exponentialMap, 1.0f).GetNormalized();
        }
        else
        {
            float sin, cos;
            SinCos(halfAngle, sin, cos);
            return AZ::Quaternion::CreateFromVector3AndValue((sin / halfAngle) * exponentialMap, cos);
        }
    }


    AZ_MATH_INLINE void Quaternion::StoreToFloat4(float* values) const
    {
        Simd::Vec4::StoreUnaligned(values, m_value);
    }


    AZ_MATH_INLINE float Quaternion::GetX() const
    {
        return m_x;
    }


    AZ_MATH_INLINE float Quaternion::GetY() const
    {
        return m_y;
    }


    AZ_MATH_INLINE float Quaternion::GetZ() const
    {
        return m_z;
    }


    AZ_MATH_INLINE float Quaternion::GetW() const
    {
        return m_w;
    }


    AZ_MATH_INLINE float Quaternion::GetElement(int index) const
    {
        AZ_MATH_ASSERT((index >= 0) && (index < 4), "Invalid index for component access!\n");
        return m_values[index];
    }


    AZ_MATH_INLINE void Quaternion::SetX(float x)
    {
        m_x = x;
    }


    AZ_MATH_INLINE void Quaternion::SetY(float y)
    {
        m_y = y;
    }


    AZ_MATH_INLINE void Quaternion::SetZ(float z)
    {
        m_z = z;
    }


    AZ_MATH_INLINE void Quaternion::SetW(float w)
    {
        m_w = w;
    }


    AZ_MATH_INLINE void Quaternion::SetElement(int index, float v)
    {
        AZ_MATH_ASSERT((index >= 0) && (index < 4), "Invalid index for component access!\n");
        m_values[index] = v;
    }


    AZ_MATH_INLINE float Quaternion::operator()(int index) const
    {
        return GetElement(index);
    }


    AZ_MATH_INLINE void Quaternion::Set(float x)
    {
        m_value = Simd::Vec4::Splat(x);
    }


    AZ_MATH_INLINE void Quaternion::Set(float x, float y, float z, float w)
    {
        m_value = Simd::Vec4::LoadImmediate(x, y, z, w);
    }


    AZ_MATH_INLINE void Quaternion::Set(const Vector3& v, float w)
    {
        m_value = Simd::Vec4::FromVec3(v.GetSimdValue());
        m_w = w;
    }


    AZ_MATH_INLINE void Quaternion::Set(const float values[])
    {
        m_value = Simd::Vec4::LoadUnaligned(values);
    }


    AZ_MATH_INLINE Quaternion Quaternion::GetConjugate() const
    {
#if AZ_TRAIT_USE_PLATFORM_SIMD_SCALAR
        return Quaternion(-m_x, -m_y, -m_z, m_w);
#else
        const Simd::Vec4::Int32Type conjugateMask(Simd::Vec4::LoadAligned((const int32_t*)&Simd::g_negateXYZMask));
        return Quaternion(Simd::Vec4::Xor(m_value, Simd::Vec4::CastToFloat(conjugateMask)));
#endif
    }


    AZ_MATH_INLINE Quaternion Quaternion::GetInverseFast() const
    {
        return GetConjugate();
    }


    AZ_MATH_INLINE void Quaternion::InvertFast()
    {
        *this = GetInverseFast();
    }


    AZ_MATH_INLINE Quaternion Quaternion::GetInverseFull() const
    {
        return GetConjugate() / GetLengthSq();
    }


    AZ_MATH_INLINE void Quaternion::InvertFull()
    {
        *this = GetInverseFull();
    }


    AZ_MATH_INLINE float Quaternion::Dot(const Quaternion& q) const
    {
#if AZ_TRAIT_USE_PLATFORM_SIMD_SCALAR
        return m_x * q.m_x + m_y * q.m_y + m_z * q.m_z + m_w * q.m_w;
#else
        return Simd::Vec1::SelectIndex0(Simd::Vec4::Dot(m_value, q.m_value));
#endif
    }


    AZ_MATH_INLINE float Quaternion::GetLengthSq() const
    {
        return Dot(*this);
    }


    AZ_MATH_INLINE float Quaternion::GetLength() const
    {
        const Simd::Vec1::FloatType lengthSq = Simd::Vec4::Dot(m_value, m_value);
        return Simd::Vec1::SelectIndex0(Simd::Vec1::Sqrt(lengthSq));
    }


    AZ_MATH_INLINE float Quaternion::GetLengthEstimate() const
    {
        const Simd::Vec1::FloatType lengthSq = Simd::Vec4::Dot(m_value, m_value);
        return Simd::Vec1::SelectIndex0(Simd::Vec1::SqrtEstimate(lengthSq));
    }


    AZ_MATH_INLINE float Quaternion::GetLengthReciprocal() const
    {
        const Simd::Vec1::FloatType lengthSq = Simd::Vec4::Dot(m_value, m_value);
        return Simd::Vec1::SelectIndex0(Simd::Vec1::SqrtInv(lengthSq));
    }


    AZ_MATH_INLINE float Quaternion::GetLengthReciprocalEstimate() const
    {
        const Simd::Vec1::FloatType lengthSq = Simd::Vec4::Dot(m_value, m_value);
        return Simd::Vec1::SelectIndex0(Simd::Vec1::SqrtInvEstimate(lengthSq));
    }


    AZ_MATH_INLINE Quaternion Quaternion::GetNormalized() const
    {
        return Quaternion(Simd::Vec4::Normalize(m_value));
    }


    AZ_MATH_INLINE Quaternion Quaternion::GetNormalizedEstimate() const
    {
        return Quaternion(Simd::Vec4::NormalizeEstimate(m_value));
    }


    AZ_MATH_INLINE void Quaternion::Normalize()
    {
        *this = GetNormalized();
    }


    AZ_MATH_INLINE void Quaternion::NormalizeEstimate()
    {
        *this = GetNormalizedEstimate();
    }


    AZ_MATH_INLINE float Quaternion::NormalizeWithLength()
    {
        const float length = Simd::Vec1::SelectIndex0(
            Simd::Vec1::Sqrt(Simd::Vec4::Dot(m_value, m_value)));
        m_value = Simd::Vec4::Div(m_value, Simd::Vec4::Splat(length));
        return length;
    }


    AZ_MATH_INLINE float Quaternion::NormalizeWithLengthEstimate()
    {
        const float length = Simd::Vec1::SelectIndex0(
            Simd::Vec1::SqrtEstimate(Simd::Vec4::Dot(m_value, m_value)));
        m_value = Simd::Vec4::Div(m_value, Simd::Vec4::Splat(length));
        return length;
    }


    AZ_MATH_INLINE Quaternion Quaternion::GetShortestEquivalent() const
    {
        if (GetW() < 0.0f)
        {
            return -(*this);
        }

        return *this;
    }


    AZ_MATH_INLINE void Quaternion::ShortestEquivalent()
    {
        *this = GetShortestEquivalent();
    }


    AZ_MATH_INLINE Quaternion Quaternion::Lerp(const Quaternion& dest, float t) const
    {
        if (Dot(dest) >= 0.0f)
        {
            return (*this) * (1.0f - t) + dest * t;
        }

        return (*this) * (1.0f - t) - dest * t;
    }


    AZ_MATH_INLINE Quaternion Quaternion::NLerp(const Quaternion& dest, float t) const
    {
        Quaternion result = Lerp(dest, t);
        result.Normalize();
        return result;
    }


    AZ_MATH_INLINE Quaternion Quaternion::Squad(const Quaternion& dest, const Quaternion& in, const Quaternion& out, float t) const
    {
        float k = 2.0f * (1.0f - t) * t;
        Quaternion temp1 = in.Slerp(out, t);
        Quaternion temp2 = Slerp(dest, t);
        return temp1.Slerp(temp2, k);
    }


    AZ_MATH_INLINE bool Quaternion::IsClose(const Quaternion& q, float tolerance) const
    {
#if AZ_TRAIT_USE_PLATFORM_SIMD_SCALAR
        return ((fabsf(q.m_x - m_x) <= tolerance) && (fabsf(q.m_y - m_y) <= tolerance) && (fabsf(q.m_z - m_z) <= tolerance) && (fabsf(q.m_w - m_w) <= tolerance));
#else
        Simd::Vec4::FloatType absDiff = Simd::Vec4::Abs(Simd::Vec4::Sub(q.m_value, m_value));
        return Simd::Vec4::CmpAllLt(absDiff, Simd::Vec4::Splat(tolerance));
#endif
    }


    AZ_MATH_INLINE bool Quaternion::IsIdentity(float tolerance) const
    {
#if AZ_TRAIT_USE_PLATFORM_SIMD_SCALAR
        return (fabsf(m_x) <= tolerance) && (fabsf(m_y) <= tolerance) && (fabsf(m_z) <= tolerance) && (fabsf(m_w) >= (1.0f - tolerance));
#else
        return IsClose(CreateIdentity(), tolerance);
#endif
    }


    AZ_MATH_INLINE bool Quaternion::IsZero(float tolerance) const
    {
#if AZ_TRAIT_USE_PLATFORM_SIMD_SCALAR
        return (fabsf(m_x) <= tolerance) && (fabsf(m_y) <= tolerance) && (fabsf(m_z) <= tolerance) && (fabsf(m_w) <= tolerance);
#else
        Simd::Vec4::FloatType absDiff = Simd::Vec4::Abs(m_value);
        return Simd::Vec4::CmpAllLt(absDiff, Simd::Vec4::Splat(tolerance));
#endif
    }


    AZ_MATH_INLINE Quaternion& Quaternion::operator=(const Quaternion& rhs)
    {
        m_value = rhs.m_value;
        return *this;
    }


    AZ_MATH_INLINE Quaternion Quaternion::operator-() const
    {
#if AZ_TRAIT_USE_PLATFORM_SIMD_SCALAR
        return Quaternion(-m_x, -m_y, -m_z, -m_w);
#else
        const Simd::Vec4::Int32Type negateMask(Simd::Vec4::LoadAligned((const int32_t*)&Simd::g_negateMask));
        return Quaternion(Simd::Vec4::Xor(m_value, Simd::Vec4::CastToFloat(negateMask)));
#endif
    }


    AZ_MATH_INLINE Quaternion Quaternion::operator+(const Quaternion& q) const
    {
        return Quaternion(Simd::Vec4::Add(m_value, q.m_value));
    }


    AZ_MATH_INLINE Quaternion Quaternion::operator-(const Quaternion& q) const
    {
        return Quaternion(Simd::Vec4::Sub(m_value, q.m_value));
    }


    AZ_MATH_INLINE Quaternion Quaternion::operator*(const Quaternion& q) const
    {
        return Quaternion(Simd::Vec4::QuaternionMultiply(m_value, q.m_value));
    }


    AZ_MATH_INLINE Quaternion Quaternion::operator*(float multiplier) const
    {
        return Quaternion(Simd::Vec4::Mul(m_value, Simd::Vec4::Splat(multiplier)));
    }


    AZ_MATH_INLINE Quaternion Quaternion::operator/(float divisor) const
    {
        return Quaternion(Simd::Vec4::Mul(m_value, Simd::Vec4::Splat(1.0f / divisor)));
    }


    AZ_MATH_INLINE Quaternion& Quaternion::operator+=(const Quaternion& q)
    {
        *this = *this + q;
        return *this;
    }


    AZ_MATH_INLINE Quaternion& Quaternion::operator-=(const Quaternion& q)
    {
        *this = *this - q;
        return *this;
    }


    AZ_MATH_INLINE Quaternion& Quaternion::operator*=(const Quaternion& q)
    {
        *this = *this * q;
        return *this;
    }


    AZ_MATH_INLINE Quaternion& Quaternion::operator*=(float multiplier)
    {
        *this = *this * multiplier;
        return *this;
    }


    AZ_MATH_INLINE Quaternion& Quaternion::operator/=(float divisor)
    {
        *this = *this / divisor;
        return *this;
    }


    AZ_MATH_INLINE bool Quaternion::operator==(const Quaternion& rhs) const
    {
        return Simd::Vec4::CmpAllEq(m_value, rhs.m_value);
    }


    AZ_MATH_INLINE bool Quaternion::operator!=(const Quaternion& rhs) const
    {
        return !Simd::Vec4::CmpAllEq(m_value, rhs.m_value);
    }


    AZ_MATH_INLINE Vector3 Quaternion::TransformVector(const Vector3& v) const
    {
        return Vector3(Simd::Vec4::QuaternionTransform(m_value, v.GetSimdValue()));
    }


    AZ_MATH_INLINE float Quaternion::GetAngle() const
    {
        return 2.0f * Acos(AZ::GetClamp(GetW(), -1.0f, 1.0f));
    }


    AZ_MATH_INLINE Vector3 Quaternion::GetEulerDegrees() const
    {
        return Vector3RadToDeg(GetEulerRadians());
    }


    AZ_MATH_INLINE Vector3 Quaternion::GetEulerDegreesXYZ() const
    {
        return Vector3RadToDeg(GetEulerRadiansXYZ());
    }


    AZ_MATH_INLINE Vector3 Quaternion::GetEulerDegreesYXZ() const
    {
        return Vector3RadToDeg(GetEulerRadiansYXZ());
    }


    AZ_MATH_INLINE Vector3 Quaternion::GetEulerDegreesZYX() const
    {
        return Vector3RadToDeg(GetEulerRadiansZYX());
    }


    AZ_MATH_INLINE void Quaternion::SetFromEulerDegrees(const Vector3& eulerDegrees)
    {
        SetFromEulerRadians(Vector3DegToRad(eulerDegrees));
    }


    AZ_MATH_INLINE Vector3 Quaternion::GetImaginary() const
    {
        return Vector3(Simd::Vec4::ToVec3(m_value));
    }


    AZ_MATH_INLINE bool Quaternion::IsFinite() const
    {
        return IsFiniteFloat(GetX()) && IsFiniteFloat(GetY()) && IsFiniteFloat(GetZ()) && IsFiniteFloat(GetW());
    }


    AZ_MATH_INLINE Simd::Vec4::FloatType Quaternion::GetSimdValue() const
    {
        return m_value;
    }


    AZ_MATH_INLINE Quaternion Quaternion::GetAbs() const
    {
        return Quaternion(Simd::Vec4::Abs(m_value));
    }


    // Non-member functionality belonging to the AZ namespace
    AZ_MATH_INLINE Vector3 ConvertQuaternionToEulerDegrees(const Quaternion& q)
    {
        return q.GetEulerDegrees();
    }


    AZ_MATH_INLINE Vector3 ConvertQuaternionToEulerRadians(const Quaternion& q)
    {
        return q.GetEulerRadians();
    }


    AZ_MATH_INLINE Quaternion ConvertEulerRadiansToQuaternion(const Vector3& eulerRadians)
    {
        Quaternion q;
        q.SetFromEulerRadians(eulerRadians);
        return q;
    }


    AZ_MATH_INLINE Quaternion ConvertEulerDegreesToQuaternion(const Vector3& eulerDegrees)
    {
        Quaternion q;
        q.SetFromEulerDegrees(eulerDegrees);
        return q;
    }


    AZ_MATH_INLINE void ConvertQuaternionToAxisAngle(const Quaternion& quat, Vector3& outAxis, float& outAngle)
    {
        quat.ConvertToAxisAngle(outAxis, outAngle);
    }


    AZ_MATH_INLINE Quaternion operator*(float multiplier, const Quaternion& rhs)
    {
        return rhs * multiplier;
    }
}
