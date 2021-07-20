/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/MathUtils.h>

namespace AZ
{
    //! Limits for transform scale values.
    //! The scale should not be zero to avoid problems with inverting.
    //! @{
    static constexpr float MinTransformScale = 1e-2f;
    static constexpr float MaxTransformScale = 1e9f;
    //! @}

    //! The basic transformation class, represented using a quaternion rotation, float scale and vector translation.
    //! By design, cannot represent skew transformations.
    class Transform
    {
    public:

        AZ_TYPE_INFO(Transform, "{5D9958E9-9F1E-4985-B532-FFFDE75FEDFD}");

        using Axis = Constants::Axis;

        //! AzCore Reflection.
        //! @param context reflection context
        static void Reflect(ReflectContext* context);

        //! Default constructor does not initialize the matrix.
        Transform() = default;

        //! Construct a transform from components.
        Transform(const Vector3& translation, const Quaternion& rotation, float scale);

        //! Creates an identity transform.
        static Transform CreateIdentity();

        //! Sets the matrix to be a rotation around a specified axis.
        //! @{
        static Transform CreateRotationX(float angle);
        static Transform CreateRotationY(float angle);
        static Transform CreateRotationZ(float angle);
        //! @}

        //! Sets the matrix from a quaternion, translation is set to zero.
        static Transform CreateFromQuaternion(const class Quaternion& q);

        //! Sets the matrix from a quaternion and a translation.
        static Transform CreateFromQuaternionAndTranslation(const class Quaternion& q, const Vector3& p);

        //! Constructs from a Matrix3x3, translation is set to zero.
        //! Note that Transform only allows uniform scale, so if the matrix has different scale values along its axes,
        //! the largest matrix scale value will be used to uniformly scale the Transform.
        static Transform CreateFromMatrix3x3(const class Matrix3x3& value);

        //! Constructs from a Matrix3x3 and translation Vector3.
        //! Note that Transform only allows uniform scale, so if the matrix has different scale values along its axes,
        //! the largest matrix scale value will be used to uniformly scale the Transform.
        static Transform CreateFromMatrix3x3AndTranslation(const class Matrix3x3& value, const Vector3& p);

        //! Constructs from a Matrix3x4.
        //! Note that Transform only allows uniform scale, so if the matrix has different scale values along its axes,
        //! the largest matrix scale value will be used to uniformly scale the Transform.
        static Transform CreateFromMatrix3x4(const Matrix3x4& value);

        //! Sets the transform to apply (uniform) scale only, no rotation or translation.
        static Transform CreateUniformScale(const float scale);

        //! Sets the matrix to be a translation matrix, rotation part is set to identity.
        static Transform CreateTranslation(const Vector3& translation);

        //! Creates a "look at" transform.
        //! Given a source position and target position, computes a transform at the source position that points
        //! toward the target along a chosen local-space axis.
        //! @param from The source position (world space).
        //! @param to The target position (world space).
        //! @param forwardAxis The local-space basis axis that should "look at" the target.
        //! @return The resulting Matrix3x4 or the identity if the source and target coincide.
        static Transform CreateLookAt(const Vector3& from, const Vector3& to, Transform::Axis forwardAxis = Transform::Axis::YPositive);

        static const Transform& Identity();

        Vector3 GetBasis(int index) const;
        Vector3 GetBasisX() const;
        Vector3 GetBasisY() const;
        Vector3 GetBasisZ() const;
        void GetBasisAndTranslation(Vector3* basisX, Vector3* basisY, Vector3* basisZ, Vector3* pos) const;

        const Vector3& GetTranslation() const;
        void SetTranslation(float x, float y, float z);
        void SetTranslation(const Vector3& v);

        const Quaternion& GetRotation() const;
        void SetRotation(const Quaternion& rotation);

        float GetUniformScale() const;
        void SetUniformScale(const float scale);

        //! Sets the transform's scale to a unit value and returns the previous scale value.
        float ExtractUniformScale();

        void MultiplyByUniformScale(float scale);

        Transform operator*(const Transform& rhs) const;
        Transform& operator*=(const Transform& rhs);

        Vector3 TransformPoint(const Vector3& rhs) const;
        Vector4 TransformPoint(const Vector4& rhs) const;

        //! Applies rotation and scale, but not translation.
        Vector3 TransformVector(const Vector3& rhs) const;

        Transform GetInverse() const;
        void Invert();

        bool IsOrthogonal(float tolerance = Constants::Tolerance) const;
        Transform GetOrthogonalized() const;

        void Orthogonalize();

        bool IsClose(const Transform& rhs, float tolerance = Constants::Tolerance) const;

        bool operator==(const Transform& rhs) const;
        bool operator!=(const Transform& rhs) const;

        Vector3 GetEulerDegrees() const;
        Vector3 GetEulerRadians() const;
        void SetFromEulerDegrees(const Vector3& eulerDegrees);
        void SetFromEulerRadians(const Vector3& eulerRadians);

        bool IsFinite() const;

    private:

        Quaternion m_rotation;
        float m_scale;
        Vector3 m_translation;
    };

    extern const Transform g_transformIdentity;

    //! Non-member functionality belonging to the AZ namespace
    //!
    //! Converts a transform to corresponding component-wise Euler angles.
    //! @param Transform transform The input transform to decompose.
    //! @return Vector3 A vector containing component-wise rotation angles in degrees.
    Vector3 ConvertTransformToEulerDegrees(const Transform& transform);
    Vector3 ConvertTransformToEulerRadians(const Transform& transform);

    //! Create a transform from Euler Angles (e.g. rotation angles in X, Y, and Z)
    //! @param Vector3 eulerDegrees A vector containing component-wise rotation angles in degrees.
    //! @return Transform A transform made from the rotational components.
    Transform ConvertEulerDegreesToTransform(const Vector3& eulerDegrees);

    //! Create a rotation transform from Euler angles in radian around each base axis.
    //!        This version uses precise sin/cos for a more accurate conversion.
    //! @param Vector3 eulerDegrees A vector containing component-wise rotation angles in radian.
    //! @return Transform A transform made from the composite of rotations first around z-axis, and y-axis and then x-axis.
    Transform ConvertEulerRadiansToTransform(const Vector3& eulerRadians);
} // namespace AZ

#include <AzCore/Math/Transform.inl>
