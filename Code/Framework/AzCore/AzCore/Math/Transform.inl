/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace AZ
{
    AZ_MATH_INLINE Transform::Transform(const Vector3& translation, const Quaternion& rotation, float scale)
        : m_rotation(rotation)
        , m_translationScale(Simd::Vec4::ReplaceIndex3(Simd::Vec4::FromVec3(translation.GetSimdValue()), scale))
    {
    }

    AZ_MATH_INLINE Transform Transform::CreateIdentity()
    {
        Transform result;
        result.m_rotation = Quaternion::CreateIdentity();
        result.m_translationScale = Simd::Vec4::LoadImmediate(0.0f, 0.0f, 0.0f, 1.0f);
        return result;
    }

    AZ_MATH_INLINE Transform Transform::CreateRotationX(float angle)
    {
        return CreateFromQuaternion(Quaternion::CreateRotationX(angle));
    }

    AZ_MATH_INLINE Transform Transform::CreateRotationY(float angle)
    {
        return CreateFromQuaternion(Quaternion::CreateRotationY(angle));
    }

    AZ_MATH_INLINE Transform Transform::CreateRotationZ(float angle)
    {
        return CreateFromQuaternion(Quaternion::CreateRotationZ(angle));
    }

    AZ_MATH_INLINE Transform Transform::CreateFromQuaternion(const Quaternion& q)
    {
        Transform result;
        result.m_rotation = q;
        result.m_translationScale = Simd::Vec4::LoadImmediate(0.0f, 0.0f, 0.0f, 1.0f);
        return result;
    }

    AZ_MATH_INLINE Transform Transform::CreateFromQuaternionAndTranslation(const class Quaternion& q, const Vector3& p)
    {
        Transform result;
        result.m_rotation = q;
        result.m_translationScale = Simd::Vec4::ReplaceIndex3(Simd::Vec4::FromVec3(p.GetSimdValue()), 1.0f);
        return result;
    }

    AZ_MATH_INLINE Transform Transform::CreateUniformScale(float scale)
    {
        Transform result;
        result.m_rotation = Quaternion::CreateIdentity();
        result.m_translationScale = Simd::Vec4::LoadImmediate(0.0f, 0.0f, 0.0f, scale);
        return result;
    }

    AZ_MATH_INLINE Transform Transform::CreateTranslation(const Vector3& translation)
    {
        Transform result;
        result.m_rotation = Quaternion::CreateIdentity();
        result.m_translationScale = Simd::Vec4::ReplaceIndex3(Simd::Vec4::FromVec3(translation.GetSimdValue()), 1.0f);
        return result;
    }

    AZ_MATH_INLINE Vector3 Transform::GetBasis(int32_t index) const
    {
        switch (index)
        {
        case 0:
            return GetBasisX();
        case 1:
            return GetBasisY();
        case 2:
            return GetBasisZ();
        default:
            AZ_MATH_ASSERT(false, "Invalid index for component access.\n");
            return Vector3::CreateZero();
        }
    }

    AZ_MATH_INLINE Vector3 Transform::GetBasisX() const
    {
        return m_rotation.TransformVector(Vector3::CreateAxisX(m_scale));
    }

    AZ_MATH_INLINE Vector3 Transform::GetBasisY() const
    {
        return m_rotation.TransformVector(Vector3::CreateAxisY(m_scale));
    }

    AZ_MATH_INLINE Vector3 Transform::GetBasisZ() const
    {
        return m_rotation.TransformVector(Vector3::CreateAxisZ(m_scale));
    }

    AZ_MATH_INLINE Vector3 Transform::GetTranslation() const
    {
        return Vector3(Simd::Vec4::ToVec3(m_translationScale));
    }

    AZ_MATH_INLINE void Transform::SetTranslation(float x, float y, float z)
    {
        m_translationX = x;
        m_translationY = y;
        m_translationZ = z;
    }

    AZ_MATH_INLINE void Transform::SetTranslation(const Vector3& v)
    {
        // Single 16-byte aligned blend+store: take xyz from v, keep W (scale) from existing memory.
        // Avoids the scalar extraction that 3 separate m_x/y/z writes would force from v's SIMD register.
        m_translationScale = Simd::Vec4::ReplaceIndex3(Simd::Vec4::FromVec3(v.GetSimdValue()), Simd::Vec4::SplatIndex3(m_translationScale));
    }

    AZ_MATH_INLINE const Quaternion& Transform::GetRotation() const
    {
        return m_rotation;
    }

    AZ_MATH_INLINE void Transform::SetRotation(const Quaternion& rotation)
    {
        m_rotation = rotation;
    }

    AZ_MATH_INLINE float Transform::GetUniformScale() const
    {
        return m_scale;
    }

    AZ_MATH_INLINE void Transform::SetUniformScale(const float scale)
    {
        m_scale = scale;
    }

    AZ_MATH_INLINE float Transform::ExtractUniformScale()
    {
        const float scale = m_scale;
        m_scale = 1.0f;
        return scale;
    }

    AZ_MATH_INLINE void Transform::MultiplyByUniformScale(float scale)
    {
        m_scale *= scale;
    }

    AZ_MATH_INLINE Transform Transform::operator*(const Transform& rhs) const
    {
        Transform result;
        result.m_rotation = m_rotation * rhs.m_rotation;
        // translation = rotate(scale * rhs.translation) + translation
        const Simd::Vec3::FloatType scaled = Simd::Vec3::Mul(Simd::Vec4::ToVec3(rhs.m_translationScale), Simd::Vec3::Splat(m_scale));
        const Simd::Vec3::FloatType rotated = Simd::Vec4::QuaternionTransform(m_rotation.GetSimdValue(), scaled);
        const Simd::Vec3::FloatType newTransform = Simd::Vec3::Add(rotated, Simd::Vec4::ToVec3(m_translationScale));
        result.m_translationScale = Simd::Vec4::ReplaceIndex3(Simd::Vec4::FromVec3(newTransform), m_scale * rhs.m_scale);
        return result;
    }

    AZ_MATH_INLINE Transform& Transform::operator*=(const Transform& rhs)
    {
        *this = *this * rhs;
        return *this;
    }

    AZ_MATH_INLINE Vector3 Transform::TransformPoint(const Vector3& rhs) const
    {
        const Simd::Vec3::FloatType scaled = Simd::Vec3::Mul(rhs.GetSimdValue(), Simd::Vec3::Splat(m_scale));
        const Simd::Vec3::FloatType rotated = Simd::Vec4::QuaternionTransform(m_rotation.GetSimdValue(), scaled);
        return Vector3(Simd::Vec3::Add(rotated, Simd::Vec4::ToVec3(m_translationScale)));
    }

    AZ_MATH_INLINE Vector4 Transform::TransformPoint(const Vector4& rhs) const
    {
        const Simd::Vec3::FloatType scaled = Simd::Vec3::Mul(Simd::Vec4::ToVec3(rhs.GetSimdValue()), Simd::Vec3::Splat(m_scale));
        const Simd::Vec3::FloatType rotated = Simd::Vec4::QuaternionTransform(m_rotation.GetSimdValue(), scaled);
        const Simd::Vec3::FloatType result = Simd::Vec3::Madd(Simd::Vec4::ToVec3(m_translationScale), Simd::Vec3::Splat(rhs(3)), rotated);
        return Vector4::CreateFromVector3AndFloat(Vector3(result), rhs(3));
    }

    AZ_MATH_INLINE Vector3 Transform::TransformVector(const Vector3& rhs) const
    {
        return Vector3(Simd::Vec4::QuaternionTransform(m_rotation.GetSimdValue(), Simd::Vec3::Mul(rhs.GetSimdValue(), Simd::Vec3::Splat(m_scale))));
    }

    AZ_MATH_INLINE Transform Transform::GetInverse() const
    {
        Transform out;
        out.m_rotation = m_rotation.GetConjugate();
        const float inverseScale = 1.0f / m_scale;
        const Simd::Vec3::FloatType rotated = Simd::Vec4::QuaternionTransform(out.m_rotation.GetSimdValue(), Simd::Vec4::ToVec3(m_translationScale));
        const Simd::Vec3::FloatType newTransform = Simd::Vec3::Mul(rotated, Simd::Vec3::Splat(-inverseScale));
        out.m_translationScale = Simd::Vec4::ReplaceIndex3(Simd::Vec4::FromVec3(newTransform), inverseScale);
        return out;
    }

    AZ_MATH_INLINE void Transform::Invert()
    {
        *this = GetInverse();
    }

    AZ_MATH_INLINE bool Transform::IsOrthogonal(float tolerance) const
    {
        return AZ::IsClose(m_scale, 1.0f, tolerance);
    }

    AZ_MATH_INLINE Transform Transform::GetOrthogonalized() const
    {
        Transform result;
        result.m_rotation = m_rotation;
        result.m_translationX = m_translationX;
        result.m_translationY = m_translationY;
        result.m_translationZ = m_translationZ;
        result.m_scale = 1.0f;
        return result;
    }

    AZ_MATH_INLINE void Transform::Orthogonalize()
    {
        m_scale = 1.0f;
    }

    AZ_MATH_INLINE bool Transform::IsClose(const Transform& rhs, float tolerance) const
    {
        // Packed 4-lane compare on {tx, ty, tz, scale}.
        return m_rotation.IsClose(rhs.m_rotation, tolerance)
            && Vector4(m_translationScale).IsClose(Vector4(rhs.m_translationScale), tolerance);
    }

    AZ_MATH_INLINE bool Transform::operator==(const Transform& rhs) const
    {
        // Packed 4-lane exact equality on {tx, ty, tz, scale}.
        return m_rotation == rhs.m_rotation
            && Simd::Vec4::CmpAllEq(m_translationScale, rhs.m_translationScale);
    }

    AZ_MATH_INLINE bool Transform::operator!=(const Transform& rhs) const
    {
        return !operator==(rhs);
    }

    AZ_MATH_INLINE Vector3 Transform::GetEulerDegrees() const
    {
        return m_rotation.GetEulerDegrees();
    }

    AZ_MATH_INLINE Vector3 Transform::GetEulerRadians() const
    {
        return m_rotation.GetEulerRadians();
    }

    AZ_MATH_INLINE void Transform::SetFromEulerDegrees(const Vector3& eulerDegrees)
    {
        m_translationScale = Simd::Vec4::LoadImmediate(0.0f, 0.0f, 0.0f, 1.0f);
        m_rotation.SetFromEulerDegrees(eulerDegrees);
    }

    AZ_MATH_INLINE void Transform::SetFromEulerRadians(const Vector3& eulerRadians)
    {
        m_translationScale = Simd::Vec4::LoadImmediate(0.0f, 0.0f, 0.0f, 1.0f);
        m_rotation.SetFromEulerRadians(eulerRadians);
    }

    AZ_MATH_INLINE bool Transform::IsFinite() const
    {
        // Packed Vec4 finite check on {tx, ty, tz, scale}: abs(v) <= FloatMax
        // catches both NaN (unordered compare returns false) and Inf (Inf > FloatMax)
        return m_rotation.IsFinite() && Simd::Vec4::CmpAllLtEq(Simd::Vec4::Abs(m_translationScale), Simd::Vec4::Splat(Constants::FloatMax));
    }

    // Non-member functionality belonging to the AZ namespace
    AZ_MATH_INLINE Vector3 ConvertTransformToEulerDegrees(const Transform& transform)
    {
        return transform.GetEulerDegrees();
    }

    AZ_MATH_INLINE Vector3 ConvertTransformToEulerRadians(const Transform& transform)
    {
        return transform.GetEulerRadians();
    }

    AZ_MATH_INLINE Transform ConvertEulerDegreesToTransform(const Vector3& eulerDegrees)
    {
        Transform finalRotation;
        finalRotation.SetFromEulerDegrees(eulerDegrees);
        return finalRotation;
    }

    AZ_MATH_INLINE Transform ConvertEulerRadiansToTransform(const Vector3& eulerRadians)
    {
        Transform finalRotation;
        finalRotation.SetFromEulerRadians(eulerRadians);
        return finalRotation;
    }
}
